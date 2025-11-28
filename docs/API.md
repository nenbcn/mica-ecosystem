# Shared Modules - MICA Ecosystem

> **Architecture by**: gaesca04 (computer engineer)

Comprehensive reference for all shared modules in `lib/`. These modules are designed to be device-agnostic and reusable across all MICA applications.

---

## 📚 Module Index

### Application Layer
- [system_state](#systemstate) - Event coordinator and state machine (future)

### Services Layer
- [wifi_connect](#wificonnect) - WiFi connection management
- [wifi_config_mode](#wificonfigmode) - AP mode + captive portal configuration
- [mqtt_handler](#mqtthandler) - AWS IoT MQTT communication (generic)
- [ota_manager](#otamanager) - Over-The-Air firmware updates
- [eeprom_config](#eepromconfig) - Persistent key-value storage
- [device_id](#deviceid) - Unique device identifier from MAC address

### Drivers Layer
- [button_manager](#buttonmanager) - GPIO button input with debounce
- [led_manager](#ledmanager) - WS2812B RGB LED status indicator

### Utils Layer
- [Log](#log) - Logging system with severity levels
- [UtcClock](#utcclock) - Time management and NTP synchronization

---

## Application Layer

### system_state
**Location**: `lib/application/system_state/` (future)  
**Purpose**: Centralized event coordination and state machine for system lifecycle  
**Status**: Currently app-specific in `apps/recirculator/src/`, will be generalized

**Key Functions** (future generic API):
```cpp
void initializeSystemState();
void notifySystemState(TaskNotificationEvent event);
SystemState getCurrentState();
```

**Used By**: All apps (future)  
**Dependencies**: All other modules

---

## Services Layer

### wifi_connect
**Location**: `lib/wifi_connect/`  
**Purpose**: Manages WiFi connection with auto-reconnect and exponential backoff

**Public API**:
```cpp
void initializeWiFiConnection();
void wifiConnectTask(void* pvParameters);  // FreeRTOS task
bool isWifiConnected();
```

**Features**:
- Auto-reconnect with exponential backoff (1s → 2s → 5s → 10s → 30s)
- Stores credentials in EEPROM via `wifi_config_mode`
- Notifies `system_state` on connect/disconnect
- Non-blocking (FreeRTOS task)

**Used By**: Recirculator, Gateway (future)  
**Dependencies**: `eeprom_config`, `system_state`

**Example**:
```cpp
// In main.cpp
initializeWiFiConnection();
xTaskCreate(wifiConnectTask, "WiFiConnect", 4096, NULL, 1, &wifiTaskHandle);
```

---

### wifi_config_mode
**Location**: `lib/wifi_config_mode/`  
**Purpose**: AP mode + captive portal for WiFi configuration

**Public API**:
```cpp
void wifiConfigModeTask(void* pvParameters);  // FreeRTOS task
bool isInConfigMode();
```

**Features**:
- Creates AP: "MICA-{DeviceType}" (e.g., "MICA-Recirculator")
- Captive portal at 192.168.4.1
- Web form for SSID/password input
- Saves credentials to EEPROM
- Auto-exits to normal mode after successful save

**Activation**:
- Long press button (5 seconds)
- Or automatic if no WiFi credentials stored

**Used By**: Recirculator, Gateway (future)  
**Dependencies**: `device_id`, `eeprom_config`, `button_manager`

**Topics**: N/A (local configuration only)

---

### mqtt_handler
**Location**: `lib/mqtt_handler/`  
**Purpose**: Generic AWS IoT MQTT communication with queue architecture

**Public API**:
```cpp
// Initialization
void initializeMQTTHandler(const char* deviceType, const char* deviceId);

// Publishing (thread-safe via queue)
bool mqttPublish(const char* topic, const char* payload, bool retain = false);

// Subscribing (callback system)
typedef void (*MqttMessageHandler)(const char* topic, const char* payload, unsigned int length);
bool mqttSubscribe(const char* topic, MqttMessageHandler handler);

// Status
bool isMqttConnected();

// FreeRTOS tasks
void mqttConnectTask(void* pvParameters);
void mqttPublishTask(void* pvParameters);
```

**Features**:
- **Device-agnostic**: `deviceType` parameter ("recirculator", "gateway", etc.)
- **Thread-safe**: FreeRTOS queue for publishing (20 messages buffer)
- **Callback system**: Multiple modules can subscribe to topics
- **Auto-reconnect**: Handles MQTT disconnections
- **TLS/SSL**: Secure connection to AWS IoT Core

**Queue Architecture**:
```cpp
// Modules call:
mqttPublish("mica/dev/telemetry/recirculator/ABC123/temperature", "{\"temp\":25.5}", true);
// → xQueueSend() → mqttPublishTask() consumes → mqttClient.publish()
```

**Topic Convention**:
- Telemetry: `mica/dev/telemetry/{deviceType}/{deviceId}/{metric}`
- Commands: `mica/dev/command/{deviceType}/{deviceId}/{action}`
- Status: `mica/dev/status/{deviceType}/{deviceId}/{component}`

**Used By**: Recirculator, Gateway (future)  
**Dependencies**: `device_id`, `system_state`, WiFiClientSecure, PubSubClient

**Example Usage**:
```cpp
// In relay_controller.cpp
void initializeRelayController() {
    mqttSubscribe("mica/dev/command/recirculator/ABC123/power-state", handlePowerCommand);
}

void publishRelayState() {
    DynamicJsonDocument doc(128);
    doc["deviceId"] = getDeviceId();
    doc["state"] = isRelayActive ? "ON" : "OFF";
    String json;
    serializeJson(doc, json);
    mqttPublish("mica/dev/telemetry/recirculator/ABC123/power-state", json.c_str(), true);
}
```

---

### ota_manager
**Location**: `lib/ota_manager/`  
**Purpose**: Over-The-Air firmware updates from AWS S3/API

**Public API**:
```cpp
void initializeOTAManager();
void otaTask(void* pvParameters);  // FreeRTOS task
bool isOTAInProgress();
```

**Features**:
- Downloads firmware from configured URL
- Verifies integrity (SHA256 hash)
- Atomic updates (no partial updates)
- Auto-reboot after successful update
- Rollback on failure

**Configuration**:
- OTA URL stored in EEPROM
- Can be updated via MQTT command

**Used By**: Recirculator, Gateway (future)  
**Dependencies**: HTTPClient, Update library

**Safety**:
- ⚠️ Relay always OFF during OTA
- ⚠️ No user interaction during update

---

### eeprom_config
**Location**: `lib/eeprom_config/`  
**Purpose**: Persistent key-value storage for configuration

**Public API**:
```cpp
// Initialization
void eepromInitialize();

// WiFi credentials
bool loadWiFiCredentials(char* ssid, char* password);
void saveWiFiCredentials(const char* ssid, const char* password);

// Device-specific config
float getStoredMaxTemperature();
void saveMaxTemperature(float temp);
uint32_t getStoredMaxTime();
void saveMaxTime(uint32_t seconds);

// Generic key-value
String readString(uint16_t address, uint16_t maxLength);
void writeString(uint16_t address, const String& value);
```

**Storage Layout**:
```
Address | Size | Content
--------|------|------------------
0-31    | 32   | WiFi SSID
32-95   | 64   | WiFi Password
100-103 | 4    | Max Temperature (float)
200-203 | 4    | Max Time (uint32_t)
```

**Features**:
- Non-volatile (survives power loss)
- Wear leveling for frequent writes
- Default values on first boot

**Used By**: All apps  
**Dependencies**: EEPROM library (ESP32)

---

### device_id
**Location**: `lib/device_id/`  
**Purpose**: Generate unique device identifier from MAC address

**Public API**:
```cpp
const char* getDeviceId();  // Returns "AABBCC" (6 hex chars)
```

**Features**:
- Based on ESP32 WiFi MAC address
- Short format: last 6 hex digits (e.g., "A1B2C3")
- Consistent across reboots
- Used in MQTT topics and telemetry

**Used By**: All apps  
**Dependencies**: WiFi library

**Example**:
```cpp
const char* deviceId = getDeviceId();
// deviceId = "A1B2C3"

// Use in topic
snprintf(topic, sizeof(topic), "mica/dev/telemetry/recirculator/%s/temperature", deviceId);
// topic = "mica/dev/telemetry/recirculator/A1B2C3/temperature"
```

---

## Drivers Layer

### button_manager
**Location**: `lib/button_manager/`  
**Purpose**: Generic GPIO button input with debounce and event detection

**Public API**:
```cpp
void initializeButtonManager();
void buttonTask(void* pvParameters);  // FreeRTOS task

// Event types (sent to system_state)
typedef enum {
    BUTTON_EVENT_SHORT_PRESS,
    BUTTON_EVENT_LONG_PRESS,
    BUTTON_EVENT_VERY_LONG_PRESS
} ButtonEvent;
```

**Features**:
- Debouncing (50ms)
- Short press detection (<5s)
- Long press detection (≥5s) → Config mode
- Very long press detection (≥10s) → Factory reset
- Non-blocking (FreeRTOS task)

**Configuration** (in `config.h`):
```cpp
#define BUTTON_PIN 9  // GPIO pin
```

**Used By**: All apps  
**Dependencies**: `system_state`

**Event Flow**:
```
Button Press → Debounce → Detect Duration → Notify system_state → App handles event
```

---

### led_manager
**Location**: `lib/led_manager/`  
**Purpose**: WS2812B RGB LED status indicator

**Public API**:
```cpp
void initializeLedManager();
void ledTask(void* pvParameters);  // FreeRTOS task
void setLedColor(uint8_t r, uint8_t g, uint8_t b);
void setLedBlink(uint32_t intervalMs);
void setLedPattern(LedPattern pattern);
```

**LED Patterns**:
```cpp
typedef enum {
    LED_PATTERN_OFF,              // LED off
    LED_PATTERN_SOLID,            // Solid color
    LED_PATTERN_BLINK_SLOW,       // 1 Hz blink
    LED_PATTERN_BLINK_FAST,       // 5 Hz blink
    LED_PATTERN_BREATHING,        // Fade in/out
    LED_PATTERN_RAINBOW           // Cycling colors
} LedPattern;
```

**Status Colors** (convention):
- 🔴 Red: Error, disconnected
- 🟢 Green: Connected, OK
- 🔵 Blue: Config mode
- 🟡 Yellow: Warning
- 🟣 Purple: OTA update
- ⚪ White: Initializing

**Configuration** (in `config.h`):
```cpp
#define NEOPIXEL_PIN 5
#define NEOPIXEL_COUNT 1
```

**Used By**: All apps  
**Dependencies**: Adafruit NeoPixel library

---

## Utils Layer

### Log
**Location**: `lib/Log/`  
**Purpose**: Logging system with severity levels and Serial output

**Public API**:
```cpp
void Log::init();
void Log::process(Print* printer);

// Logging functions (variadic, printf-style)
void Log::error(const char* format, ...);
void Log::warn(const char* format, ...);
void Log::info(const char* format, ...);
void Log::debug(const char* format, ...);
void Log::verbose(const char* format, ...);
```

**Features**:
- Multiple severity levels: ERROR, WARN, INFO, DEBUG, VERBOSE
- printf-style formatting
- Timestamp support (if UtcClock available)
- Color-coded output (ANSI colors)
- Compile-time filtering (via `CORE_DEBUG_LEVEL`)

**Usage**:
```cpp
// In main.cpp
Log::init();

// In any module
Log::info("WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
Log::error("Temperature sensor failed: %.2f°C", temperature);
Log::debug("Relay timer: %lu/%lu seconds", elapsedSeconds, maxTimeSeconds);
```

**Output Format**:
```
[12:34:56] [INFO] WiFi connected, IP: 192.168.1.100
[12:34:57] [ERROR] Temperature sensor failed: -127.00°C
```

**Used By**: All apps  
**Dependencies**: Arduino Print class

---

### UtcClock
**Location**: `lib/UtcClock/`  
**Purpose**: Time management and NTP synchronization

**Public API**:
```cpp
void initUtcClock();
unsigned long getUtcTimestamp();
String getFormattedTime();
bool isTimeSynchronized();
```

**Features**:
- NTP synchronization (pool.ntp.org)
- Timezone support
- Automatic resync (every 12 hours)
- Epoch timestamp generation

**Used By**: Log, telemetry timestamps  
**Dependencies**: WiFi, time.h

---

## 🔧 Module Dependency Graph

```
┌──────────────────────────────────────────────────┐
│              Application Layer                   │
│  ┌───────────────┐                               │
│  │ system_state  │  (future generic)             │
│  └───────┬───────┘                               │
│          │                                        │
├──────────┼────────────────────────────────────────┤
│          │       Services Layer                   │
│  ┌───────▼────┐  ┌─────────────┐  ┌──────────┐  │
│  │wifi_connect│  │mqtt_handler │  │ota_manager│ │
│  └─────┬──────┘  └──────┬──────┘  └────┬─────┘  │
│        │                │                │        │
│  ┌─────▼──────────┐  ┌──▼──────┐  ┌────▼────┐   │
│  │wifi_config_mode│  │device_id│  │eeprom   │   │
│  └────────────────┘  └─────────┘  │_config  │   │
│                                     └─────────┘   │
├──────────────────────────────────────────────────┤
│              Drivers Layer                       │
│  ┌──────────────┐  ┌──────────┐                 │
│  │button_manager│  │led_manager│                │
│  └──────────────┘  └──────────┘                 │
├──────────────────────────────────────────────────┤
│              Utils Layer                         │
│  ┌────┐  ┌─────────┐                            │
│  │Log │  │UtcClock │                            │
│  └────┘  └─────────┘                            │
└──────────────────────────────────────────────────┘
```

---

## 📝 Adding a New Shared Module

### 1. Choose the Correct Layer
- **Services**: No hardware access (WiFi, cloud, storage)
- **Drivers**: Hardware abstraction (GPIO, I2C, sensors)
- **Utils**: Helper functions (logging, time, math)

### 2. Create Module Directory
```bash
mkdir -p lib/<layer>/<module_name>
```

### 3. Add Files
```
lib/<layer>/<module_name>/
├── <module_name>.cpp
└── <module_name>.h
```

### 4. Follow API Guidelines
- Clear, documented public API
- FreeRTOS tasks for blocking operations
- Use `system_state` for event coordination
- Generic (no hardcoded device types)

### 5. Update Documentation
- Add to this file (`shared-modules.md`)
- Update dependency graph
- Add usage examples

---

## 🎯 Best Practices

### Thread Safety
- Use FreeRTOS queues for inter-task communication
- Mutexes for shared resources (EEPROM, displays)
- Avoid global mutable state

### Resource Management
- Free allocated memory
- Close connections properly
- Use RAII patterns where possible

### Error Handling
- Always check return values
- Log errors with context
- Fail gracefully (don't crash)

### Testing
- Unit test pure logic functions
- Integration test with real hardware
- Verify MQTT topics in AWS console

---

**Last Updated**: 28 November 2025  
**Architecture**: gaesca04 (computer engineer)
