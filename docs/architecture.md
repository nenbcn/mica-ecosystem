# MICA Ecosystem - System Architecture

> **Architecture by**: gaesca04 (Computer Engineer)  
> **Last Updated**: 28 November 2025  
> **Type**: Monorepo with multiple apps sharing common libraries

## 1. Ecosystem Overview

**MICA Ecosystem** is an IoT monorepo containing multiple applications that share common services and drivers:

### Applications
1. **Recirculator** (Production): ESP32-C3 water recirculation controller with temperature monitoring
2. **Gateway** (Future): ESP32 hub for sensors with LoRa transmission

### Shared Components
- **Services**: WiFi, MQTT, OTA, storage (business logic)
- **Drivers**: Buttons, LEDs (hardware abstraction)
- **Utils**: Logging, time management

**Platform**: ESP32 family, FreeRTOS, Arduino Framework, PlatformIO monorepo

---

## 2. Monorepo Structure

```
mica-ecosystem/
├── apps/                    # Individual applications
│   ├── recirculator/       # App 1: Water pump control
│   └── gateway/            # App 2: LoRa sensor hub (future)
│
├── lib/                     # Shared libraries (PlatformIO standard)
│   ├── services/           # Business logic (shared)
│   ├── drivers/            # Hardware abstraction (shared)
│   └── utils/              # Helper utilities (shared)
│
├── include/                 # Global configuration
│   ├── config.h            # Hardware pins, defines
│   └── secrets.h           # Credentials (gitignored)
│
└── docs/                    # Documentation
```

**Key principle**: Apps are independent but share code from `lib/` to avoid duplication.

---

## 3. Layered Architecture (4 Layers)

The system follows a **4-layer architecture pattern** for clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────────────┐
│                     APPLICATION LAYER                               │
│  Location: lib/application/ + apps/*/src/main.cpp                   │
│                                                                      │
│  - system_state  (Shared: Event Coordinator)                        │
│  - main.cpp      (Per-app: Entry Point)                             │
└─────────────────────────────┬───────────────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────────────┐
│                      SERVICES LAYER                                 │
│  Location: lib/services/ (SHARED across all apps)                   │
│                                                                      │
│  - wifi_connect      (WiFi management)                              │
│  - wifi_config_mode  (AP + Captive Portal)                          │
│  - mqtt_handler      (AWS IoT - generic)                            │
│  - ota_manager       (Firmware updates)                             │
│  - eeprom_config     (Persistent storage)                           │
│  - device_id         (Unique device identifier)                     │
└─────────────────────────────┬───────────────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────────────┐
│                      DRIVERS LAYER                                  │
│  Location: lib/drivers/ (SHARED) + apps/*/src/ (SPECIFIC)           │
│                                                                      │
│  SHARED (lib/drivers/):                                             │
│  - button_manager      (GPIO input)                                 │
│  - led_manager         (WS2812B NeoPixel)                           │
│                                                                      │
│  SPECIFIC (apps/recirculator/src/):                                 │
│  - relay_controller    (GPIO relay)                                 │
│  - temperature_sensor  (DS18B20 1-Wire)                             │
│  - displayManager      (SSD1306 OLED I2C)                           │
└─────────────────────────────┬───────────────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────────────┐
│                       UTILS LAYER                                   │
│  Location: lib/utils/ (SHARED helper utilities)                     │
│                                                                      │
│  - Log         (Logging with levels)                                │
│  - UtcClock    (NTP time sync)                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### Global Configuration
**Location**: `include/` (PlatformIO standard for global headers)
- `config.h` - Hardware pins, ESP32 variant defines
- `secrets.h` - AWS credentials, WiFi (gitignored)

---

## 4. Architecture Benefits

### ✅ Code Reuse
- **Services** (WiFi, MQTT, OTA, storage) shared by all apps
- **Shared drivers** (buttons, LEDs) reused across apps
- **Utils** (Log, UtcClock) common to all
- No code duplication for network/cloud infrastructure

### ✅ Separation of Concerns
- **Application** (per-app): Coordinates app-specific modules, manages state
- **Services** (shared): Business logic without hardware access
- **Drivers** (shared + per-app): Hardware abstraction
- **Utils** (shared): Helper functions (logging, time)

### ✅ Application Independence
- Each app has its own `system_state` coordinating its specific modules
- Recirculator: coordinates relay, temperature sensor, display
- Gateway (future): will coordinate LoRa, different sensors
- Apps share infrastructure (WiFi, MQTT) but not business logic

