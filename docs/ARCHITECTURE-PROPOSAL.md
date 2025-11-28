# MICA Ecosystem - Arquitectura Monorepo DEFINITIVA

> **Diseñada por**: gaesca04 (ingeniero informático, experto en arquitectura)  
> **Fecha**: 28 Noviembre 2025  
> **Estado**: ✅ VALIDADA Y APROBADA

## 🎯 Principio Fundamental

**Múltiples aplicaciones (apps/) que comparten servicios y drivers comunes (lib/)**

Esta es la estructura estándar de PlatformIO para monorepos:
- `apps/` = Proyectos individuales con su propio `platformio.ini`
- `lib/` = Librerías compartidas (PlatformIO busca aquí automáticamente)
- `include/` = Headers globales compartidos

---

## 📐 Estructura Definitiva

```
mica-ecosystem/
│
├── apps/                                    # Aplicaciones específicas del ecosistema
│   │
│   ├── recirculator/                        # APP 1: Control bomba recirculación
│   │   ├── platformio.ini                   # Config específico recirculator
│   │   └── src/
│   │       ├── main.cpp                     # Entry point recirculator
│   │       ├── relay_controller.cpp/h       # Driver: Control relay (GPIO)
│   │       ├── temperature_sensor.cpp/h     # Driver: DS18B20 (1-Wire)
│   │       └── displayManager.cpp/h         # Driver: SSD1306 OLED (I2C)
│   │
│   └── gateway/                             # APP 2: Hub sensores LoRa (FUTURO)
│       ├── platformio.ini                   # Config específico gateway
│       └── src/
│           ├── main.cpp                     # Entry point gateway
│           └── lora_manager.cpp/h           # Driver: LoRa radio
│
├── lib/                                     # Librerías COMPARTIDAS (PlatformIO busca aquí)
│   │
│   ├── services/                            # CAPA: Servicios de negocio (sin HW directo)
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
│   │   ├── device_id/
│   │   │   ├── device_id.cpp
│   │   │   └── device_id.h
│   │   └── README.md                        # Explica servicios compartidos
│   │
│   ├── drivers/                             # CAPA: Drivers hardware compartidos
│   │   ├── button_manager/
│   │   │   ├── button_manager.cpp
│   │   │   └── button_manager.h
│   │   ├── led_manager/
│   │   │   ├── led_manager.cpp
│   │   │   └── led_manager.h
│   │   └── README.md                        # Explica drivers compartidos
│   │
│   └── utils/                               # CAPA: Utilidades helper
│       ├── Log/
│       │   ├── Log.cpp
│       │   └── Log.h
│       ├── UtcClock/
│       │   ├── UtcClock.cpp
│       │   └── UtcClock.h
│       └── README.md                        # Explica utilidades
│
├── include/                                 # Headers GLOBALES compartidos
│   ├── config.h                             # Configuración hardware (pines GPIO, etc.)
│   └── secrets.h                            # Credenciales (AWS, WiFi) - GITIGNORED
│
├── docs/                                    # Documentación del ecosistema
│   ├── architecture.md
│   ├── ISSUES.md
│   ├── REFACTORING-PLAN.md
│   └── secrets.h.template
│
├── .gitignore                               # Ignora secrets.h, .pio, etc.
├── README.md                                # Documentación raíz del monorepo
└── platformio.ini                           # Config global (opcional, para referencia)
```

---

## 🏗️ Capas de Arquitectura

### 1️⃣ Application Layer (Coordinación) - ESPECÍFICO POR APP
**Ubicación**: `apps/*/src/` (cada app tiene el suyo)

| Módulo | Responsabilidad | Compartido | Razón |
|--------|-----------------|------------|-------|
| `system_state` | Event coordinator, state machine, task lifecycle | ❌ No | Coordina módulos específicos de cada app |
| `main.cpp` | Entry point | ❌ No | Inicializa módulos específicos |

**Por qué NO es compartido**: 
- `system_state.cpp` del recirculator incluye y coordina: `relay_controller`, `temperature_sensor`, `displayManager`
- `system_state.cpp` del gateway incluirá y coordinará: `lora_manager`, diferentes sensores
- Cada app tiene diferentes módulos → diferentes coordinadores

**Función**: Coordina módulos específicos de la app, gestiona eventos, maneja transiciones de estado.

---

### 2️⃣ Services Layer (Lógica de Negocio)
**Ubicación**: `lib/services/`

| Módulo | Responsabilidad | Compartido |
|--------|-----------------|------------|
| `wifi_connect` | WiFi connection, auto-reconnect | ✅ Sí |
| `wifi_config_mode` | AP mode + captive portal | ✅ Sí |
| `mqtt_handler` | AWS IoT MQTT communication (generic) | ✅ Sí |
| `ota_manager` | Firmware updates OTA | ✅ Sí |
| `eeprom_config` | Persistent storage (key-value) | ✅ Sí |
| `device_id` | Unique device identifier (MAC) | ✅ Sí |

**Función**: Servicios de negocio sin acceso directo a hardware (usan APIs ESP32).

---

### 3️⃣ Drivers Layer (Hardware Abstraction)

#### **Drivers COMPARTIDOS**
**Ubicación**: `lib/drivers/`

| Módulo | Hardware | Compartido |
|--------|----------|------------|
| `button_manager` | GPIO button input + debounce | ✅ Sí |
| `led_manager` | WS2812B NeoPixel RGB LED | ✅ Sí |

