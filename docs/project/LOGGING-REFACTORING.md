# Plan de Refactorización de Logs

> **Basado en**: `LOGGING-RULES.md`  
> **Objetivo**: Reducir spam de logs de ~144/min a ~15/min (90% reducción)

**Fecha**: 29 Noviembre 2025

---

## 📋 Cambios por Archivo

### 🔴 PRIORIDAD ALTA

#### 1. `lib/services/mqtt_handler/mqtt_handler.cpp`

**Problema**: Log en cada publish exitoso genera spam

**Cambios**:

```cpp
// LÍNEA ~381 - mqttPublishTask()
// ANTES:
if (published) {
    Log::debug("Published to %s: %s", msg.topic, msg.payload);
}

// DESPUÉS:
if (published) {
    Log::debug("Published to %s", msg.topic);  // Sin payload, más corto
}

// LÍNEA ~385 - Mantener error log
else {
    Log::error("Failed to publish to %s. MQTT State: %d", msg.topic, mqttClient.state());
}
```

```cpp
// LÍNEA ~461 - mqttPublish()
// ANTES:
Log::debug("Enqueued MQTT message to %s", topic);

// DESPUÉS: Eliminar (demasiado frecuente, incluso en debug)
// Solo mantener error de queue full (línea 466)
```

**Resultado**: Reducción de ~12 logs/min a 0 logs/min (solo errores)

---

#### 2. `lib/services/eeprom_config/eeprom_config.cpp`

**Status**: ✅ No requiere cambios

- Logs de WiFi credentials: Mantenidos (no hay problema de seguridad en este contexto)
- Logs de max temperature/time: Mantenidos (parámetros de configuración)

---

#### 3. `apps/recirculator/src/drivers/relay_controller.cpp`

**Cambios en MQTT Handlers**:

```cpp
// LÍNEA ~38-44 - handleMaxTemperatureCommand()
// MANTENER logs actuales - Parametrización de usuario (poco frecuente)
Log::info("Temperature %.2f received and saved from MQTT.", temp);
Log::error("Failed to save temperature from MQTT.");
// NO CAMBIAR - Son configuraciones importantes del usuario
```

```cpp
// LÍNEA ~53-59 - handleMaxTimeCommand()
// MANTENER logs actuales - Parametrización de usuario (poco frecuente)
Log::info("Max time %lu seconds received and saved from MQTT.", maxTime);
Log::error("Failed to save max time from MQTT.");
// NO CAMBIAR - Son configuraciones importantes del usuario
```

**Cambios en Buzzer Test** (reducir verbosidad en startup):

**Cambios**:

```cpp
// LÍNEA ~236-267 - testBuzzer()
// ANTES:
void testBuzzer() {
    Log::info("Testing buzzer on GPIO %d...", BUZZER_PIN);
    Log::info("Test 1: Digital toggle...");
    // ...
    Log::info("Test 2: Low tone 100Hz MAX POWER...");
    // ...
    Log::info("Test 3: Mid tone 1000Hz MAX POWER...");
    // ...
    Log::info("Test 4: High tone 2000Hz MAX POWER...");
    // ...
    Log::info("Test 5: Very high tone 4000Hz MAX POWER...");
    Log::info("Buzzer test completed. Did you hear anything?");
}

// DESPUÉS:
void testBuzzer() {
    Log::info("Buzzer test started on GPIO %d", BUZZER_PIN);
    Log::debug("Test 1: Digital toggle");
    // ... código ...
    Log::debug("Test 2: Low tone 100Hz");
    // ... código ...
    Log::debug("Test 3: Mid tone 1000Hz");
    // ... código ...
    Log::debug("Test 4: High tone 2000Hz");
    // ... código ...
    Log::debug("Test 5: Very high tone 4000Hz");
    Log::info("Buzzer test completed");
}
```

```cpp
// LÍNEA ~276 - activateRelay()
// ANTES:
if (isRelayActive()) {
    Log::debug("Relay already ON, ignoring duplicate activation.");
    return false;
}

// DESPUÉS: Mantener como está (debug correcto)
```

```cpp
// LÍNEA ~307 - deactivateRelay()
// ANTES:
if (!isRelayActive()) {
    Log::debug("Relay already OFF, ignoring duplicate deactivation.");
    return false;
}

// DESPUÉS: Mantener como está (debug correcto)
```

**Resultado**: Reducción de 7 logs a 2 logs en startup

---

### 🟡 PRIORIDAD MEDIA

#### 4. `lib/services/wifi_config_mode/wifi_config_mode.cpp`

**Problema**: Muchos logs durante config mode

**Cambios**:

```cpp
// LÍNEA ~30
// ANTES:
Log::info("Entering initializeWiFiConfigMode()...");

// DESPUÉS:
Log::debug("Entering initializeWiFiConfigMode()");
```

```cpp
// LÍNEA ~35
// ANTES:
Log::info("Scanning available WiFi networks...");

// DESPUÉS:
Log::debug("Scanning WiFi networks");
```

```cpp
// LÍNEA ~55-56
// ANTES:
Log::info("Access Point started with SSID: %s", AP_SSID);
Log::info("IP Address: %s", WiFi.softAPIP().toString().c_str());

// DESPUÉS: Consolidar
Log::info("AP started: %s (IP: %s)", AP_SSID, WiFi.softAPIP().toString().c_str());
```

