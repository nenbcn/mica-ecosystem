# Shared Libraries - MICA Ecosystem

> **Architecture by**: gaesca04 (computer engineer)

This directory contains all **shared code** used across multiple MICA applications (recirculator, gateway, etc.).

PlatformIO automatically finds and links these libraries when building any app.

---

## 📂 Directory Structure

```
lib/
├── services/        # Business logic (no direct hardware)
├── drivers/         # Hardware abstraction (shared)
└── utils/           # Helper utilities
```

**Note**: The `application/` layer (`system_state`) lives in `apps/*/src/` as it's app-specific coordination.

---

## 🏗️ Layers

### services/
**Purpose**: Business services without direct hardware access

| Module | Description |
|--------|-------------|
| `wifi_connect` | WiFi connection management with auto-reconnect |
| `wifi_config_mode` | AP mode + captive portal for configuration |
| `mqtt_handler` | AWS IoT MQTT communication (generic, deviceType parameter) |
| `ota_manager` | Over-The-Air firmware updates |
| `eeprom_config` | Persistent key-value storage |
| `device_id` | Unique device identifier from MAC address |

**Shared by**: All apps

---

### drivers/
**Purpose**: Hardware abstraction for **shared** peripherals

| Module | Hardware | Description |
|--------|----------|-------------|
| `button_manager` | GPIO input | Button handler with debounce, SHORT_PRESS/LONG_PRESS events |
| `led_manager` | WS2812B NeoPixel | RGB LED status indicator |

**Shared by**: All apps

**Note**: Device-specific drivers (relay, temperature sensor, display) are in `apps/*/src/`

---

### utils/
**Purpose**: Helper utilities

| Module | Description |
|--------|-------------|
| `Log` | Logging with severity levels (DEBUG, INFO, WARN, ERROR) |
| `UtcClock` | Time management and NTP synchronization |

**Shared by**: All apps

---

## 🔧 Usage in Apps

PlatformIO automatically includes these libraries. In your app code:

```cpp
// apps/recirculator/src/main.cpp
#include "system_state.h"     // From apps/recirculator/src/
#include "wifi_connect.h"     // From lib/services/wifi_connect/
#include "mqtt_handler.h"     // From lib/services/mqtt_handler/
#include "button_manager.h"   // From lib/drivers/button_manager/
#include "Log.h"              // From lib/utils/Log/

void setup() {
    initializeSystemState();
    // Use shared services automatically
}
```

No special configuration needed in `platformio.ini` - PlatformIO finds `lib/` from workspace root.

---

## 📝 Adding New Shared Module

1. **Choose the correct layer**:
   - No hardware access → `services/`
   - Hardware abstraction → `drivers/`
   - Helper functions → `utils/`
   - App coordination → `apps/*/src/` (app-specific)

2. **Create module directory**:
   ```bash
   mkdir -p lib/services/my_module
   ```

3. **Add source files**:
   ```bash
   lib/services/my_module/
   ├── my_module.cpp
   └── my_module.h
   ```

4. **Use in any app**:
   ```cpp
   #include "my_module.h"
   ```

All apps will automatically have access to it!

---

## 🚫 What NOT to put here

- **Device-specific code**: Put in `apps/*/src/`
- **App entry points**: `main.cpp` stays in `apps/*/src/`
- **Configs/secrets**: Use `include/` directory

---

**Last Updated**: 28 November 2025  
**Architecture**: gaesca04 (computer engineer)
