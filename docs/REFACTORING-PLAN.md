# Plan de Refactorización - MICA Ecosystem Monorepo

## 🎓 Autoría y Reconocimiento

Este plan de refactorización ha sido diseñado siguiendo las recomendaciones del **ingeniero informático gaesca04**, quien ha aplicado conocimientos avanzados de arquitectura de software (específicamente sobre **monorepos**) adquiridos en su formación académica. Su análisis técnico profesional ha identificado puntos críticos de mejora que permitirán crear un ecosistema escalable y mantenible para todos los dispositivos MICA.

**gaesca04** es un técnico excelente cuyas indicaciones seguiremos al pie de la letra para garantizar la calidad profesional del código.

---

## 📋 Contexto: ¿Por qué Monorepo?

### Situación Actual
- **mica-recirculator**: Sistema de control de bomba de recirculación (relay, sensor temperatura)
- **mica-gateway**: Sistema de sensores con transmisión LoRa (en desarrollo)

### Problemas Identificados
1. **Duplicación de código**: Módulos WiFi, MQTT, Button Manager están duplicados
2. **Mantenimiento difícil**: Un bug fix en WiFi debe aplicarse en 2 repos
3. **Escalabilidad limitada**: Cada nuevo dispositivo MICA requiere copiar código
4. **Acoplamiento alto**: `mqtt_handler` tiene hardcoded "recirculator" en topics

### Solución: Monorepo
Estructura unificada `mica-ecosystem` con:
- **Apps específicas**: `apps/recirculator/`, `apps/gateway/`
- **Librerías compartidas**: `libs/core/` (wifi, mqtt, buttons, etc.)
- **Utilidades comunes**: `libs/utils/` (Log, UtcClock)

---

## 🎯 Objetivos del Proyecto

### Objetivos Técnicos
1. ✅ **Eliminar duplicación de código** entre recirculator y gateway
2. ✅ **Código genérico y reutilizable** (MQTT handler parametrizado)
3. ✅ **Mejor prácticas de C++** (eliminar includes.h anti-pattern)
4. ✅ **Arquitectura profesional** escalable a 5+ dispositivos MICA

### Objetivos de Calidad
1. ✅ **Compilación exitosa** después de cada issue
2. ✅ **Commits individuales** para cada cambio lógico
3. ✅ **Testing manual** en hardware antes de cada commit
4. ✅ **Documentación actualizada** con cada cambio

---

## 📊 Análisis Técnico - Revisión de gaesca04

### Archivos Compartibles (libs/core/)
| Módulo | Estado | Cambios Necesarios | Prioridad |
|--------|--------|-------------------|-----------|
| `wifi_connect` | ✅ Genérico | Ninguno | ✅ OK |
| `wifi_config_mode` | ✅ Genérico | Ninguno | ✅ OK |
| `button_manager` | ✅ Genérico | Ninguno | ✅ OK |
| `led_manager` | ✅ Genérico | Ninguno | ✅ OK |
| `system_state` | ✅ Genérico | Ninguno | ✅ OK |
| `device_id` | ✅ Genérico | Ninguno | ✅ OK |
| `ota_manager` | ✅ Genérico | Ninguno | ✅ OK |
| `eeprom_config` | ⚠️ Semi-genérico | Key-value v2 (futuro) | 🟢 BAJA |
| **`mqtt_handler`** | ❌ Hardcoded | **Parametrizar deviceType** | 🔴 **ALTA** |

### Archivos Específicos (apps/recirculator/)
- `main.cpp` - Entry point específico
- `relay_controller.cpp/h` - Control de relay (no existe en gateway)
- `temperature_sensor.cpp/h` - Sensor DS18B20 (recirculator only)
- `displayManager.cpp/h` - OLED con info de recirculator

### Archivos de Configuración
- **`config.h`**: Mover a `libs/core/` (hardware común ESP32)
- **`secrets.h`**: Mover a `libs/core/` (credenciales compartidas)
- **`includes.h`**: 🗑️ **ELIMINAR** (anti-patrón → includes explícitos)

---

## 🚀 Estrategia de Implementación

### Enfoque: Refactorizar ANTES de Mover
**Razón**: Cambios incrementales probados minimizan riesgo de errores.

