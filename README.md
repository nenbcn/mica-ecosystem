# MICA Ecosystem - IoT Devices Monorepo

> **Architecture by**: gaesca04 (computer engineer, software architecture expert)

Sistema integrado de dispositivos IoT MICA con arquitectura monorepo diseñada para **máxima reutilización de código** entre múltiples aplicaciones.

---

## 🎯 Concepto Clave

**Múltiples aplicaciones que comparten servicios y drivers comunes**

- `apps/` = Proyectos independientes (cada uno con su `platformio.ini`)
- `lib/` = Librerías compartidas (PlatformIO busca aquí automáticamente)
- `include/` = Configuración global (hardware, credenciales)

---

## 🎓 Arquitectura en 4 Capas

Diseñado por: **gaesca04** (ingeniero informático, experto en monorepos)

```
┌─────────────────────────────────────────────┐
│     APPLICATION LAYER                       │
│  (Coordination, State Management)           │
│  Location: lib/application/ + apps/*/main   │
├─────────────────────────────────────────────┤
│     SERVICES LAYER                          │
│  (WiFi, MQTT, OTA, Storage)                 │
│  Location: lib/services/                    │
├─────────────────────────────────────────────┤
│     DRIVERS LAYER                           │
│  Shared: lib/drivers/                       │
│  Specific: apps/*/src/                      │
│  (GPIO, I2C, 1-Wire, Hardware)              │
├─────────────────────────────────────────────┤
│     UTILS LAYER                             │
│  (Logging, Time)                            │
│  Location: lib/utils/                       │
└─────────────────────────────────────────────┘
```

---

## 📁 Estructura del Proyecto

```
mica-ecosystem/
│
├── apps/                        # 📱 Aplicaciones independientes
│   ├── recirculator/            # APP 1: Control bomba recirculación
│   │   ├── platformio.ini       # Config específico
│   │   └── src/
│   │       ├── main.cpp         # Entry point
│   │       ├── relay_controller.*       # Driver específico
│   │       ├── temperature_sensor.*     # Driver específico
│   │       └── displayManager.*         # Driver específico
│   │
│   └── gateway/                 # APP 2: Hub sensores LoRa (futuro)
│       ├── platformio.ini
│       └── src/
│           └── main.cpp
│
├── lib/                         # 📚 Librerías COMPARTIDAS
│   │
│   ├── application/             # CAPA: Coordinación
│   │   └── system_state/        # Event coordinator, state machine
│   │
│   ├── services/                # CAPA: Lógica de negocio
│   │   ├── wifi_connect/        # WiFi connection management
│   │   ├── wifi_config_mode/    # AP mode + captive portal
│   │   ├── mqtt_handler/        # AWS IoT MQTT (generic)
│   │   ├── ota_manager/         # Firmware updates
│   │   ├── eeprom_config/       # Persistent storage
│   │   └── device_id/           # Unique device ID
│   │
│   ├── drivers/                 # CAPA: Drivers compartidos
│   │   ├── button_manager/      # GPIO button handler
│   │   └── led_manager/         # WS2812B NeoPixel
│   │
│   └── utils/                   # CAPA: Utilidades
│       ├── Log/                 # Logging system
│       └── UtcClock/            # Time management
│
├── include/                     # ⚙️ Configuración GLOBAL
│   ├── config.h                 # Hardware pins, ESP32 defines
│   └── secrets.h                # Credentials (gitignored)
│
├── docs/                        # 📖 Documentación
│   ├── architecture.md          # Arquitectura detallada
│   ├── ARCHITECTURE-PROPOSAL.md # Propuesta aprobada
│   ├── ISSUES.md                # Issues y progreso
│   └── REFACTORING-PLAN.md      # Plan de migración
│
├── platformio.ini               # Config raíz (opcional/legacy)
└── README.md                    # Este archivo
```

---

## 🌟 Dispositivos

### Recirculator (Producción ✅)
Control inteligente de bomba de recirculación de agua con:
- **Hardware**: ESP32-C3, Relay, DS18B20 (temperatura), OLED SSD1306
- **Conectividad**: WiFi, MQTT (AWS IoT Core)
- **Características**:
  - Control relay con timeouts configurables
  - Monitoreo de temperatura en tiempo real
  - Telemetría a AWS IoT vía MQTT
  - Display local OLED
  - Config mode (captive portal)
  - OTA updates

### Gateway (Planificado 🚧)
Hub de sensores con transmisión LoRa
- Reutilizará: WiFi, MQTT, OTA, button_manager, led_manager
- Específico: LoRa driver, sensor aggregation

## 🚀 Ventajas del Monorepo

✅ **Cero duplicación**: Un solo `mqtt_handler.cpp` para todos los dispositivos  
✅ **Mantenimiento centralizado**: Bug fix en WiFi → afecta todos los dispositivos  
✅ **Escalable**: Añadir nuevo dispositivo = reutilizar 80% del código  
✅ **Consistencia**: Misma arquitectura, mismo estilo, mismos estándares  
✅ **Testing compartido**: Validar una vez, usar en todos lados

## 🛠️ Desarrollo

### Compilar y Subir Firmware

**Recirculator**:
```bash
cd apps/recirculator
~/.platformio/penv/bin/platformio run           # Compilar
~/.platformio/penv/bin/platformio run --target upload  # Subir
~/.platformio/penv/bin/platformio device monitor       # Monitor serial
```