```cpp
// LÍNEA ~66
// ANTES:
Log::info("HTTP Request received at /");

// DESPUÉS:
Log::debug("HTTP request at /");
```

```cpp
// LÍNEA ~154-157
// ANTES:
Log::info("Starting Web Server...");
server.begin();
Log::info("Web Server started successfully.");
Log::info("Please, enter the following URL in your browser: http://192.168.4.1");

// DESPUÉS: Consolidar
Log::info("Web server started: http://192.168.4.1");
server.begin();
```

**Resultado**: Reducción de ~8 logs a 3 logs durante config mode

---

#### 5. `lib/drivers/led_manager/led_manager.cpp`

**Status**: ✅ Ya optimizado correctamente

- Initialization: `Log::info()` ✅
- State changes: `Log::debug()` ✅

**No requiere cambios**

---

#### 6. `lib/drivers/button_manager/button_manager.cpp`

**Status**: ✅ Ya optimizado correctamente

- Initialization: `Log::info()` ✅
- Button events: `Log::info()` ✅ (son eventos importantes)
- Wait state: `Log::debug()` ✅

**No requiere cambios**

---

### 🟢 PRIORIDAD BAJA

#### 7. `apps/recirculator/src/drivers/temperature_sensor.cpp`

**Status**: ✅ Ya optimizado correctamente

- Change detection: Implementado ✅
- Error logging once: Implementado ✅

**No requiere cambios**

---

#### 8. `lib/services/wifi_connect/wifi_connect.cpp`

**Status**: ✅ Correcto

- Connection loop: `Log::debug()` ✅
- Success/Fail: `Log::info()`/`Log::error()` ✅

**No requiere cambios**

---

#### 9. `apps/recirculator/src/system_state.cpp`

**Status**: ✅ Correcto

- Initialization: `Log::info()`/`Log::error()` ✅
- State transitions: `Log::info()` ✅

**No requiere cambios**

---

## 📊 Resumen de Cambios

### Por Prioridad

| Prioridad       | Archivos | Logs Eliminados | Logs Convertidos a Debug |
|-----------------|----------|-----------------|--------------------------|
| 🔴 Alta         | 2        | ~22 logs        | ~10 logs                 |
| 🟡 Media        | 1        | ~5 logs         | ~5 logs                  |
| 🟢 Baja         | 6        | 0 logs          | 0 logs (ya optimizados)  |
| **Total**       | **9**    | **~27 logs**    | **~15 logs**             |

### Impacto Esperado

**Antes** (logs por minuto):
```
mqtt_handler publish:     12 logs/min
relay buzzer test:         7 logs/startup
wifi config mode:          8 logs/config
Total frecuente:         ~27 logs/min + events
```

**Después** (logs por minuto):
```
mqtt_handler:              0 logs/min (solo errores)
relay buzzer:              2 logs/startup
wifi config:               3 logs/config
Total frecuente:          ~5 logs/min + events
```

**Reducción total**: ~80% en operación normal

**✅ Logs mantenidos sin cambios**: 
- EEPROM credentials, temperature, time (configuración importante)
- State transitions (eventos del sistema)
- Error logs (troubleshooting)

---

## ✅ Checklist de Implementación

### Fase 1 - Cambios de Optimización (30 min)
- [ ] `mqtt_handler.cpp` - Eliminar logs de publish success
- [ ] `relay_controller.cpp` - Cambiar buzzer test logs a debug
- [ ] `wifi_config_mode.cpp` - Reducir logs de config mode

### Fase 3 - Testing (30 min)
- [ ] Compilar código
- [ ] Upload a hardware
- [ ] Ejecutar sistema 5 minutos
- [ ] Contar logs por categoría
- [ ] Verificar logs de configuración presentes
- [ ] Verificar que no hay spam
- [ ] Validar que errores siguen logueándose

### Fase 4 - Documentación (10 min)
- [ ] Actualizar CHANGELOG.md con nota sobre EEPROM
- [ ] Commit con mensaje descriptivo
- [ ] Push a GitHub

---

## 🎯 Criterios de Aceptación

1. ✅ Logs en operación normal < 20 logs/min
2. ✅ Logs de configuración presentes y funcionando
3. ✅ Logs de errores mantienen contexto completo
4. ✅ No hay logs repetitivos (< 5s entre logs iguales)
5. ✅ Startup logs < 15 líneas
6. ✅ Config mode logs < 5 líneas
7. ✅ Todos los cambios compilan sin errores
8. ✅ Funcionalidad 100% preservada

---

## 📝 Mensaje de Commit

```
refactor(logs): Optimize logging system to reduce spam

Optimization changes:
- mqtt_handler: Remove publish success logs (now debug only)
- relay_controller: Change buzzer test logs to debug level
- wifi_config_mode: Consolidate and reduce config mode logs

Impact:
- Reduced from ~27 logs/min to ~5 logs/min (80% reduction)
- Maintained all error logging with full context
- Kept configuration logs unchanged (EEPROM, state transitions)
- Debug logs still available for development
- Improved serial monitor readability

Based on: docs/project/LOGGING-RULES.md
Recommended by: gaesca04 (software architecture)
```

---

**Próximo paso**: Revisar código juntos y aplicar cambios