```
Fase 1: Pre-Migración (repo actual)
  ├─ Refactorizar mqtt_handler → deviceType parameter
  ├─ Consolidar config.h (mover pines desde includes.h)
  ├─ Eliminar includes.h → includes explícitos
  └─ Probar: Compilación + Upload + Test funcional

Fase 2: Migración Monorepo
  ├─ Crear estructura mica-ecosystem/
  ├─ Mover archivos a apps/ y libs/
  ├─ Configurar platformio.ini
  └─ Probar: Compilación en nueva estructura

Fase 3: Documentación
  ├─ Actualizar architecture.md
  ├─ Crear docs/shared-modules.md
  └─ Crear docs/monorepo-guide.md
```

---

## 📝 Issues del Proyecto

### FASE 1: PRE-MIGRACIÓN (repo actual)

#### Issue #1: Refactorizar mqtt_handler - Parametrizar deviceType
**Prioridad**: 🔴 **CRÍTICA**  
**Objetivo**: Hacer `mqtt_handler` genérico para recirculator y gateway.

**Cambios Necesarios**:
1. Modificar `initializeMQTTHandler()`:
   ```cpp
   // ANTES:
   void initializeMQTTHandler();
   
   // DESPUÉS:
   void initializeMQTTHandler(const char* deviceType, const char* deviceId);
   ```

2. Parametrizar topics MQTT:
   ```cpp
   // ANTES (hardcoded):
   "mica/dev/telemetry/recirculator/" + deviceId + "/temperature"
   
   // DESPUÉS (dinámico):
   "mica/dev/telemetry/" + deviceType + "/" + deviceId + "/temperature"
   ```

3. Actualizar llamadas en `main.cpp`:
   ```cpp
   initializeMQTTHandler("recirculator", deviceId);
   ```

**Testing**:
- [ ] Compila sin errores
- [ ] Conecta a MQTT AWS IoT
- [ ] Topics correctos en AWS IoT console
- [ ] Comandos power-state funcionan
- [ ] Telemetría se publica correctamente

**Commit**: 
```
refactor(mqtt): Parametrize deviceType for multi-device support

- Add deviceType parameter to initializeMQTTHandler()
- Make MQTT topics dynamic based on device type
- Maintain backward compatibility with recirculator
- Preparation for mica-ecosystem monorepo

Recommended by: gaesca04 (monorepo architecture expert)
```

---

#### Issue #2: Consolidar config.h - Hardware Pins
**Prioridad**: 🟡 **MEDIA**  
**Objetivo**: Mover definiciones de pines desde `includes.h` a `config.h`.

**Cambios Necesarios**:
1. Mover definiciones de `includes.h` a `config.h`:
   ```cpp
   // libs/core/config.h
   #ifdef ESP32_C3
       #define BUTTON_PIN 9
       #define RELAY_PIN 8
       #define TEMPERATURE_SENSOR_PIN 2
       // ... resto pines
   #else
       #define BUTTON_PIN 13
       #define RELAY_PIN 12
       // ... resto pines
   #endif
   ```

2. Eliminar duplicaciones
3. Mantener `secrets.h` separado

**Testing**:
- [ ] Compila sin errores
- [ ] Todos los pines GPIO funcionan
- [ ] LED, botón, relay, sensor OK

**Commit**:
```
refactor(config): Consolidate hardware pin definitions

- Move GPIO pin definitions from includes.h to config.h
- Organize by ESP32 variant (C3 vs WROOM)
- Remove duplicated definitions
- Better organization for shared hardware config

Recommended by: gaesca04
```

---

#### Issue #3: Eliminar includes.h - Includes Explícitos
**Prioridad**: 🔴 **ALTA**  
**Objetivo**: Eliminar anti-patrón de header global, usar includes explícitos.

**Cambios Necesarios**:
1. Para cada `.cpp`, añadir includes necesarios:
   ```cpp
   // Ejemplo: mqtt_handler.cpp
   #include "mqtt_handler.h"
   #include "eeprom_config.h"
   #include "secrets.h"
   #include "device_id.h"
   #include "system_state.h"
   #include <WiFiClientSecure.h>
   #include <PubSubClient.h>
   #include <ArduinoJson.h>
   ```

2. Eliminar `#include "includes.h"` de todos los archivos
3. Eliminar archivo `includes.h`
4. Verificar cada módulo independientemente

