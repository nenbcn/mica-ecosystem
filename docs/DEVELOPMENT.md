# Monorepo Development Guide - MICA Ecosystem

> **Architecture by**: gaesca04 (computer engineer, monorepo expert)

Complete guide for developers working in the MICA monorepo. Learn how to add new apps, modify shared modules, and maintain code quality across the ecosystem.

---

## 🎯 Core Concepts

### What is a Monorepo?
A single Git repository containing **multiple projects** that share code and tooling.

**Advantages**:
- ✅ **Code sharing**: Write once, use everywhere
- ✅ **Atomic changes**: Update shared code and all apps in one commit
- ✅ **Consistent tooling**: Same PlatformIO config, same standards
- ✅ **Easy refactoring**: Change APIs everywhere at once
- ✅ **Single source of truth**: No version conflicts

**Challenges**:
- ⚠️ **Breaking changes**: Affect all apps simultaneously
- ⚠️ **Testing**: Must test all apps when changing shared code
- ⚠️ **Discipline**: Requires careful API design

---

## 📁 Repository Structure

```
mica-ecosystem/
│
├── apps/                    # Independent applications
│   ├── recirculator/        # Water recirculation control
│   │   ├── platformio.ini   # PlatformIO config for this app
│   │   └── src/
│   │       ├── main.cpp              # Entry point
│   │       ├── system_state.cpp/h    # App state machine
│   │       └── drivers/              # Device-specific drivers
│   │           ├── relay_controller.cpp/h
│   │           ├── temperature_sensor.cpp/h
│   │           └── displayManager.cpp/h
│   │
│   └── gateway/             # Future: LoRa gateway (not implemented yet)
│       └── ...
│
├── lib/                     # Shared libraries (PlatformIO auto-finds)
│   ├── button_manager/      # GPIO button with debounce
│   ├── device_id/           # Unique device identifier
│   ├── eeprom_config/       # Persistent storage
│   ├── led_manager/         # WS2812B RGB LED
│   ├── Log/                 # Logging system
│   ├── mqtt_handler/        # Generic MQTT client
│   ├── ota_manager/         # OTA updates
│   ├── UtcClock/            # Time management
│   ├── wifi_config_mode/    # AP + captive portal
│   └── wifi_connect/        # WiFi connection
│
├── include/                 # Global configuration
│   ├── config.h             # Hardware pins, constants
│   └── secrets.h            # AWS credentials (gitignored)
│
├── docs/                    # Documentation
│   ├── shared-modules.md    # API reference for lib/
│   ├── monorepo-guide.md    # This file
│   ├── architecture.md      # System architecture
│   └── ...
│
├── platformio.ini           # Root config (points to apps)
└── README.md                # Project overview
```

---

## 🚀 Common Tasks

### 1. Adding a New Application

**Example**: Adding `apps/gateway/`

#### Step 1: Create Directory Structure
```bash
mkdir -p apps/gateway/src
mkdir -p apps/gateway/src/drivers
```

#### Step 2: Create `platformio.ini`
```ini
; apps/gateway/platformio.ini
[env:esp32_gateway]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

; Point to shared libraries
build_flags = 
    -Iinclude                 ; Global config
    -Isrc                     ; App source

; External dependencies
lib_deps = 
    knolleary/PubSubClient@^2.8
    ; Add more as needed
```

#### Step 3: Create `main.cpp`
```cpp
// apps/gateway/src/main.cpp
#include <Arduino.h>
#include "config.h"           // From include/
#include "wifi_connect.h"     // From lib/
#include "mqtt_handler.h"     // From lib/
#include "Log.h"              // From lib/

void setup() {
    Serial.begin(115200);
    Log::init();
    
    Log::info("=== MICA Gateway ===");
    
    // Initialize shared modules
    initializeWiFiConnection();
    initializeMQTTHandler("gateway", getDeviceId());
    
    // Gateway-specific initialization
    // ...
}

void loop() {
    delay(1000);
}
```