**Gateway** (futuro):
```bash
cd apps/gateway
~/.platformio/penv/bin/platformio run
```

### Estructura de Cada App

Cada app en `apps/*/` es independiente:
- Tiene su propio `platformio.ini`
- Define su placa y configuración
- Usa librerías de `lib/` automáticamente (PlatformIO busca en workspace root)
- Accede a configs globales en `include/`

### Añadir Nueva Aplicación

1. **Crear directorio**:
   ```bash
   mkdir -p apps/my_device/src
   ```

2. **Copiar platformio.ini template**:
   ```bash
   cp apps/recirculator/platformio.ini apps/my_device/
   # Editar board, settings específicos
   ```

3. **Crear main.cpp**:
   ```cpp
   // apps/my_device/src/main.cpp
   #include "system_state.h"  // Automáticamente de lib/application/
   #include "wifi_connect.h"  // Automáticamente de lib/services/
   
   void setup() {
       initializeSystemState();
       // Device-specific initialization
   }
   ```

4. **Compilar**:
   ```bash
   cd apps/my_device
   ~/.platformio/penv/bin/platformio run
   ```

¡Todos los servicios compartidos están disponibles automáticamente!

---

**Compilar el recirculator:**
```bash
# Desde la raíz del monorepo
~/.platformio/penv/bin/platformio run -e esp32_c3_recirculator

# O simplemente (usa el entorno por defecto)
~/.platformio/penv/bin/platformio run
```

**Flashear a dispositivo:**
```bash
~/.platformio/penv/bin/platformio run -e esp32_c3_recirculator --target upload
```

**Monitor serial:**
```bash
~/.platformio/penv/bin/platformio device monitor
```

**Limpiar build:**
```bash
~/.platformio/penv/bin/platformio run --target clean
```

### Cambiar Entre Aplicaciones

Para trabajar en diferentes dispositivos, cambia el symlink `src`:

```bash
# Trabajar en recirculator (actual)
ln -sfn apps/recirculator/src src

# Trabajar en gateway (futuro)
ln -sfn apps/gateway/src src
```

O actualiza `default_envs` en `platformio.ini` y compila con `-e <entorno>`.

### Añadir Nueva Aplicación

1. **Crear estructura:**
   ```bash
   mkdir -p apps/nuevo-dispositivo/src/{application,services,drivers}
   ```

2. **Copiar archivos base:**
   ```bash
   cp apps/recirculator/src/config.h apps/nuevo-dispositivo/src/
   cp apps/recirculator/src/secrets.h apps/nuevo-dispositivo/src/
   ```

3. **Añadir entorno en `platformio.ini`:**
   ```ini
   [env:nuevo_dispositivo]
   platform = espressif32
   board = <tu_board>
   framework = arduino
   build_flags = 
       -I src
       -I src/application
       -I src/services  
       -I src/drivers
   lib_extra_dirs = libs/core
   ```

4. **Cambiar symlink y compilar:**
   ```bash
   ln -sfn apps/nuevo-dispositivo/src src
   platformio run -e nuevo_dispositivo
   ```

### Modificar Módulo Compartido

⚠️ **Cuidado**: Cambios en `lib/` o `libs/core/` afectan **todos los dispositivos**
- Mantener retrocompatibilidad
- Usar dependency injection (parámetros, callbacks)
- Hacer device-agnostic
- Testing exhaustivo antes de commitear
- Compilar **todos** los entornos para verificar:
  ```bash
  platformio run -e esp32_c3_recirculator
  platformio run -e esp32_c3_gateway
  ```

## 📚 Documentación

### Documentación Técnica

- **[docs/architecture.md](./docs/architecture.md)** - Arquitectura del sistema recirculator (3 capas, FreeRTOS, MQTT)
- **[docs/hardware.md](./docs/hardware.md)** - Especificaciones de hardware y pinout
- **[docs/ISSUES.md](./docs/ISSUES.md)** - Tareas y planificación del proyecto
- **[docs/REFACTORING-PLAN.md](./docs/REFACTORING-PLAN.md)** - Plan de migración al monorepo
- **[docs/CHANGELOG-2025-11-28.md](./docs/CHANGELOG-2025-11-28.md)** - Historial de cambios

### Guías de Desarrollo

- **[.github/copilot-instructions.md](./.github/copilot-instructions.md)** - Estándares de código, workflow Git, testing
  - Naming conventions (camelCase, snake_case, UPPER_CASE)
  - Arquitectura de 3 capas (Application → Services → Drivers)
  - Git commit format y branching strategy
  - Testing checklist
  - Principios de arquitectura por gaesca04

## 🏆 Reconocimiento

Toda la arquitectura del monorepo está basada en las recomendaciones profesionales de **gaesca04** (ingeniero informático), quien aplicó sus conocimientos avanzados de arquitectura de software, específicamente patrones de monorepo y arquitectura en capas, para diseñar una solución escalable, mantenible y profesional.

gaesca04 es un técnico excelente cuyas indicaciones seguimos al pie de la letra.

---

**Última Actualización**: 28 Noviembre 2025  
**Arquitectura por**: gaesca04 (computer engineer)  
**Estado**: ✅ Estructura creada, migración en progreso
