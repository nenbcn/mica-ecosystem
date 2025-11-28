# MICA Recirculator - Water Recirculation Control System

> **Part of**: MICA Ecosystem Monorepo  
> **Architecture by**: gaesca04 (computer engineer)

Intelligent water recirculation pump controller with temperature monitoring, MQTT telemetry, and local OLED display.

---

## 🎯 Features

### Core Functionality
- ✅ **Relay Control**: ON/OFF control of water pump via GPIO
- ✅ **Temperature Monitoring**: DS18B20 1-Wire sensor (5-second polling)
- ✅ **Automatic Shutoff**: Stops pump when target temperature reached
- ✅ **Safety Timeout**: Maximum runtime protection (default: 2 minutes)
- ✅ **Local Display**: SSD1306 OLED shows status, temperature, timer
- ✅ **Audio Feedback**: Buzzer melodies for status changes
- ✅ **RGB LED**: Visual status indicator (WS2812B)

### Connectivity
- ✅ **WiFi**: Auto-connect with stored credentials
- ✅ **MQTT**: AWS IoT Core telemetry and remote control
- ✅ **OTA Updates**: Over-the-air firmware updates
- ✅ **Config Mode**: AP + captive portal for WiFi setup

### User Interface
- ✅ **Button Control**:
  - Short press: Toggle pump ON/OFF
  - Long press (5s): Enter WiFi config mode
- ✅ **OLED Display**: Shows temperature, state, timer, WiFi/MQTT status
- ✅ **LED Indicators**: Color-coded status (red/green/blue)
- ✅ **Buzzer**: Startup, success, game over melodies

---

## 🔌 Hardware

### ESP32-C3 (Seeed XIAO)
- **MCU**: ESP32-C3 (RISC-V, 160MHz)
- **RAM**: 400KB SRAM
- **Flash**: 4MB
- **WiFi**: 802.11 b/g/n
- **Power**: 5V USB-C

### Peripherals
| Component | Model | Interface | Pin | Purpose |
|-----------|-------|-----------|-----|---------|
| Temperature Sensor | DS18B20 | 1-Wire | D0 (GPIO2) | Water temperature |
| Relay Module | Generic 5V | GPIO | D8 (GPIO8) | Pump control |
| Display | SSD1306 128x64 | I2C | D4/D5 (GPIO6/7) | Local UI |
| Button | Momentary | GPIO | D9 (GPIO9) | User input |
| LED | WS2812B | NeoPixel | D3 (GPIO5) | Status indicator |
| Buzzer | Passive | PWM | D7 (GPIO20) | Audio feedback |

**Pin Map** (defined in `include/config.h`):
```cpp
#define BUTTON_PIN 9              // D9
#define RELAY_PIN 8               // D8
#define TEMPERATURE_SENSOR_PIN 2  // D0
#define BUZZER_PIN 20             // D7
#define SDA_PIN 6                 // D4 (I2C)
#define SCL_PIN 7                 // D5 (I2C)
#define NEOPIXEL_PIN 5            // D3
```

---

## 📊 System Architecture

### FreeRTOS Tasks
| Task | Priority | Stack | Function |
|------|----------|-------|----------|
| WiFi Connect | 1 | 4096 | WiFi connection management |
| WiFi Config Mode | 1 | 8192 | AP + captive portal |
| MQTT Connect | 2 | 4096 | MQTT connection |
| MQTT Publish | 2 | 4096 | Queue-based publishing |
| Temperature Sensor | 3 | 4096 | Read DS18B20 every 5s |
| Relay Controller | 3 | 4096 | Timer, temp checks, safety |
| Display Manager | 1 | 4096 | Update OLED display |
| LED Manager | 1 | 2048 | RGB LED patterns |
| Button Manager | 1 | 2048 | Debounce, event detection |
| OTA Manager | 1 | 8192 | Firmware updates |
| System State | 2 | 4096 | Event coordinator |

### State Machine
```
[SYSTEM_STATE_INIT]
    ↓
[SYSTEM_STATE_CONNECTING]  ← WiFi connecting
    ↓
[SYSTEM_STATE_CONNECTED_WIFI]  ← WiFi OK
    ↓
[SYSTEM_STATE_CONNECTED_MQTT]  ← MQTT OK
    ↓
[SYSTEM_STATE_OPERATIONAL]  ← Ready
    ↓
[SYSTEM_STATE_OTA_UPDATE]  ← Updating firmware
```

### Relay Controller Logic
```
[RELAY_OFF]
    ↓ (Button press or MQTT command)
[RELAY_ON]
    ├─ Timer running (countdown)
    ├─ Temperature monitoring
    └─ Checks every loop:
        ├─ Timeout reached? → RELAY_OFF (Game Over melody)
        ├─ Target temp reached? → RELAY_OFF (Success melody)
        └─ Manual stop? → RELAY_OFF
```