#### Step 4: Add Device-Specific Drivers
```bash
# Example: LoRa driver for gateway
touch apps/gateway/src/drivers/lora_driver.cpp
touch apps/gateway/src/drivers/lora_driver.h
```

#### Step 5: Update Root `platformio.ini`
```ini
[platformio]
default_envs = esp32_c3_recirculator  ; Change to gateway to build it

src_dir = apps/gateway/src           ; Point to gateway source
```

#### Step 6: Compile and Test
```bash
cd mica-ecosystem
platformio run
```

---

### 2. Modifying a Shared Module

**Example**: Adding a function to `mqtt_handler`

#### Step 1: Understand Impact
⚠️ **Any change affects ALL apps using this module**

Check which apps use it:
```bash
grep -r "mqtt_handler.h" apps/
# Output: apps/recirculator/src/main.cpp
# Output: apps/gateway/src/main.cpp (future)
```

#### Step 2: Update Header (`mqtt_handler.h`)
```cpp
// lib/mqtt_handler/mqtt_handler.h

// Add new function
bool mqttPublishBatch(const char* baseTopic, JsonDocument& doc);
```

#### Step 3: Implement (`mqtt_handler.cpp`)
```cpp
// lib/mqtt_handler/mqtt_handler.cpp

bool mqttPublishBatch(const char* baseTopic, JsonDocument& doc) {
    String json;
    serializeJson(doc, json);
    return mqttPublish(baseTopic, json.c_str(), false);
}
```

#### Step 4: Update Documentation
```markdown
<!-- docs/API.md -->

### mqtt_handler
**Public API**:
- `mqttPublishBatch()` - Publish JSON document to topic
```

#### Step 5: Test ALL Apps
```bash
# Test recirculator
cd apps/recirculator
platformio run

# Test gateway (when implemented)
cd apps/gateway
platformio run
```

#### Step 6: Commit Atomically
```bash
git add lib/mqtt_handler/ docs/shared-modules.md
git commit -m "feat(mqtt): Add mqttPublishBatch for JSON documents

- Add new function to mqtt_handler
- Update shared-modules.md documentation
- Backwards compatible (existing functions unchanged)"
```

---

### 3. Breaking Changes in Shared Modules

**Example**: Changing `initializeMQTTHandler()` signature

#### Step 1: Plan the Change
```cpp
// Old API (breaking)
void initializeMQTTHandler(const char* deviceType, const char* deviceId);

// New API (adds parameter)
void initializeMQTTHandler(const char* deviceType, const char* deviceId, uint16_t port);
```

#### Step 2: Update ALL Apps **in the same commit**
```bash
# Update lib/mqtt_handler/
# Update apps/recirculator/src/main.cpp
# Update apps/gateway/src/main.cpp (future)
# Update docs/API.md
```

#### Step 3: Commit Atomically
```bash
git add lib/mqtt_handler/ apps/ docs/
git commit -m "refactor(mqtt): Add port parameter to initializeMQTTHandler()

BREAKING CHANGE: initializeMQTTHandler() now requires port parameter

- Update mqtt_handler API
- Update recirculator to pass MQTT_PORT
- Update gateway to pass MQTT_PORT
- Update documentation

All apps compile and tested."
```

---

### 4. Adding a New Shared Module

**Example**: Creating `lib/pressure_sensor/`

#### Step 1: Choose the Correct Location
- Services? (No hardware) → `lib/services/`
- Driver? (Hardware access) → `lib/drivers/`
- Utility? (Helper functions) → `lib/utils/`

**Decision**: Pressure sensor = **Driver** → `lib/pressure_sensor/`

#### Step 2: Create Files
```bash
mkdir -p lib/pressure_sensor
touch lib/pressure_sensor/pressure_sensor.cpp
touch lib/pressure_sensor/pressure_sensor.h
```

#### Step 3: Design Generic API
```cpp
// lib/pressure_sensor/pressure_sensor.h
#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include <Arduino.h>

// Generic API (no device-specific logic)
bool initializePressureSensor(uint8_t i2cAddress);
float readPressure();  // Returns pressure in kPa
bool isSensorReady();

#endif
```