### ✅ Scalability
- Easy to add new apps (copy `apps/recirculator/` template)
- New apps automatically get all shared services
- Device-specific code isolated in app folder

### ✅ Maintainability
- Fix bug in service → All apps benefit
- Change driver API → Clear impact analysis
- Configs in one place (`include/`)

---

## 5. Modules by Layer

### 5.1 Application Layer (DEVICE-SPECIFIC)

**Location**: `apps/*/src/` (each app has its own)

**Why not shared?** Application layer coordinates device-specific modules (relay_controller, temperature_sensor, displayManager) which are unique to each app. Gateway would have its own system_state coordinating LoRa, different sensors, etc.

#### system_state (PER-APP)
- **Location**: `apps/recirculator/src/system_state.*`, `apps/gateway/src/system_state.*`
- **Role**: Event coordinator and state machine for the specific application
- **State Flow**: CONNECTING → WIFI → MQTT → OPERATIONAL
- **Interface**: `notifySystemState(event)`, `initializeSystemState()`
- **Responsibility**: Coordinates app-specific modules, manages task lifecycle
- **Specific to**: Each app (different modules per app)

#### main.cpp (PER-APP)
- **Location**: `apps/recirculator/src/main.cpp`, `apps/gateway/src/main.cpp`
- **Role**: Entry point and system initialization
- **Interface**: `setup()`, `loop()`
- **Responsibility**: Bootstraps system_state, initializes device-specific modules
- **Specific to**: Each app

---

### 5.2 Services Layer (ALL SHARED)

**Location**: `lib/services/`  
**Purpose**: Business services without direct hardware interaction

#### wifi_connect
- **Location**: `lib/services/wifi_connect/`
- **Role**: WiFi connection management
- **Features**: Auto-connect, exponential backoff, EEPROM credentials
- **Interface**: `initWiFi()`, `isWiFiConnected()`
- **Shared by**: All apps

#### wifi_config_mode
- **Location**: `lib/services/wifi_config_mode/`
- **Role**: Configuration portal via Access Point
- **Features**: AP mode + captive portal, credential storage
- **Trigger**: Long button press (5s)
- **Interface**: `startConfigMode()`, `isInConfigMode()`
- **Shared by**: All apps

#### mqtt_handler
- **Location**: `lib/services/mqtt_handler/`
- **Role**: AWS IoT Core communication
- **Architecture**: Generic with deviceType parameter, FreeRTOS queues
- **Topics**: `mica/dev/{command|telemetry}/{deviceType}/{deviceId}/{subject}`
- **Interface**: `mqttPublish()`, `mqttSubscribe()`, `isMqttConnected()`
- **Shared by**: All apps (deviceType parameterized)

#### ota_manager
- **Location**: `lib/services/ota_manager/`
- **Role**: Over-The-Air firmware updates
- **Triggers**: MQTT command or HTTP
- **Interface**: `initOTA()`, `handleOTA()`
- **Shared by**: All apps

#### eeprom_config
- **Location**: `lib/services/eeprom_config/`
- **Role**: Persistent configuration storage
- **Stores**: WiFi credentials, device-specific configs
- **Never Stores**: Runtime state (safety by design)
- **Interface**: `saveConfig()`, `loadConfig()`, key-value operations
- **Shared by**: All apps

#### device_id
- **Location**: `lib/services/device_id/`
- **Role**: Unique device identifier generation
- **Source**: ESP32 MAC address
- **Interface**: `getDeviceId()`, `initDeviceId()`
- **Shared by**: All apps

---

### 5.3 Drivers Layer

**Purpose**: Hardware abstraction and GPIO/peripheral interaction

#### Shared Drivers (lib/drivers/)

##### button_manager
- **Location**: `lib/drivers/button_manager/`
- **Role**: Button input handler (GPIO interrupt-based)
- **Hardware**: GPIO buttons with debouncing
- **Events**: `SHORT_PRESS`, `LONG_PRESS` (5s threshold)
- **Interface**: `initButtonManager()` - Publishes events to system_state
- **Note**: Generic, no device-specific logic
- **Shared by**: All apps (recirculator, gateway)

##### led_manager
- **Location**: `lib/drivers/led_manager/`
- **Role**: Visual status indicator
- **Hardware**: WS2812B NeoPixel (addressable RGB LED)
- **States**: Colors/patterns for WiFi, MQTT, errors, config mode
- **Interface**: `setLEDColor()`, `setLEDPattern()`
- **Shared by**: All apps

