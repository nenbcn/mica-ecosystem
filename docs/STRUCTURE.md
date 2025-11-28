# MICA Ecosystem - Estructura Final del Monorepo

> **Arquitectura validada por**: gaesca04 (ingeniero informático)  
> **Fecha**: 28 Noviembre 2025  
> **Estado**: ✅ IMPLEMENTADA

## 📂 Estructura de Directorios

```
mica-ecosystem/
│
├── apps/                                    # Aplicaciones independientes
│   └── recirculator/                        # App: Control bomba recirculación
│       ├── platformio.ini                   # (futuro) Config específico
│       └── src/
│           ├── main.cpp                     # Entry point
│           ├── system_state.cpp/h           # Coordinador (específico app)
│           └── drivers/                     # Drivers específicos recirculator
│               ├── relay_controller.cpp/h   # Driver: Relay GPIO
│               ├── temperature_sensor.cpp/h # Driver: DS18B20 1-Wire
│               └── displayManager.cpp/h     # Driver: OLED I2C
│
├── lib/                                     # Librerías COMPARTIDAS
│   ├── services/                            # Servicios de negocio
│   │   ├── wifi_connect/
│   │   │   ├── wifi_connect.cpp
│   │   │   └── wifi_connect.h
│   │   ├── wifi_config_mode/
│   │   │   ├── wifi_config_mode.cpp
│   │   │   └── wifi_config_mode.h
│   │   ├── mqtt_handler/
│   │   │   ├── mqtt_handler.cpp
│   │   │   └── mqtt_handler.h
│   │   ├── ota_manager/
│   │   │   ├── ota_manager.cpp
│   │   │   └── ota_manager.h
│   │   ├── eeprom_config/
│   │   │   ├── eeprom_config.cpp
│   │   │   └── eeprom_config.h
│   │   └── device_id/
│   │       ├── device_id.cpp
│   │       └── device_id.h
│   │
│   ├── drivers/                             # Drivers compartidos
│   │   ├── button_manager/
│   │   │   ├── button_manager.cpp
│   │   │   └── button_manager.h
│   │   └── led_manager/
│   │       ├── led_manager.cpp
│   │       └── led_manager.h
│   │
│   └── utils/                               # Utilidades
│       ├── Log/
│       │   ├── Log.cpp
│       │   └── Log.h
│       └── UtcClock/
│           ├── UtcClock.cpp
│           └── UtcClock.h
│
├── include/                                 # Configuración global
│   ├── config.h                             # Hardware pins, defines
│   └── secrets.h                            # Credenciales (gitignored)
│
├── docs/                                    # Documentación
│   ├── architecture.md
│   ├── ARCHITECTURE-PROPOSAL.md
│   ├── ISSUES.md
│   └── STRUCTURE.md                         # Este archivo
│
├── .gitignore
├── platformio.ini                           # Config raíz (apunta a apps/)
└── README.md
```

---

## 🎯 Clasificación de Módulos

### ✅ COMPARTIDOS (lib/)

Usados por **todas las apps** (recirculator, gateway futuro):

| Módulo | Ubicación | Tipo | Razón para compartir |
|--------|-----------|------|---------------------|
| `wifi_connect` | lib/services/ | Service | Todas las apps necesitan WiFi |
| `wifi_config_mode` | lib/services/ | Service | Portal configuración común |
| `mqtt_handler` | lib/services/ | Service | Comunicación AWS IoT (genérico) |
| `ota_manager` | lib/services/ | Service | Updates OTA comunes |
| `eeprom_config` | lib/services/ | Service | Storage persistente común |
| `device_id` | lib/services/ | Service | ID único basado en MAC |
| `button_manager` | lib/drivers/ | Driver | Gestión botones GPIO genérica |
| `led_manager` | lib/drivers/ | Driver | Control LEDs RGB genérico |
| `Log` | lib/utils/ | Utility | Sistema logging común |
| `UtcClock` | lib/utils/ | Utility | Gestión tiempo NTP común |

### ❌ ESPECÍFICOS POR APP (apps/*/src/)

Únicos para cada aplicación:

| Módulo | Ubicación | Tipo | Razón para NO compartir |
|--------|-----------|------|------------------------|
| `main.cpp` | apps/recirculator/src/ | Entry | Inicializa módulos específicos |
| `system_state` | apps/recirculator/src/ | Coordinator | Coordina relay, temp, display |
| `relay_controller` | apps/recirculator/src/drivers/ | Driver | Solo recirculator tiene relay |
| `temperature_sensor` | apps/recirculator/src/drivers/ | Driver | Solo recirculator tiene DS18B20 |
| `displayManager` | apps/recirculator/src/drivers/ | Driver | Solo recirculator tiene OLED |