#### Step 4: Implement
```cpp
// lib/pressure_sensor/pressure_sensor.cpp
#include "pressure_sensor.h"
#include "Log.h"
#include <Wire.h>

static uint8_t sensorAddress = 0x76;

bool initializePressureSensor(uint8_t i2cAddress) {
    sensorAddress = i2cAddress;
    Wire.begin();
    
    // Test communication
    Wire.beginTransmission(sensorAddress);
    if (Wire.endTransmission() != 0) {
        Log::error("Pressure sensor not found at 0x%02X", sensorAddress);
        return false;
    }
    
    Log::info("Pressure sensor initialized at 0x%02X", sensorAddress);
    return true;
}

float readPressure() {
    // Read from sensor (implementation depends on sensor model)
    // ...
    return 101.3;  // Example: 101.3 kPa
}

bool isSensorReady() {
    Wire.beginTransmission(sensorAddress);
    return Wire.endTransmission() == 0;
}
```

#### Step 5: Document in `API.md`
```markdown
### pressure_sensor
**Location**: `lib/pressure_sensor/`  
**Purpose**: Generic I2C pressure sensor driver

**Public API**:
- `initializePressureSensor(address)` - Initialize sensor
- `readPressure()` - Read pressure in kPa
- `isSensorReady()` - Check sensor status

**Used By**: Recirculator (future), Gateway  
**Dependencies**: Wire (I2C)
```

#### Step 6: Use in App
```cpp
// apps/recirculator/src/main.cpp
#include "pressure_sensor.h"

void setup() {
    // Initialize shared module
    initializePressureSensor(0x76);
}

void loop() {
    float pressure = readPressure();
    Log::info("Pressure: %.2f kPa", pressure);
    delay(1000);
}
```

PlatformIO automatically finds and compiles `lib/pressure_sensor/` - **no configuration needed!**

---

## 🔧 PlatformIO Configuration

### How PlatformIO Finds Shared Libraries

PlatformIO automatically scans `lib/` in the workspace root and includes all directories as libraries. **No explicit configuration needed** for most cases.

**What PlatformIO does**:
1. Scans `mica-ecosystem/lib/`
2. Finds `button_manager/`, `mqtt_handler/`, etc.
3. Adds them to the include path
4. Compiles `.cpp` files automatically
5. Links them into the firmware

### Root `platformio.ini` (Workspace Level)
```ini
; mica-ecosystem/platformio.ini
[platformio]
default_envs = esp32_c3_recirculator  ; Which app to build by default

src_dir = apps/recirculator/src       ; Source code location

description = MICA IoT Ecosystem - Monorepo with shared libraries
```

### App-Specific `platformio.ini`
Each app has its own configuration:

```ini
; apps/recirculator/platformio.ini
[env:esp32_c3_recirculator]
platform = espressif32
board = seeed_xiao_esp32c3
framework = arduino
monitor_speed = 115200

; Library dependency mode
lib_ldf_mode = chain+  ; Follow #include chain

; Include paths
build_flags = 
    -D ESP32_C3
    -Iinclude                      ; Global config (config.h, secrets.h)
    -Isrc                          ; App source
    -Isrc/drivers                  ; Device-specific drivers

; External dependencies (from Arduino Library Manager)
lib_deps =
    knolleary/PubSubClient@^2.8.0
    bblanchon/ArduinoJson@^6.18.5
    adafruit/Adafruit NeoPixel@^1.11.0
    ; ... more
```

**Key Points**:
- `lib_ldf_mode = chain+` → PlatformIO follows `#include` statements automatically
- No need to list `lib/` modules explicitly
- Only need to add **external** dependencies in `lib_deps`

---

## 📝 Code Standards

### Module Organization

#### Header File (`.h`)
```cpp
// lib/my_module/my_module.h
#ifndef MY_MODULE_H  // Header guard
#define MY_MODULE_H

#include <Arduino.h>

/**
 * @brief Initialize my module
 * @param param Configuration parameter
 * @return true if successful, false otherwise
 */
bool initializeMyModule(int param);

/**
 * @brief Do something useful
 */
void doSomething();

#endif // MY_MODULE_H
```