---

#### Device-Specific Drivers (apps/recirculator/src/drivers/)

##### relay_controller
- **Location**: `apps/recirculator/src/drivers/relay_controller.*`
- **Role**: Relay control with safety monitoring
- **Hardware**: GPIO output to relay module
- **Features**: ON/OFF control, timeout timer, temperature threshold
- **Safety**: Monitoring loop (1s), auto-shutoff on limits
- **Interface**: `activateRelay()`, `deactivateRelay()`, `isRelayActive()`
- **Specific to**: Recirculator only

##### temperature_sensor
- **Location**: `apps/recirculator/src/drivers/temperature_sensor.*`
- **Role**: Temperature measurement and publishing
- **Hardware**: DS18B20 digital sensor (1-Wire protocol)
- **Polling**: Every 5 seconds
- **Error Handling**: -127°C indicates sensor error, publishes error to MQTT
- **Interface**: `initTemperatureSensor()`, `getLatestTemperature()`
- **Specific to**: Recirculator only

##### displayManager
- **Location**: `apps/recirculator/src/drivers/displayManager.*`
- **Role**: Local visual feedback display
- **Hardware**: SSD1306 OLED 128x64 pixels (I2C)
- **Content**: Temperature, relay state, timers, system status
- **Interface**: `initDisplay()`, `updateDisplay()`
- **Specific to**: Recirculator only

---

### 5.4 Utils Layer (lib/utils/)

#### Log
- **Location**: `lib/utils/Log/`
- **Role**: Logging with severity levels
- **Levels**: DEBUG, INFO, WARN, ERROR
- **Interface**: `Log::info()`, `Log::error()`, etc.
- **Shared by**: All apps

#### UtcClock
- **Location**: `lib/utils/UtcClock/`
- **Role**: Time management and NTP sync
- **Features**: UTC time, timezone handling
- **Interface**: `getUtcTime()`, `syncNTP()`
- **Shared by**: All apps

---

### 5.5 Global Configuration (include/)

#### config.h
- **Location**: `include/config.h`
- **Content**: GPIO pins, ESP32 variant defines, hardware configuration
- **Example**: `#define BUTTON_PIN 9`, `#define RELAY_PIN 8`
- **Shared by**: All apps

#### secrets.h
- **Location**: `include/secrets.h` (gitignored)
- **Content**: AWS IoT credentials, WiFi defaults, API keys
- **Security**: Never committed to git
- **Template**: `docs/secrets.h.template`
- **Shared by**: All apps

---

### 5.6 Module Classification Summary

| Module | Layer | Location | Shared | Hardware |
|--------|-------|----------|--------|----------|
| system_state | Application | apps/*/src/ | ❌ No | ❌ No |
| main.cpp | Application | apps/*/src/ | ❌ No | ❌ No |
| wifi_connect | Services | lib/services/ | ✅ Yes | ❌ No |
| wifi_config_mode | Services | lib/services/ | ✅ Yes | ❌ No |
| mqtt_handler | Services | lib/services/ | ✅ Yes | ❌ No |
| ota_manager | Services | lib/services/ | ✅ Yes | ❌ No |
| eeprom_config | Services | lib/services/ | ✅ Yes | ❌ No |
| device_id | Services | lib/services/ | ✅ Yes | ❌ No |
| button_manager | Drivers | lib/drivers/ | ✅ Yes | ✅ GPIO |
| led_manager | Drivers | lib/drivers/ | ✅ Yes | ✅ GPIO |
| relay_controller | Drivers | apps/recirculator/src/drivers/ | ❌ No | ✅ GPIO |
| temperature_sensor | Drivers | apps/recirculator/src/drivers/ | ❌ No | ✅ 1-Wire |
| displayManager | Drivers | apps/recirculator/src/drivers/ | ❌ No | ✅ I2C |
| Log | Utils | lib/utils/ | ✅ Yes | ❌ No |
| UtcClock | Utils | lib/utils/ | ✅ Yes | ❌ No |
| config.h | Config | include/ | ✅ Yes | N/A |
| secrets.h | Config | include/ | ✅ Yes | N/A |

---

## 6. MQTT Topics (Recirculator App)

**Pattern**: `mica/dev/{direction}/{deviceType}/{deviceId}/{subject}`