---

## 📡 MQTT Topics

### Device ID
- **Format**: Last 6 hex digits of MAC address (e.g., `A1B2C3`)
- **Function**: `getDeviceId()` from `lib/device_id/`

### Telemetry (Publish)
| Topic | Payload | Retain | Frequency |
|-------|---------|--------|-----------|
| `mica/dev/telemetry/recirculator/{deviceId}/temperature` | `{"deviceId":"ABC123","temperature":25.5,"timestamp":1234567890}` | Yes | 5s |
| `mica/dev/telemetry/recirculator/{deviceId}/power-state` | `{"deviceId":"ABC123","state":"ON","remainingTime":120,"timestamp":1234567890}` | Yes | On change |
| `mica/dev/status/recirculator/{deviceId}/online` | `{"deviceId":"ABC123","online":true,"timestamp":1234567890}` | Yes | On connect |

### Commands (Subscribe)
| Topic | Payload | Action |
|-------|---------|--------|
| `mica/dev/command/recirculator/{deviceId}/power-state` | `"ON"` or `"OFF"` | Turn pump on/off |
| `mica/dev/command/recirculator/{deviceId}/max-temperature` | `"35.0"` | Set target temperature (°C) |
| `mica/dev/command/recirculator/{deviceId}/max-time` | `"120"` | Set max runtime (seconds) |

### Configuration Topics
| Topic | Payload | Action |
|-------|---------|--------|
| `mica/dev/config/recirculator/{deviceId}/max-temperature` | `{"value":35.0}` | Persistent config update |
| `mica/dev/config/recirculator/{deviceId}/max-time` | `{"value":120}` | Persistent config update |

**Example AWS IoT Console Command**:
```bash
# Turn pump ON
aws iot-data publish \
  --topic "mica/dev/command/recirculator/A1B2C3/power-state" \
  --payload "ON"

# Set max temperature
aws iot-data publish \
  --topic "mica/dev/command/recirculator/A1B2C3/max-temperature" \
  --payload "35.0"
```

---

## 🚀 Getting Started

### Prerequisites
- PlatformIO Core or VS Code + PlatformIO extension
- ESP32-C3 board with USB-C cable
- AWS IoT Core account with device certificate

### 1. Clone Repository
```bash
git clone https://github.com/nenbcn/mica-ecosystem.git
cd mica-ecosystem
```

### 2. Configure Secrets
```bash
# Copy template
cp docs/secrets.h.template include/secrets.h

# Edit with your credentials
nano include/secrets.h
```

**Required Credentials** (`include/secrets.h`):
```cpp
constexpr char AWS_IOT_ENDPOINT[] = "your-endpoint.iot.us-east-1.amazonaws.com";
constexpr int MQTT_PORT = 8883;
constexpr char AWS_CERT_CA[] = R"EOF(
-----BEGIN CERTIFICATE-----
[Your Amazon Root CA Certificate]
-----END CERTIFICATE-----
)EOF";
const String IOT_API_ENDPOINT = "https://your-api.execute-api.us-east-1.amazonaws.com/prod";
const String IOT_API_KEY = "your-api-key";
```

### 3. Compile Firmware
```bash
cd mica-ecosystem
platformio run
```

### 4. Upload to Device
```bash
platformio run --target upload
```

### 5. Monitor Serial Output
```bash
platformio device monitor
```

### 6. First Boot Setup
1. **Power on** device
2. **Wait** for startup melody
3. **Hold button** for 5 seconds
4. **Connect** to WiFi AP "MICA-Recirculator"
5. **Open** browser (captive portal auto-opens)
6. **Enter** WiFi credentials
7. **Save** and device will reboot

---

## 🔧 Configuration

### Hardware Configuration (`include/config.h`)
```cpp
// Temperature Settings
#define DEFAULT_MAX_TEMPERATURE 30.0f  // °C
#define DEFAULT_MAX_TIME 120          // seconds

// EEPROM Addresses
#define MAX_TEMP_ADDR 100
#define MAX_TIME_ADDR 200

// Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
```

### Runtime Configuration (via MQTT)
```bash
# Set max temperature to 35°C
aws iot-data publish \
  --topic "mica/dev/command/recirculator/ABC123/max-temperature" \
  --payload "35.0"

# Set max runtime to 3 minutes
aws iot-data publish \
  --topic "mica/dev/command/recirculator/ABC123/max-time" \
  --payload "180"
```

Configuration is saved to EEPROM and persists across reboots.

---

## 📱 Usage

