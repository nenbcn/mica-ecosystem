# Hardware Configuration - MICA Recirculator

## Microcontroller
**Seeed Studio XIAO ESP32-C3**
- Chipset: ESP32-C3 (RISC-V single-core @ 160MHz)
- RAM: 400KB SRAM
- Flash: 4MB
- WiFi: 2.4GHz 802.11 b/g/n
- Bluetooth: BLE 5.0
- USB: Native USB serial/JTAG
- Dimensions: 21mm x 17.5mm

## Pin Configuration

### Power Pins
| Cable Color | Function | Description |
|-------------|----------|-------------|
| **Azul** | 5V | Power supply input (5V) |
| **Negro** | GND | Ground |
| **Verde** | 3V3 | 3.3V regulated output |

### Signal Pins
| Pin Name | GPIO | Cable Color | Function | Description |
|----------|------|-------------|----------|-------------|
| **D0** | GPIO 2 | **Amarillo** | Temperature Sensor | DS18B20 OneWire temperature sensor |
| **D3** | GPIO 5 | - | NeoPixel LED | WS2812B RGB LED for status indication |
| **D7** | GPIO 20 | - | Buzzer | Piezo buzzer for audio feedback |
| **D8** | GPIO 8 | **Rojo** | Relay Control | Relay module control (active HIGH) |
| **D9** | GPIO 9 | - | Button Input | Physical button with internal pull-up |
| **D4** | GPIO 6 | - | I2C SDA | OLED display (optional) |
| **D5** | GPIO 7 | - | I2C SCL | OLED display (optional) |

## Peripheral Components

### 1. DS18B20 Temperature Sensor
- **Interface**: OneWire (1-Wire protocol)
- **Pin**: D0 (GPIO 2)
- **Cable**: Yellow
- **Power**: 3.3V or 5V
- **Pull-up resistor**: 4.7kΩ required between data and VCC
- **Range**: -55°C to +125°C
- **Accuracy**: ±0.5°C (-10°C to +85°C)

### 2. Relay Module
- **Pin**: D8 (GPIO 8)
- **Cable**: Red
- **Control**: Active HIGH (3.3V logic)
- **Power**: Typically 5V for relay coil
- **Function**: Controls water pump or heating element
- **Max operating time**: 2 minutes (configurable via MQTT)
- **Auto-stop conditions**: 
  - Timeout (default 2 min)
  - Maximum temperature reached

### 3. NeoPixel LED (WS2812B)
- **Pin**: D3 (GPIO 5)
- **Type**: WS2812B RGB LED
- **Protocol**: Addressable single-wire protocol
- **Power**: 5V recommended (works with 3.3V)
- **Status Indicators**:
  - **Red blinking**: Connecting to WiFi
  - **Green blinking**: Connected to WiFi
  - **Green solid**: Connected to MQTT

### 4. Buzzer
- **Pin**: D7 (GPIO 20)
- **Type**: Passive piezo buzzer
- **Control**: PWM/Tone generation via LEDC
- **Melodies**:
  - **Startup**: 3-note test sequence (E5-G5-E6)
  - **Success**: Super Mario Bros main theme (temperature reached)
  - **Timeout**: Super Mario Bros Game Over (time expired)

### 5. Physical Button
- **Pin**: D9 (GPIO 9)
- **Type**: Momentary push button
- **Pull-up**: Internal pull-up enabled
- **Active**: LOW (pressed = GND)
- **Functions**:
  - **Short press**: Toggle relay ON/OFF
  - **Long press (5s)**: Enter WiFi configuration mode

### 6. OLED Display (Optional)
- **Interface**: I2C
- **SDA**: D4 (GPIO 6)
- **SCL**: D5 (GPIO 7)
- **Type**: SSD1306 128x64 monochrome
- **Address**: 0x3C (default)
- **Power**: 3.3V
- **Function**: Display system status, temperature, and relay state

## Wiring Diagram Summary

```
┌─────────────────────────────────────┐
│   XIAO ESP32-C3 (Top View)          │
│                                     │
│  5V  ●───────────● Azul (Power)    │
│  GND ●───────────● Negro (Ground)  │
│  3V3 ●───────────● Verde (3.3V)    │
│                                     │
│  D0  ●───────────● Amarillo (Temp) │
│  D3  ●───────────  NeoPixel        │
│  D7  ●───────────  Buzzer          │
│  D8  ●───────────● Rojo (Relay)    │
│  D9  ●───────────  Button          │
│                                     │
└─────────────────────────────────────┘
```

## Notes

### GPIO Limitations on ESP32-C3
- **Strapping pins**: GPIO 2, 8, 9 have boot-time functions
- **GPIO 20**: May have limitations on XIAO ESP32-C3 board
- **USB pins**: GPIO 18, 19 used for USB (do not use)

### Power Considerations
- **Total current**: ~200mA typical operation
- **Relay current**: Add relay coil current (typically 50-80mA)
- **NeoPixel current**: Up to 60mA per LED at full white
- **Recommended power**: 5V @ 500mA minimum

### Programming
- **USB**: Native USB-C on board
- **Boot mode**: Hold BOOT button while connecting USB (if needed)
- **Reset**: Press RESET button to restart

## MQTT Topics
All topics use device ID based on MAC address: `588C81A532B0`

### Subscribe (Commands)
- `mica/dev/command/recirculator/{deviceId}/power-state` - ON/OFF relay control
- `mica/dev/command/recirculator/{deviceId}/max-temperature` - Set max temperature (float)
- `mica/dev/command/recirculator/{deviceId}/max-time` - Set max time in seconds (1-3600)
- `mica/dev/command/recirculator/{deviceId}/ota` - OTA firmware update

### Publish (Telemetry)
- `mica/dev/telemetry/recirculator/{deviceId}/temperature` - Temperature readings (1Hz)
- `mica/dev/status/recirculator/{deviceId}/healthcheck` - Device health status

## Firmware Features
- **FreeRTOS**: Multi-tasking architecture
- **WiFi**: Auto-reconnect with EEPROM credentials
- **MQTT**: AWS IoT Core with TLS certificates
- **OTA**: Over-the-air firmware updates
- **Configuration**: WiFi AP mode for initial setup
- **Persistence**: EEPROM storage for settings
