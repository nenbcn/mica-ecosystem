# MICA Ecosystem - System Architecture

> **Architecture by**: gaesca04 (Computer Engineer)  
> **Last Updated**: 28 November 2025

IoT monorepo for ESP32 devices with FreeRTOS and Arduino Framework.

**Apps**: Recirculator (production), Gateway (future)  
**Platform**: PlatformIO monorepo, shared services and drivers

---

## 1. Layered Architecture (4 Layers)

The system follows a **4-layer architecture pattern** for clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────────────┐
│                     APPLICATION LAYER                               │
│  Location: apps/*/src/ (app-specific)                               │
│                                                                      │
│  - system_state  (Event Coordinator - per app)                      │
│  - main.cpp      (Entry Point - per app)                            │
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

**Global Configuration**: `include/` (config.h, secrets.h)

---

## 2. Modules by Layer

### Application Layer (Per-App)
**Location**: `apps/*/src/`

| Module | Purpose |
|--------|---------|
| **system_state** | Event coordinator, state machine (CONNECTING → WIFI → MQTT → OPERATIONAL) |
| **main.cpp** | Entry point, initialization |

Each app has its own `system_state` coordinating device-specific modules.

### Services Layer (Shared)
**Location**: `lib/`

| Module | Purpose | Interface |
|--------|---------|-----------|
| **wifi_connect** | WiFi management, auto-reconnect | `initWiFi()`, `isWiFiConnected()` |
| **wifi_config_mode** | AP mode + captive portal | `startConfigMode()`, `isInConfigMode()` |
| **mqtt_handler** | AWS IoT MQTT (generic) | `mqttPublish()`, `mqttSubscribe()` |
| **ota_manager** | Firmware updates | `initOTA()`, `handleOTA()` |
| **eeprom_config** | Persistent storage | `saveConfig()`, `loadConfig()` |
| **device_id** | Unique ID from MAC | `getDeviceId()` |
| **button_manager** | GPIO button (debounce, SHORT/LONG press) | `initButtonManager()` |
| **led_manager** | WS2812B status LED | `setLEDColor()`, `setLEDPattern()` |
| **Log** | Logging system | `Log::info()`, `Log::error()` |
| **UtcClock** | NTP time sync | `getUtcTime()`, `syncNTP()` |

### Drivers Layer (App-Specific)
**Location**: `apps/recirculator/src/drivers/`

| Module | Hardware | Purpose |
|--------|----------|---------|
| **relay_controller** | GPIO relay | ON/OFF control, safety timeouts |
| **temperature_sensor** | DS18B20 (1-Wire) | Temperature monitoring, MQTT publish |
| **displayManager** | SSD1306 OLED (I2C) | Local display, status feedback |

---

## 4. MQTT Topics (Recirculator)

Pattern: `mica/dev/{command|telemetry|status}/{deviceType}/{deviceId}/{subject}`

**Subscribe (Commands)**:
- `power-state` - `"ON"` | `"OFF"`
- `max-temperature` - `35.0` (float °C)
- `max-time` - `120` (int seconds)

**Publish (Telemetry)**:
- `temperature` - Every 5s
- `power-state` - On change (retained)
- `relay-timer` - Every 5s while ON

---

## 5. FreeRTOS Concurrency

**Tasks**: System State (pri 3), WiFi/MQTT (pri 2), Relay/Sensors (pri 1)  
**Thread Safety**: Mutexes for state, temperature, EEPROM  
**Events**: Task notifications via `system_state`

---

## 6. Configuration

| Type | Location | Content |
|------|----------|---------|
| **Static** | `include/config.h` | GPIO pins, hardware defines |
| **Secrets** | `include/secrets.h` (gitignored) | AWS IoT certs, WiFi defaults |
| **Dynamic** | EEPROM | WiFi credentials, device configs |
| **Remote** | MQTT | Real-time updates |

**Safety**: Relay state never persisted to EEPROM (always OFF on boot)

---

## 7. PlatformIO Configuration

Each app has its own `platformio.ini`:

```ini
[env:esp32_c3_recirculator]
platform = espressif32
board = seeed_xiao_esp32c3
framework = arduino
build_flags = 
    -I../../include  # Global configs
    -Ilib/services/wifi_connect  # Shared libraries
    -Ilib/drivers/button_manager
    # etc.
lib_extra_dirs = 
    lib/services
    lib/drivers
    lib/utils
lib_deps = 
    adafruit/Adafruit NeoPixel
    knolleary/PubSubClient
    # External dependencies
```

**PlatformIO discovers** `lib/` subdirectories via `lib_extra_dirs`.

---

## 8. Adding New App

```bash
mkdir -p apps/my_device/src
cp apps/recirculator/platformio.ini apps/my_device/
# Edit board and settings
```

```cpp
// apps/my_device/src/main.cpp
#include "wifi_connect.h"  // From lib/services/wifi_connect/
#include "mqtt_handler.h"  // From lib/services/mqtt_handler/
#include "button_manager.h"  // From lib/drivers/button_manager/
#include "Log.h"  // From lib/utils/Log/

void setup() {
    initWiFi();
    initializeMQTTHandler("my_device", getDeviceId());
    // Device-specific code...
}
```

All shared libraries automatically available via `lib_extra_dirs`!

---

**Architecture**: gaesca04 (Computer Engineer) | **Version**: 3.0 | 28 Nov 2025
