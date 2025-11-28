# MICA Recirculator - System Architecture

> **Architecture by**: gaesca04 (Computer Engineer)  
> **Last Updated**: 28 November 2025

## 1. System Overview

ESP32-C3 water recirculation controller with temperature monitoring, relay control, and AWS IoT MQTT telemetry.

**Platform**: ESP32-C3, FreeRTOS, Arduino Framework

Part of MICA IoT ecosystem. Core modules designed for reuse across multiple devices (Gateway, sensors, etc.).

## 2. Layered Architecture

The system follows a **3-layer architecture pattern** for clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────┐
│                   APPLICATION LAYER                         │
│  (Business Logic, State Coordination, Entry Point)          │
│                                                              │
│  - system_state  (Event Coordinator)                        │
│  - main          (Entry Point)                              │
└──────────────────────────┬──────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│                    SERVICES LAYER                           │
│  (Business Services, No Direct Hardware Access)             │
│                                                              │
│  - wifi_connect      (WiFi management)                      │
│  - wifi_config_mode  (AP + Captive Portal)                  │
│  - mqtt_handler      (AWS IoT communication)                │
│  - ota_manager       (Firmware updates)                     │
│  - eeprom_config     (Persistent storage)                   │
│  - device_id         (Unique device identifier)             │
└──────────────────────────┬──────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│                    DRIVERS LAYER                            │
│  (Hardware Abstraction, GPIO/I2C/1-Wire Interaction)        │
│                                                              │
│  - button_manager      (GPIO input - buttons)               │
│  - led_manager         (WS2812B NeoPixel control)           │
│  - relay_controller    (GPIO output - relay control)        │
│  - temperature_sensor  (DS18B20 - 1-Wire)                   │
│  - displayManager      (SSD1306 OLED - I2C)                 │
└─────────────────────────────────────────────────────────────┘
```

**Architecture Benefits**:
- **Clear Separation**: Each layer has specific responsibilities
- **Testability**: Services can be tested without hardware
- **Portability**: Drivers isolate hardware dependencies
- **Maintainability**: Changes in one layer don't affect others
- **Reusability**: Services layer is hardware-agnostic

## 3. Modules by Layer

### 3.1 Application Layer

**Purpose**: Business logic orchestration and system entry point

#### system_state
- **Role**: Event coordinator and state machine
- **State Flow**: CONNECTING → WIFI → MQTT → OPERATIONAL
- **Interface**: `notifySystemState(event)`, `initializeSystemState()`
- **Responsibility**: Coordinates all modules, manages task lifecycle
- **Layer**: Application (Core - Shared across devices)

#### main
- **Role**: Entry point and system initialization
- **Interface**: `setup()`, `loop()`
- **Responsibility**: Bootstraps system_state
- **Layer**: Application (Device-specific)

---

### 3.2 Services Layer

**Purpose**: Business services without direct hardware interaction

#### wifi_connect
- **Role**: WiFi connection management
- **Features**: Auto-connect, exponential backoff, EEPROM credentials
- **Interface**: `initWiFi()`, `isWiFiConnected()`
- **Layer**: Services (Core - Shared)

#### wifi_config_mode
- **Role**: Configuration portal via Access Point
- **Features**: AP mode + captive portal, credential storage
- **Trigger**: Long button press (5s)
- **Interface**: `startConfigMode()`, `isInConfigMode()`
- **Layer**: Services (Core - Shared)

#### mqtt_handler
- **Role**: AWS IoT Core communication
- **Architecture**: Generic with deviceType parameter, FreeRTOS queues
- **Topics**: `mica/dev/{command|telemetry}/recirculator/{deviceId}/{subject}`
- **Interface**: `mqttPublish()`, `mqttSubscribe()`, `isMqttConnected()`
- **Layer**: Services (Core - Shared)

#### ota_manager
- **Role**: Over-The-Air firmware updates
- **Triggers**: MQTT command or HTTP
- **Interface**: `initOTA()`, `handleOTA()`
- **Layer**: Services (Core - Shared)

#### eeprom_config
- **Role**: Persistent configuration storage
- **Stores**: WiFi credentials, safety limits (max temp, max time)
- **Never Stores**: Runtime state (relay ON/OFF - safety by design)
- **Interface**: `saveMaxTemperature()`, `loadMaxTime()`, etc.
- **Layer**: Services (Core - Shared)

#### device_id
- **Role**: Unique device identifier generation
- **Source**: ESP32 MAC address
- **Interface**: `getDeviceId()`, `initDeviceId()`
- **Layer**: Services (Core - Shared)

---

### 3.3 Drivers Layer

**Purpose**: Hardware abstraction and GPIO/peripheral interaction

#### button_manager
- **Role**: Button input handler (GPIO interrupt-based)
- **Hardware**: GPIO buttons with debouncing
- **Events**: `SHORT_PRESS`, `LONG_PRESS` (5s threshold)
- **Interface**: `initButtonManager()` - Publishes events to system_state
- **Note**: Generic, no device-specific logic
- **Layer**: Drivers (Core - Shared)

#### led_manager
- **Role**: Visual status indicator
- **Hardware**: WS2812B NeoPixel (addressable RGB LED)
- **States**: Colors/patterns for WiFi, MQTT, errors, config mode
- **Interface**: `setLEDColor()`, `setLEDPattern()`
- **Layer**: Drivers (Core - Shared)

#### relay_controller
- **Role**: Relay control with safety monitoring
- **Hardware**: GPIO output to relay module
- **Features**: ON/OFF control, timeout timer, temperature threshold
- **Safety**: Monitoring loop (1s), auto-shutoff on limits
- **Interface**: `activateRelay()`, `deactivateRelay()`, `isRelayActive()`
- **Layer**: Drivers (Device-specific - Recirculator)

#### temperature_sensor
- **Role**: Temperature measurement and publishing
- **Hardware**: DS18B20 digital sensor (1-Wire protocol)
- **Polling**: Every 5 seconds
- **Error Handling**: -127°C indicates sensor error, publishes error to MQTT
- **Interface**: `initTemperatureSensor()`, `getLatestTemperature()`
- **Layer**: Drivers (Device-specific - Recirculator)

#### displayManager
- **Role**: Local visual feedback display
- **Hardware**: SSD1306 OLED 128x64 pixels (I2C)
- **Content**: Temperature, relay state, timers, system status
- **Interface**: `initDisplay()`, `updateDisplay()`
- **Layer**: Drivers (Device-specific - Recirculator)

---

### 3.4 Module Classification Summary

| Module | Layer | Shared/Device | Hardware Access |
|--------|-------|---------------|-----------------|
| system_state | Application | Shared (Core) | No |
| main | Application | Device-specific | No |
| wifi_connect | Services | Shared (Core) | No (uses ESP32 WiFi API) |
| wifi_config_mode | Services | Shared (Core) | No (uses ESP32 WiFi API) |
| mqtt_handler | Services | Shared (Core) | No (uses WiFiClientSecure) |
| ota_manager | Services | Shared (Core) | No (uses HTTPUpdate) |
| eeprom_config | Services | Shared (Core) | No (uses Preferences API) |
| device_id | Services | Shared (Core) | No (reads MAC from ESP32) |
| button_manager | Drivers | Shared (Core) | Yes (GPIO input) |
| led_manager | Drivers | Shared (Core) | Yes (GPIO output - WS2812B) |
| relay_controller | Drivers | Device-specific | Yes (GPIO output) |
| temperature_sensor | Drivers | Device-specific | Yes (1-Wire GPIO) |
| displayManager | Drivers | Device-specific | Yes (I2C) |

## 4. MQTT Topics

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

## 5. Event Flow

```
Input → Module → Event → System State → Handler → Action
```

**Example**: Button → `SHORT_PRESS` → System State → `activateRelay()` → GPIO ON + MQTT publish

## 6. Concurrency

### FreeRTOS Tasks
- System State (pri 3), Display (pri 3)
- WiFi (pri 2), MQTT (pri 2), Temperature (pri 2)
- Relay (pri 1), Button (pri 1), LED (pri 1)

### Thread Safety
- Mutexes: state, temperature, EEPROM
- Event bus via task notifications

## 7. Configuration

**Static** (config.h): GPIO pins, hardware variants  
**Dynamic** (EEPROM): WiFi credentials, max temp, max time  
**Remote** (MQTT): Real-time config updates

**Defaults**: Max temp 30°C, Max time 120s, Device ID = MAC

## 8. Error Handling

- WiFi/MQTT: Auto-reconnect with backoff
- Sensor -127°C: Continue, publish error
- Fail-safe: Relay OFF on boot

---

**Architecture**: gaesca04 (Computer Engineer)  
**Version**: 2.0 | 28 Nov 2025