#### Implementation File (`.cpp`)
```cpp
// lib/my_module/my_module.cpp
#include "my_module.h"
#include "Log.h"

// Private variables (static)
static int internalState = 0;

// Private functions (static)
static void helperFunction() {
    // ...
}

// Public API implementation
bool initializeMyModule(int param) {
    internalState = param;
    Log::info("MyModule initialized with param: %d", param);
    return true;
}

void doSomething() {
    helperFunction();
}
```

### Naming Conventions

#### Functions
- **camelCase** for public API: `initializeMQTTHandler()`, `readTemperature()`
- **snake_case** for private: `static void handle_error()`

#### Variables
- **camelCase**: `int maxTemperature`, `bool isConnected`
- **g_** prefix for globals: `g_systemState`, `g_mqttClient`

#### Constants
- **UPPER_SNAKE_CASE** for macros: `#define BUTTON_PIN 9`
- **PascalCase** for constexpr: `constexpr char AP_SSID[]`

#### Types
- **PascalCase**: `typedef enum { ... } SystemState;`

### Comments
```cpp
// Single-line comment for brief explanation
int temperature = readSensor();

/**
 * @brief Multi-line Doxygen comment for functions
 * 
 * Detailed description of what the function does.
 * 
 * @param deviceType Type of device (e.g., "recirculator")
 * @param deviceId Unique device identifier
 * @return true if initialization successful
 */
bool initializeMQTTHandler(const char* deviceType, const char* deviceId);
```

---

## 🧪 Testing Strategy

### Unit Testing (Future)
```bash
# In each module directory
lib/mqtt_handler/
├── mqtt_handler.cpp
├── mqtt_handler.h
└── test/
    └── test_mqtt_handler.cpp  # Unit tests
```

### Integration Testing
```bash
# Compile all apps
platformio run -e esp32_c3_recirculator
platformio run -e esp32_gateway

# Upload and monitor
platformio run -e esp32_c3_recirculator --target upload
platformio device monitor
```

### Hardware Validation Checklist
- [ ] Compiles without errors
- [ ] WiFi connects
- [ ] MQTT publishes correctly
- [ ] Sensors read valid data
- [ ] Actuators respond to commands
- [ ] Config mode works
- [ ] OTA updates successful
- [ ] No memory leaks (monitor free heap)

---

## 🔄 Git Workflow

### Branch Strategy
```
main           ← Production-ready code
  ├─ feature/add-gateway        ← New app
  ├─ feature/improve-mqtt       ← Shared module enhancement
  └─ fix/wifi-reconnect-bug     ← Bug fix
```

### Commit Message Format
```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `refactor`: Code restructuring
- `docs`: Documentation
- `test`: Tests
- `chore`: Maintenance

**Scopes**:
- `mqtt`: mqtt_handler module
- `wifi`: wifi_connect or wifi_config_mode
- `recirculator`: Recirculator app
- `gateway`: Gateway app
- `monorepo`: Repository structure

**Examples**:
```bash
# Adding a feature
git commit -m "feat(mqtt): Add batch publishing for telemetry

- Implement mqttPublishBatch() function
- Optimize network usage for multiple metrics
- Update documentation"

# Breaking change
git commit -m "refactor(wifi): Rename connectWiFi to initializeWiFiConnection

BREAKING CHANGE: Function renamed for consistency

- Update all apps to use new name
- Update documentation"
```

### Pull Request Checklist
- [ ] Code compiles without errors
- [ ] All apps tested (if shared code changed)
- [ ] Documentation updated
- [ ] No hardcoded secrets (check `.gitignore`)
- [ ] Commit messages follow format
- [ ] No regressions introduced

---

## ⚠️ Common Pitfalls

### 1. Hardcoding Device Type
❌ **BAD**:
```cpp
// lib/mqtt_handler/mqtt_handler.cpp
void publishTelemetry() {
    mqttClient.publish("mica/dev/telemetry/recirculator/...", payload);  // Hardcoded!
}
```

✅ **GOOD**:
```cpp
// lib/mqtt_handler/mqtt_handler.cpp
static char deviceType[32];