### Normal Operation
1. **Short press button** → Pump turns ON
2. **OLED shows**: Temperature, timer countdown, state
3. **LED**: Green (operational)
4. **Pump stops automatically** when:
   - Target temperature reached (Success melody ♪)
   - Maximum time elapsed (Game Over melody ♫)
   - Manual button press

### WiFi Configuration Mode
1. **Long press button** (5 seconds) → LED blinks blue
2. **Connect** to AP "MICA-Recirculator"
3. **Browser** opens captive portal automatically
4. **Enter** WiFi credentials
5. **Save** → Device reboots and connects

### Remote Control (MQTT)
```bash
# Turn ON via AWS IoT
aws iot-data publish \
  --topic "mica/dev/command/recirculator/ABC123/power-state" \
  --payload "ON"

# Turn OFF via AWS IoT
aws iot-data publish \
  --topic "mica/dev/command/recirculator/ABC123/power-state" \
  --payload "OFF"
```

---

## 🐛 Troubleshooting

### Temperature Sensor Shows -127°C
**Cause**: DS18B20 not connected or faulty  
**Solution**:
- Check wiring (VCC, GND, Data)
- Verify 4.7kΩ pull-up resistor on data line
- Sensor still publishes to MQTT (backend needs visibility)

### WiFi Won't Connect
**Cause**: Wrong credentials or signal weak  
**Solution**:
- Enter config mode (long press button)
- Re-enter WiFi credentials
- Check router distance/signal strength

### MQTT Not Connecting
**Cause**: Wrong endpoint, certificate, or credentials  
**Solution**:
- Verify `AWS_IOT_ENDPOINT` in `include/secrets.h`
- Check device certificate is registered in AWS IoT
- Verify policy allows connect/publish/subscribe

### Relay Won't Turn ON
**Cause**: Hardware issue or EEPROM corruption  
**Solution**:
- Check relay module wiring (VCC, GND, signal to GPIO8)
- Verify relay LED lights when active
- Factory reset: Hold button 10+ seconds

### OTA Update Fails
**Cause**: Network issue or corrupted firmware  
**Solution**:
- Ensure stable WiFi during update
- Check firmware URL is accessible
- Re-flash via USB if brick

---

## 🏗️ Development

### Adding Features
See [Monorepo Development Guide](../../docs/monorepo-guide.md)

### Modifying Device-Specific Code
- **Entry point**: `src/main.cpp`
- **State machine**: `src/system_state.cpp/h`
- **Drivers**: `src/drivers/`
  - `relay_controller.cpp/h` - Pump control logic
  - `temperature_sensor.cpp/h` - DS18B20 driver
  - `displayManager.cpp/h` - OLED UI

### Using Shared Modules
All `lib/` modules are automatically available:
```cpp
#include "wifi_connect.h"    // From lib/wifi_connect/
#include "mqtt_handler.h"    // From lib/mqtt_handler/
#include "Log.h"             // From lib/Log/
```

### Compilation
```bash
# Clean build
platformio run --target clean

# Compile
platformio run

# Upload
platformio run --target upload

# Monitor
platformio device monitor
```

---

## 📊 Memory Usage

**ESP32-C3 Resources**:
- **RAM**: 12.9% (42,284 / 327,680 bytes)
- **Flash**: 80.2% (1,051,602 / 1,310,720 bytes)

**Optimization Tips**:
- Use `F()` macro for string literals (saves RAM)
- Reduce task stack sizes if possible
- Disable verbose logging in production (`CORE_DEBUG_LEVEL=0`)

---

## 🔐 Security

### Credentials Storage
- ✅ `secrets.h` in `.gitignore` (never committed)
- ✅ WiFi credentials encrypted in EEPROM
- ✅ TLS/SSL for MQTT (AWS IoT certificates)

### Device Provisioning
1. Flash firmware with default API endpoint
2. Device requests certificate from provisioning API
3. API generates certificate and returns to device
4. Device stores certificate in EEPROM
5. Connects to AWS IoT with unique certificate

---

## 🌟 Credits

### Architecture
**gaesca04** - Computer engineer, software architecture expert

All architectural decisions, monorepo structure, and best practices follow professional software engineering standards recommended by gaesca04.

### Development Team
- **nenbcn** - Implementation, hardware integration, testing

---

## 📄 License

[Your License Here]

---

## 📞 Support

- **GitHub Issues**: [mica-ecosystem/issues](https://github.com/nenbcn/mica-ecosystem/issues)
- **Documentation**: [docs/](../../docs/)
- **Architecture Questions**: Contact gaesca04

---

**Last Updated**: 28 November 2025  
**Version**: 2.0.0 (Monorepo)  
**Architecture**: gaesca04 (computer engineer)