**Por qué es importante** (según Google C++ Style Guide):
- ✅ Dependencias explícitas y claras
- ✅ Menor tiempo de compilación
- ✅ Facilita testing unitario
- ✅ Evita dependencias ocultas

**Testing**:
- [ ] Compila sin errores
- [ ] No hay includes faltantes
- [ ] Cada módulo compila independientemente

**Commit**:
```
refactor(includes): Remove global includes.h anti-pattern

- Add explicit includes to each .cpp file
- Remove includes.h header
- Improve compilation time and dependencies visibility
- Follow Google C++ Style Guide best practices

Recommended by: gaesca04 (software architecture expert)
```

---

#### Issue #4: Probar Compilación y Funcionalidad Pre-Migración
**Prioridad**: 🟢 **VERIFICACIÓN**  
**Objetivo**: Validar que todos los cambios de Fase 1 funcionan correctamente.

**Testing Completo**:
- [ ] `platformio run` → Compilación exitosa
- [ ] `platformio run --target upload` → Upload exitoso
- [ ] WiFi conecta correctamente
- [ ] MQTT conecta a AWS IoT
- [ ] Botón corto toggle relay
- [ ] Botón largo entra config mode
- [ ] Sensor temperatura publica MQTT
- [ ] Relay timer funciona (timeout + temperatura)
- [ ] Display OLED muestra info correcta
- [ ] OTA funciona

**Commit**:
```
test: Verify all pre-migration refactoring changes

- Compilation successful
- All hardware components tested
- MQTT communication verified
- Ready for monorepo migration

Phase 1 complete: gaesca04 recommendations implemented
```

---

### FASE 2: MIGRACIÓN MONOREPO

#### Issue #5: Crear Estructura mica-ecosystem
**Prioridad**: 🟡 **MEDIA**  
**Objetivo**: Crear nueva estructura de directorios monorepo.

**Cambios Necesarios**:
1. Crear estructura:
   ```
   mica-ecosystem/
   ├── apps/
   │   └── recirculator/
   │       ├── platformio.ini
   │       └── src/
   ├── libs/
   │   ├── core/
   │   └── utils/
   └── docs/
   ```

2. Configurar `.gitignore` global
3. Crear `README.md` raíz del monorepo
4. Mover `secrets.h` a `libs/core/` (añadir a `.gitignore`)

**No Commit**: Preparación, no hay código aún.

---

#### Issue #6: Mover Archivos a apps/recirculator/
**Prioridad**: 🟡 **MEDIA**  
**Objetivo**: Mover archivos específicos de recirculator.

**Archivos a Mover**:
```
mica-recirculator/src/          → mica-ecosystem/apps/recirculator/src/
├── main.cpp
├── relay_controller.cpp/h
├── temperature_sensor.cpp/h
└── displayManager.cpp/h
```

**Commit**:
```
chore(monorepo): Move recirculator-specific files to apps/

- Relocate main.cpp to apps/recirculator/src/
- Move relay_controller to apps/recirculator/src/
- Move temperature_sensor to apps/recirculator/src/
- Move displayManager to apps/recirculator/src/

Monorepo structure by: gaesca04
```

---

#### Issue #7: Mover Módulos Compartidos a libs/core/
**Prioridad**: 🟡 **MEDIA**  
**Objetivo**: Centralizar código compartido.

**Archivos a Mover**:
```
mica-recirculator/src/          → mica-ecosystem/libs/core/
├── wifi_connect/
│   ├── wifi_connect.cpp
│   └── wifi_connect.h
├── wifi_config_mode/
│   ├── wifi_config_mode.cpp
│   └── wifi_config_mode.h
├── mqtt_handler/
│   ├── mqtt_handler.cpp
│   └── mqtt_handler.h
├── button_manager/
│   ├── button_manager.cpp
│   └── button_manager.h
├── led_manager/
│   ├── led_manager.cpp
│   └── led_manager.h
├── system_state/
│   ├── system_state.cpp
│   └── system_state.h
├── device_id/
│   ├── device_id.cpp
│   └── device_id.h
├── eeprom_config/
│   ├── eeprom_config.cpp
│   └── eeprom_config.h
├── ota_manager/
│   ├── ota_manager.cpp
│   └── ota_manager.h
├── config.h
└── secrets.h
```

**Commit**:
```
chore(monorepo): Move shared modules to libs/core/

- Relocate wifi, mqtt, button, led managers
- Move system_state to core
- Move eeprom_config, device_id, ota_manager
- Centralize config.h and secrets.h

Shared libraries architecture by: gaesca04
```

