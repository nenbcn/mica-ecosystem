# MICA Ecosystem - IoT Devices Monorepo

Sistema integrado de dispositivos IoT MICA con arquitectura monorepo diseñada para máxima reutilización de código y mantenibilidad.

## 🎓 Arquitectura

Diseñado por: **gaesca04** (ingeniero informático, experto en arquitectura de software y monorepos)

### Patrón de 3 Capas

```
┌─────────────────────────────────────┐
│     APPLICATION LAYER               │
│  (Business Logic, Coordination)     │
├─────────────────────────────────────┤
│     SERVICES LAYER                  │
│  (WiFi, MQTT, OTA, Storage)         │
├─────────────────────────────────────┤
│     DRIVERS LAYER                   │
│  (GPIO, I2C, 1-Wire, Hardware)      │
└─────────────────────────────────────┘
```

**Principios de Diseño:**
- **Application Layer**: Lógica de negocio, coordinación de eventos, entry point
- **Services Layer**: Funcionalidad sin acceso directo a hardware (networking, storage, OTA)
- **Drivers Layer**: Abstracción de hardware, interacción GPIO/I2C/1-Wire

## 📁 Estructura del Monorepo

```
mica-ecosystem/
├── platformio.ini           # ⚙️ Config PlatformIO raíz (define entornos)
├── src/                     # 🔗 Symlink -> apps/recirculator/src (PlatformIO compatibility)
├── apps/                    # 📱 Aplicaciones específicas por dispositivo
│   └── recirculator/        # Control de bomba de recirculación
│       └── src/
│           ├── application/ # Lógica de negocio (main, system_state)
│           ├── services/    # WiFi, MQTT, OTA, EEPROM, device_id
│           ├── drivers/     # Hardware (relay, temp sensor, display, buttons, LEDs)
│           ├── config.h     # Configuración hardware específica
│           └── secrets.h    # Credenciales WiFi/MQTT (gitignored)
├── lib/                     # 📚 Librerías personalizadas compartidas
│   ├── Log/                 # Sistema de logging
│   └── UtcClock/            # Gestión de tiempo UTC
├── libs/
│   └── core/                # 🔮 Módulos compartidos (futuro - migración pendiente)
│       ├── application/     # system_state (coordinador de eventos)
│       ├── services/        # WiFi, MQTT, OTA, EEPROM
│       ├── drivers/         # button_manager, led_manager
│       └── utils/           # Utilidades compartidas
└── docs/                    # 📖 Documentación del ecosistema
```

### Estructura PlatformIO

El proyecto usa **un único `platformio.ini`** en la raíz con **múltiples entornos**:

```ini
[platformio]
default_envs = esp32_c3_recirculator

[env:esp32_c3_recirculator]
platform = espressif32
board = seeed_xiao_esp32c3
# El código está en apps/recirculator/src/
# Accesible mediante symlink src -> apps/recirculator/src

[env:esp32_c3_gateway]  # Futuro
# Usará apps/gateway/src/
```

**Ventaja del symlink**: PlatformIO espera código en `src/`, el symlink apunta a `apps/recirculator/src/` manteniendo la organización del monorepo.

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

### Compilar Firmware

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