void initializeMQTTHandler(const char* type, const char* id) {
    strncpy(deviceType, type, sizeof(deviceType));
}

void publishTelemetry() {
    char topic[128];
    snprintf(topic, sizeof(topic), "mica/dev/telemetry/%s/...", deviceType);
    mqttClient.publish(topic, payload);
}
```

### 2. Forgetting to Update All Apps
❌ **BAD**:
```bash
# Change mqtt_handler API
git add lib/mqtt_handler/
git commit -m "feat(mqtt): Add new parameter"

# Oops! Forgot to update apps/recirculator/
# Now recirculator doesn't compile!
```

✅ **GOOD**:
```bash
# Change mqtt_handler API and ALL apps in one commit
git add lib/mqtt_handler/ apps/recirculator/ apps/gateway/ docs/
git commit -m "feat(mqtt): Add new parameter

- Update mqtt_handler API
- Update recirculator to pass new parameter
- Update gateway to pass new parameter
- Update documentation

All apps compile successfully."
```

### 3. Circular Dependencies
❌ **BAD**:
```cpp
// lib/wifi_connect/wifi_connect.cpp
#include "mqtt_handler.h"  // wifi_connect depends on mqtt_handler

// lib/mqtt_handler/mqtt_handler.cpp
#include "wifi_connect.h"  // mqtt_handler depends on wifi_connect
// → Circular dependency! Won't compile.
```

✅ **GOOD** (Use events via `system_state`):
```cpp
// lib/wifi_connect/wifi_connect.cpp
#include "system_state.h"
void onWiFiConnected() {
    notifySystemState(EVENT_WIFI_CONNECTED);  // Notify via events
}

// lib/mqtt_handler/mqtt_handler.cpp
// system_state will call initializeMQTTHandler() when WiFi connects
// No direct dependency on wifi_connect
```

### 4. Not Using `.gitignore` for Secrets
❌ **BAD**:
```bash
git add include/secrets.h  # Commits AWS credentials to Git!
```

✅ **GOOD**:
```bash
# .gitignore already includes:
include/secrets.h
**/secrets.h

# Use template instead:
cp docs/secrets.h.template include/secrets.h
# Edit include/secrets.h with your credentials
# Git will ignore it automatically
```

---

## 📚 Resources

### Internal Documentation
- [API Reference](API.md) - Complete documentation for `lib/` modules
- [Architecture Overview](architecture.md) - System design and diagrams
- [Hardware Specs](hardware.md) - Pin mappings and schematics

### External Resources
- [PlatformIO Docs](https://docs.platformio.org/) - Build system
- [FreeRTOS Guide](https://www.freertos.org/Documentation) - RTOS concepts
- [AWS IoT Core](https://docs.aws.amazon.com/iot/) - MQTT backend

### Code Style
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) - Coding standards
- [Conventional Commits](https://www.conventionalcommits.org/) - Commit message format

---

## 🎓 Architecture Credits

**All monorepo architecture and best practices** were designed by:

**gaesca04** - Computer engineer, expert in software architecture and monorepo patterns

This guide implements professional software engineering standards taught in advanced coursework and recommended for scalable IoT ecosystems.

---

## 🆘 Getting Help

### Debugging Compilation Errors
```bash
# Clean build
platformio run --target clean

# Verbose output
platformio run -v

# Check which files are being compiled
platformio run -v | grep "Compiling"
```

### Common Errors

**"No such file or directory"**
→ Check `#include` paths and `build_flags` in `platformio.ini`

**"Undefined reference to"**
→ Function declared but not implemented, or wrong linking

**"Multiple definition of"**
→ Function/variable defined in header (should be in `.cpp`)

### Contact
- GitHub Issues: [mica-ecosystem/issues](https://github.com/nenbcn/mica-ecosystem/issues)
- Architecture questions: Ask gaesca04

---

**Last Updated**: 28 November 2025  
**Architecture**: gaesca04 (computer engineer, monorepo expert)