---

#### Issue #8: Mover Utilidades a libs/utils/
**Prioridad**: 🟢 **BAJA**  
**Objetivo**: Organizar utilidades compartidas.

**Archivos a Mover**:
```
mica-recirculator/lib/          → mica-ecosystem/libs/utils/
├── Log/
│   ├── Log.cpp
│   └── Log.h
└── UtcClock/
    ├── UtcClock.cpp
    └── UtcClock.h
```

**Commit**:
```
chore(monorepo): Move utilities to libs/utils/

- Relocate Log library
- Relocate UtcClock library
- Organize shared utilities

Structure by: gaesca04
```

---

#### Issue #9: Configurar platformio.ini para Monorepo
**Prioridad**: 🔴 **CRÍTICA**  
**Objetivo**: Configurar PlatformIO para usar librerías compartidas.

**Cambios Necesarios**:
```ini
# apps/recirculator/platformio.ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino

# Apuntar a libs compartidas
lib_extra_dirs = 
    ../../libs/core
    ../../libs/utils

# Include paths
build_flags = 
    -I../../libs/core
    -I../../libs/core/wifi_connect
    -I../../libs/core/mqtt_handler
    -I../../libs/core/button_manager
    -I../../libs/core/led_manager
    -I../../libs/core/system_state
    -I../../libs/core/device_id
    -I../../libs/core/eeprom_config
    -I../../libs/core/ota_manager
    -I../../libs/utils/Log
    -I../../libs/utils/UtcClock

lib_deps = 
    adafruit/Adafruit NeoPixel@^1.10.6
    paulstoffregen/OneWire@^2.3.7
    milesburton/DallasTemperature@^3.9.1
    adafruit/Adafruit SSD1306@^2.5.7
    adafruit/Adafruit GFX Library@^1.11.3
    knolleary/PubSubClient@^2.8
    bblanchon/ArduinoJson@^6.21.2
    me-no-dev/ESPAsyncWebServer@^1.2.3
    me-no-dev/AsyncTCP@^1.1.1

monitor_speed = 115200
upload_speed = 921600
```

**Testing**:
- [ ] `platformio run` compila correctamente
- [ ] Encuentra todas las librerías en `libs/`
- [ ] No hay errores de includes

**Commit**:
```
config(platformio): Configure monorepo library paths

- Add lib_extra_dirs for shared libraries
- Configure build_flags for include paths
- Point to libs/core and libs/utils
- Maintain all existing dependencies

PlatformIO monorepo config by: gaesca04
```

---

#### Issue #10: Probar Compilación en Monorepo
**Prioridad**: 🔴 **CRÍTICA**  
**Objetivo**: Validar que la estructura monorepo compila correctamente.

**Testing**:
```bash
cd mica-ecosystem/apps/recirculator
platformio run
```

**Verificar**:
- [ ] Compilación exitosa sin errores
- [ ] Todos los includes resuelven correctamente
- [ ] Librerías compartidas encontradas
- [ ] Tamaño del firmware similar al anterior

**Commit**:
```
test(monorepo): Verify compilation in new structure

- Successful compilation in monorepo
- All shared libraries linked correctly
- Ready for hardware testing

Monorepo migration complete: gaesca04 architecture implemented
```

---

#### Issue #11: Upload y Testing en Hardware
**Prioridad**: 🔴 **CRÍTICA**  
**Objetivo**: Validar funcionalidad completa en dispositivo real.

**Testing Completo**:
```bash
cd mica-ecosystem/apps/recirculator
platformio run --target upload
platformio device monitor
```

**Checklist Funcional**:
- [ ] Dispositivo arranca correctamente
- [ ] WiFi conecta
- [ ] MQTT conecta a AWS IoT
- [ ] Botón toggle relay funciona
- [ ] Sensor temperatura publica
- [ ] Relay timer funciona
- [ ] Display OLED correcto
- [ ] Config mode (long press) funciona
- [ ] Topics MQTT correctos (con "recirculator")

**Commit**:
```
test(hardware): Validate all functionality in monorepo

- Hardware testing successful
- All features working as expected
- MQTT topics correct with deviceType parameter
- Monorepo migration complete and verified

Architecture validated by: gaesca04
```

