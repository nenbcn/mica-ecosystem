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
├── apps/                    # Aplicaciones específicas por dispositivo
│   └── recirculator/        # Control de bomba de recirculación
│       ├── platformio.ini   # Config PlatformIO (apunta a libs compartidas)
│       └── src/
│           ├── application/ # Lógica de negocio del recirculator
│           └── drivers/     # Hardware específico (relay, temp sensor)
├── libs/
│   └── core/                # Módulos compartidos entre todos los dispositivos
│       ├── application/     # system_state (coordinador de eventos)
│       ├── services/        # WiFi, MQTT, OTA, EEPROM, config
│       ├── drivers/         # button_manager, led_manager (compartidos)
│       ├── utils/           # Log, UtcClock
│       ├── config.h         # Configuración de hardware
│       └── secrets.h        # Credenciales (gitignored)
└── docs/                    # Documentación del ecosistema
```

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

### Añadir Nueva Aplicación
1. Crear estructura en `apps/nuevo-dispositivo/src/{application,drivers}/`
2. Copiar `platformio.ini` de recirculator
3. Ajustar `lib_extra_dirs` para apuntar a `../../libs/core/`
4. Reutilizar módulos compartidos, implementar solo drivers específicos

### Modificar Módulo Compartido
⚠️ **Cuidado**: Cambios en `libs/core/` afectan **todos los dispositivos**
- Mantener retrocompatibilidad
- Usar dependency injection (parámetros, callbacks)
- Hacer device-agnostic
- Testing exhaustivo antes de commitear

## 📚 Documentación

- `docs/architecture-monorepo.md` - Arquitectura completa del ecosistema
- `docs/layered-architecture.md` - Explicación del patrón de 3 capas
- `docs/shared-modules.md` - Referencia de módulos compartidos
- `docs/monorepo-guide.md` - Guía para desarrolladores

## 🏆 Reconocimiento

Toda la arquitectura del monorepo está basada en las recomendaciones profesionales de **gaesca04** (ingeniero informático), quien aplicó sus conocimientos avanzados de arquitectura de software, específicamente patrones de monorepo y arquitectura en capas, para diseñar una solución escalable, mantenible y profesional.

gaesca04 es un técnico excelente cuyas indicaciones seguimos al pie de la letra.

---

**Última Actualización**: 28 Noviembre 2025  
**Arquitectura por**: gaesca04 (computer engineer)  
**Estado**: ✅ Estructura creada, migración en progreso