**Gateway tendrá**:
- `apps/gateway/src/main.cpp`
- `apps/gateway/src/system_state.*` (coordinando LoRa, etc.)
- `apps/gateway/src/drivers/lora_manager.*`

---

## 🔑 Decisiones Clave de Arquitectura

### 1. ¿Por qué system_state NO es compartido?

**Problema identificado**:
```cpp
// lib/application/system_state/system_state.cpp (ANTES - INCORRECTO)
#include "displayManager.h"       // ❌ Específico de recirculator
#include "relay_controller.h"     // ❌ Específico de recirculator
#include "temperature_sensor.h"   // ❌ Específico de recirculator

void initializeSystemState() {
    initializeDisplayManager();   // ❌ Gateway no tiene display
    initializeRelayController();  // ❌ Gateway no tiene relay
    // ...
}
```

**Solución**:
- Cada app tiene su propio `system_state` que coordina **sus propios módulos**
- Recirculator: coordina relay, temperature, display
- Gateway: coordinará LoRa, sensores diferentes

### 2. ¿Qué SÍ es compartido?

**Criterio**: Servicios de infraestructura **sin lógica de negocio específica**

✅ **WiFi**: Todas las apps se conectan igual  
✅ **MQTT**: Protocolo genérico (deviceType parametrizado)  
✅ **OTA**: Proceso de update igual para todas  
✅ **Storage**: Key-value genérico  
✅ **Button/LED**: Hardware básico igual

### 3. Configuración global (include/)

**Por qué en `include/` y no en `lib/`**:
- `include/` es el estándar PlatformIO para headers globales
- `config.h` tiene `#define RELAY_PIN` que solo usa recirculator, pero también tiene pines compartidos
- `secrets.h` tiene credenciales AWS comunes a todas las apps
- Más fácil referenciar: `-Iinclude` que `-Ilib/config/`

---

## 📦 Ventajas de Esta Estructura

### ✅ No hay código duplicado
- WiFi, MQTT, OTA → Una sola implementación
- Cambio en servicio compartido → Afecta todas las apps

### ✅ Apps independientes
- Recirculator y gateway pueden evolucionar por separado
- Diferentes módulos, diferentes coordinadores

### ✅ Escalabilidad
- Añadir nueva app: Copiar template, adaptar `system_state`, añadir drivers específicos
- Servicios compartidos ya disponibles automáticamente

### ✅ Estándar PlatformIO
- `lib/` → PlatformIO busca automáticamente
- `include/` → Estándar para headers globales
- No necesita configuración compleja

---

## 🚀 Añadir Nueva App (Gateway)

```bash
# 1. Crear estructura
mkdir -p apps/gateway/src

# 2. Copiar template de main y system_state
cp apps/recirculator/src/main.cpp apps/gateway/src/
cp apps/recirculator/src/system_state.* apps/gateway/src/

# 3. Adaptar system_state para gateway
# - Eliminar includes de relay, temperature, display
# - Añadir includes de lora_manager, sensores gateway
# - Adaptar initializeSystemState()

# 4. Crear drivers específicos gateway
mkdir -p apps/gateway/src/drivers
# apps/gateway/src/drivers/lora_manager.cpp/h

# 5. Crear platformio.ini (opcional, o usar raíz)
cp platformio.ini apps/gateway/

# 6. Compilar
cd apps/gateway
platformio run
```

**Lo que YA tendrá automáticamente**:
- ✅ WiFi connect
- ✅ MQTT handler (AWS IoT)
- ✅ OTA manager
- ✅ Config mode (AP)
- ✅ Button manager
- ✅ LED manager
- ✅ Log system
- ✅ UTC Clock

---

## 📝 Resumen Ejecutivo

**SHARED (lib/)**:
- 6 services (WiFi, MQTT, OTA, storage, device_id)
- 2 drivers (button, LED)
- 2 utils (Log, UtcClock)

**PER-APP (apps/*/src/)**:
- `main.cpp` (entry point)
- `system_state.*` (coordinator específico)
- Drivers específicos (relay, sensors, displays, LoRa, etc.)

**GLOBAL (include/)**:
- `config.h` (hardware defines)
- `secrets.h` (credentials)

---

**Arquitectura**: gaesca04 (ingeniero informático)  
**Implementación**: Equipo MICA  
**Fecha**: 28 Noviembre 2025
