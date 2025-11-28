# MICA Ecosystem - IoT Monorepo

> **Architecture by**: gaesca04 (Computer Engineer)

IoT monorepo for ESP32 devices sharing common services (WiFi, MQTT, OTA) and drivers (buttons, LEDs).

---

## 🎯 Structure

```
mica-ecosystem/
├── apps/                    # Independent applications
│   ├── recirculator/        # Water pump controller (ESP32-C3)
│   └── gateway/             # LoRa sensor hub (future)
│
├── lib/                     # Shared libraries (organized by layer)
│   ├── services/            # Business logic (WiFi, MQTT, OTA, etc.)
│   ├── drivers/             # Hardware abstraction (buttons, LEDs)
│   └── utils/               # Helpers (Log, UtcClock)
│
└── include/                 # Global configuration
    ├── config.h             # Hardware pins
    └── secrets.h            # Credentials (gitignored)
```

**Key Principle**: Apps are independent but share code from `lib/` to avoid duplication.

---

## 🚀 Quick Start

### Compile & Upload

**Recirculator (ESP32-C3)**:
```bash
cd apps/recirculator
~/.platformio/penv/bin/platformio run                    # Compile
~/.platformio/penv/bin/platformio run --target upload    # Flash device
~/.platformio/penv/bin/platformio device monitor         # Serial monitor
```

**Clean build**:
```bash
~/.platformio/penv/bin/platformio run --target clean
```

### Configuration

1. **Hardware pins**: Edit `include/config.h`
2. **Credentials**: Copy `include/secrets.h.template` to `include/secrets.h` and add:
   - AWS IoT certificates
   - WiFi default credentials (optional)

---

## 📱 Devices

### Recirculator (Production ✅)
Water recirculation pump controller with temperature monitoring.

**Features**:
- Relay control with safety timeouts
- Real-time temperature monitoring (DS18B20)
- MQTT telemetry to AWS IoT
- Local OLED display
- WiFi config mode (captive portal)
- OTA firmware updates

**Hardware**: ESP32-C3, Relay, DS18B20, SSD1306 OLED  
**Docs**: [apps/recirculator/README.md](./apps/recirculator/README.md)

### Gateway (Planned 🚧)
LoRa sensor hub reusing 80% of recirculator's code.

---

## 📚 Documentation

| Document | Purpose |
|----------|---------|
| [docs/architecture.md](./docs/architecture.md) | Complete system architecture (4 layers, modules, MQTT) |
| [docs/hardware.md](./docs/hardware.md) | Hardware specifications and pinout |
| [docs/CHANGELOG.md](./docs/CHANGELOG.md) | Version history |
| [.github/copilot-instructions.md](./.github/copilot-instructions.md) | Code standards, Git workflow, naming conventions |
| [apps/recirculator/README.md](./apps/recirculator/README.md) | Recirculator usage guide |
| [docs/project/ISSUES.md](./docs/project/ISSUES.md) | Project tasks |

---

## 🛠️ Development

### Adding New Application

1. **Create structure**:
   ```bash
   mkdir -p apps/my_device/src
   ```

2. **Copy template**:
   ```bash
   cp apps/recirculator/platformio.ini apps/my_device/
   # Edit board and settings
   ```

3. **Create main.cpp**:
   ```cpp
   #include "wifi_connect.h"  // From lib/ (shared)
   #include "mqtt_handler.h"  // From lib/ (shared)
   
   void setup() {
       initWiFi();
       initializeMQTTHandler("my_device", getDeviceId());
       // Device-specific code...
   }
   ```

4. **Compile**:
   ```bash
   cd apps/my_device
   ~/.platformio/penv/bin/platformio run
   ```

All shared services (WiFi, MQTT, OTA) are automatically available!

### Modifying Shared Module

⚠️ **Changes in `lib/` affect ALL apps** - Test thoroughly:
```bash
# Compile all apps to verify no breakage
~/.platformio/penv/bin/platformio run
```

See [docs/architecture.md](./docs/architecture.md#5-modules-by-layer) for module classification.

---

## 🏆 Credits

Architecture designed by **gaesca04** (Computer Engineer) applying professional monorepo and layered architecture patterns.

---

**Last Updated**: 28 November 2025  
**Status**: Recirculator in production, monorepo structure operational