#### **Drivers ESPECÍFICOS de Recirculator**
**Ubicación**: `apps/recirculator/src/`

| Módulo | Hardware | Compartido |
|--------|----------|------------|
| `relay_controller` | GPIO relay control + safety timer | ❌ No (solo recirculator) |
| `temperature_sensor` | DS18B20 1-Wire temperature | ❌ No (solo recirculator) |
| `displayManager` | SSD1306 OLED I2C display | ❌ No (solo recirculator) |

**Función**: Abstracción de hardware, interacción GPIO/I2C/1-Wire.

---

### 4️⃣ Utils Layer (Utilidades)
**Ubicación**: `lib/utils/`

| Módulo | Responsabilidad | Compartido |
|--------|-----------------|------------|
| `Log` | Logging con niveles (debug, info, error) | ✅ Sí |
| `UtcClock` | Time management, NTP sync | ✅ Sí |

**Función**: Utilidades helper sin lógica de negocio.

---

## 📂 Configuración Global

**Ubicación**: `include/`

| Archivo | Contenido | Compartido |
|---------|-----------|------------|
| `config.h` | GPIO pins, hardware defines | ✅ Sí |
| `secrets.h` | AWS credentials, WiFi (gitignored) | ✅ Sí |

**Por qué `include/`**: Es el directorio estándar de PlatformIO para headers globales compartidos por todas las apps.

---

## 🔄 Reglas de Dependencia (Clean Architecture)

```
Application Layer
    ↓ (puede usar)
Services Layer
    ↓ (puede usar)
Drivers Layer
    ↓ (puede usar)
Utils Layer

❌ PROHIBIDO: Drivers → Application
❌ PROHIBIDO: Services → Application
✅ PERMITIDO: Application → Services → Drivers → Utils
```

---

## 🎯 Ventajas de Esta Arquitectura

### ✅ Estándar PlatformIO
- `lib/` es donde PlatformIO busca automáticamente librerías compartidas
- `include/` es el directorio estándar para headers globales
- Cada app tiene su propio `platformio.ini` independiente
- No necesita configuración compleja de `lib_extra_dirs`

### ✅ Escalabilidad
- Añadir nueva app: Solo crear carpeta en `apps/` con su `platformio.ini`
- Todas las apps comparten automáticamente código en `lib/`
- Drivers específicos quedan aislados en cada app

### ✅ Claridad
- **Application**: Coordinación (qué hacer)
- **Services**: Lógica de negocio (cómo hacerlo)
- **Drivers**: Hardware (interacción física)
- **Utils**: Herramientas (logging, time)

### ✅ Mantenibilidad
- Cambio en servicio compartido → Afecta todas las apps automáticamente
- Cambio en driver específico → Solo afecta su app
- Configs globales en un solo lugar (`include/`)

---

## 🔧 Configuración PlatformIO

Cada app tiene su `platformio.ini` apuntando a librerías compartidas:

```ini
# apps/recirculator/platformio.ini
[env:esp32_c3_recirculator]
platform = espressif32
board = seeed_xiao_esp32c3
framework = arduino

# PlatformIO busca automáticamente en:
# - lib/ (raíz del workspace)
# - apps/recirculator/lib/ (local a la app)

# Include global headers
build_flags = 
    -I../../include

lib_deps = 
    # Dependencias externas
    adafruit/Adafruit NeoPixel
    knolleary/PubSubClient
    # ... etc
```

**Nota**: No necesitamos `lib_extra_dirs` porque PlatformIO busca en `lib/` automáticamente desde la raíz del workspace.

---

## 📋 Plan de Migración

### Paso 1: Mover Servicios Compartidos
```bash
src/services/* → lib/services/
```

### Paso 2: Mover Drivers Compartidos
```bash
src/drivers/button_manager → lib/drivers/button_manager
src/drivers/led_manager → lib/drivers/led_manager
```

### Paso 3: Mover Coordinación (a la app, NO compartido)
```bash
src/application/system_state → apps/recirculator/src/
```

**Nota importante**: `system_state` NO es compartido porque coordina módulos específicos de cada app.

### Paso 4: Mover Drivers Específicos
```bash
src/drivers/relay_controller → apps/recirculator/src/
src/drivers/temperature_sensor → apps/recirculator/src/
src/drivers/displayManager → apps/recirculator/src/
```

### Paso 5: Mover Entry Point
```bash
src/application/main.cpp → apps/recirculator/src/
```

### Paso 6: Mover Configs Globales
```bash
src/config.h → include/
src/secrets.h → include/
```

### Paso 7: Mover Utilidades (ya están bien)
```bash
lib/Log → lib/utils/Log
lib/UtcClock → lib/utils/UtcClock
```

### Paso 8: Limpiar
```bash
rm -rf src/  # Eliminar src/ raíz (ya no necesario)
rm -rf libs/ # Eliminar libs/ (confusión, usamos lib/)
```

---

## 🚀 Próximos Pasos

1. ✅ **Documentación actualizada** (este archivo)
2. ⏭️ **Mover archivos** según plan de migración
3. ⏭️ **Actualizar platformio.ini** en `apps/recirculator/`
4. ⏭️ **Compilar y probar** que todo funciona
5. ⏭️ **Actualizar architecture.md** con estructura final
6. ⏭️ **Commit** con arquitectura definitiva

---

**Fecha**: 28 Noviembre 2025  
**Arquitectura por**: gaesca04 (ingeniero informático)  
**Estado**: ✅ APROBADA - Lista para implementar