---

### FASE 3: DOCUMENTACIÓN

#### Issue #12: Actualizar Documentación Principal
**Prioridad**: 🟡 **MEDIA**  
**Objetivo**: Documentar nueva arquitectura monorepo.

**Archivos a Crear/Actualizar**:
1. `mica-ecosystem/README.md` (raíz monorepo)
2. `mica-ecosystem/docs/architecture-recirculator.md` (mover desde `architecture.md`)
3. `mica-ecosystem/docs/shared-modules.md` (nueva)
4. `mica-ecosystem/docs/monorepo-guide.md` (nueva)
5. `mica-ecosystem/apps/recirculator/README.md` (específico)

**Commit**:
```
docs: Complete monorepo documentation

- Add root README.md with project overview
- Create architecture-recirculator.md
- Document shared modules in shared-modules.md
- Add monorepo-guide.md for developers
- Update app-specific README

Documentation structure by: gaesca04
```

---

#### Issue #13: Crear Template para secrets.h
**Prioridad**: 🟢 **BAJA**  
**Objetivo**: Facilitar configuración de credenciales.

**Archivo a Crear**:
```cpp
// docs/secrets.h.template
#ifndef SECRETS_H
#define SECRETS_H

// WiFi Credentials (will be saved to EEPROM via config mode)
// These are defaults, can be changed via AP mode

// MQTT AWS IoT Configuration
constexpr char AWS_IOT_ENDPOINT[] = "your-endpoint.iot.region.amazonaws.com";
constexpr int MQTT_PORT = 8883;

// AWS IoT Root CA Certificate
constexpr char AWS_CERT_CA[] = R"EOF(
-----BEGIN CERTIFICATE-----
[Your AWS Root CA Certificate]
-----END CERTIFICATE-----
)EOF";

// IoT API for Device Provisioning
const String IOT_API_ENDPOINT = "https://your-api.execute-api.region.amazonaws.com/prod";
const String IOT_API_KEY = "your-api-key";

#endif
```

**Commit**:
```
docs: Add secrets.h template for easy setup

- Create secrets.h.template in docs/
- Document required AWS IoT credentials
- Add instructions for device provisioning

Documentation by: gaesca04 recommendations
```

---

## 📅 Timeline Estimado

| Fase | Issues | Tiempo Estimado | Responsable |
|------|--------|-----------------|-------------|
| **Fase 1: Pre-Migración** | #1-#4 | 4-6 horas | Developer + gaesca04 review |
| **Fase 2: Migración** | #5-#11 | 6-8 horas | Developer + gaesca04 review |
| **Fase 3: Documentación** | #12-#13 | 2-3 horas | Developer |
| **TOTAL** | 13 issues | **12-17 horas** | - |

---

## ✅ Criterios de Éxito

### Por Issue
- [ ] Compilación exitosa sin errores ni warnings
- [ ] Testing manual en hardware (cuando aplique)
- [ ] Commit individual con mensaje descriptivo
- [ ] Documentación inline actualizada

### Por Fase
- [ ] Todas las funcionalidades previas mantienen operatividad
- [ ] No se introducen regresiones
- [ ] Testing completo pass (WiFi, MQTT, relay, sensor)

### Proyecto Completo
- [ ] Estructura monorepo funcional
- [ ] Código compartido entre apps sin duplicación
- [ ] MQTT handler genérico (`deviceType` parametrizado)
- [ ] Eliminado anti-patrón `includes.h`
- [ ] Documentación completa y actualizada
- [ ] Ready para añadir `apps/gateway/` en futuro

---

## 🎓 Agradecimientos

Este plan de refactorización ha sido posible gracias a la revisión técnica profesional y recomendaciones del **ingeniero informático gaesca04**, quien ha aplicado su conocimiento de arquitectura de software avanzada (monorepos) para diseñar una solución escalable y mantenible.

**gaesca04** es un técnico excelente cuyas indicaciones de ingeniería de software seguimos al pie de la letra.

---

## 📞 Contacto y Revisiones

Para cualquier duda sobre la implementación de este plan, consultar con:
- **gaesca04** - Revisor de Arquitectura y Diseño
- **Equipo MICA** - Implementación y Testing

---

**Última Actualización**: 28 Noviembre 2025  
**Versión del Plan**: 1.0  
**Estado**: 📋 Pendiente de Aprobación