### Commands (Subscribe)
- `mica/dev/command/recirculator/{deviceId}/power-state` - Payload: `"ON"` | `"OFF"`
- `mica/dev/command/recirculator/{deviceId}/max-temperature` - Payload: `35.0` (float °C)
- `mica/dev/command/recirculator/{deviceId}/max-time` - Payload: `120` (int seconds)
- `mica/dev/command/recirculator/{deviceId}/ota` - Payload: `{"url": "...", "version": "..."}`

### Telemetry (Publish)
- `mica/dev/telemetry/recirculator/{deviceId}/temperature` - Every 5s
- `mica/dev/telemetry/recirculator/{deviceId}/power-state` - On change (retained)
- `mica/dev/telemetry/recirculator/{deviceId}/relay-timer` - Every 5s while ON

### Status (Publish)
- `mica/dev/status/recirculator/{deviceId}/healthcheck` - Every 60s

---

## 7. Event Flow

```
Input → Module → Event → System State → Handler → Action
```

**Example**: Button → `SHORT_PRESS` → System State → `activateRelay()` → GPIO ON + MQTT publish

---

## 8. Concurrency

### FreeRTOS Tasks
- System State (pri 3), Display (pri 3)
- WiFi (pri 2), MQTT (pri 2), Temperature (pri 2)
- Relay (pri 1), Button (pri 1), LED (pri 1)

### Thread Safety
- Mutexes: state, temperature, EEPROM
- Event bus via task notifications

---

## 9. Configuration

**Static** (`include/config.h`): GPIO pins, hardware variants, ESP32 defines  
**Secrets** (`include/secrets.h`): AWS IoT credentials, WiFi defaults (gitignored)  
**Dynamic** (EEPROM): WiFi credentials, device-specific configs  
**Remote** (MQTT): Real-time config updates

**Recirculator defaults**: Max temp 30°C, Max time 120s, Device ID = MAC

---

## 10. Error Handling

- WiFi/MQTT: Auto-reconnect with backoff
- Sensor -127°C: Continue, publish error
- Fail-safe: Relay OFF on boot

---

## 11. PlatformIO Configuration

Each app has its own `platformio.ini`:

**Example**: `apps/recirculator/platformio.ini`
```ini
[env:esp32_c3_recirculator]
platform = espressif32
board = seeed_xiao_esp32c3
framework = arduino

# PlatformIO automatically finds libraries in:
# - lib/ (workspace root - shared libraries)
# - apps/recirculator/lib/ (app-local libraries, if any)

# Include global headers
build_flags = 
    -I../../include

lib_deps = 
    adafruit/Adafruit NeoPixel
    knolleary/PubSubClient
    bblanchon/ArduinoJson
    # ... external dependencies
```

**Key Points**:
- No need for `lib_extra_dirs` - PlatformIO finds `lib/` automatically
- `build_flags` includes `include/` for global configs
- Each app compiles independently
- All apps share code from `lib/`

---

## 12. Adding New Application

To add a new app (e.g., `gateway`):

1. **Create app folder**:
   ```bash
   mkdir -p apps/gateway/src
   ```

2. **Create platformio.ini**:
   ```bash
   cp apps/recirculator/platformio.ini apps/gateway/platformio.ini
   # Edit for gateway-specific board/settings
   ```

3. **Create main.cpp**:
   ```cpp
   // apps/gateway/src/main.cpp
   #include "system_state.h"  // From apps/gateway/src/ (gateway's own)
   #include "wifi_connect.h"  // From lib/services/ (shared)
   #include "lora_manager.h"  // From apps/gateway/src/ (gateway-specific)
   // ... automatically uses shared services
   
   void setup() {
       initializeSystemState();  // Gateway's system_state
       // Gateway-specific initialization
   }
   ```

4. **Create gateway's system_state**:
   ```bash
   # Copy and adapt from recirculator as template
   cp apps/recirculator/src/system_state.* apps/gateway/src/
   # Edit to coordinate gateway-specific modules (LoRa, etc.)
   ```

4. **Add device-specific drivers**:
   ```bash
   # apps/gateway/src/lora_manager.cpp/h
   # Gateway-specific hardware
   ```

5. **Compile**:
   ```bash
   cd apps/gateway
   platformio run
   ```

All shared services (WiFi, MQTT, OTA, etc.) are automatically available!

---

**Architecture**: gaesca04 (Computer Engineer)  
**Type**: PlatformIO Monorepo  
**Version**: 3.0 | 28 Nov 2025
