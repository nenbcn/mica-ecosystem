# Reglas de Logging - MICA Ecosystem

> **Arquitectura por gaesca04** (ingeniero informático)  
> Optimización de logs para sistemas embebidos con recursos limitados

**Fecha**: 29 Noviembre 2025  
**Versión**: 1.0

---

## 🎯 Objetivos

1. **Reducir spam de logs** en tareas repetitivas
2. **Maximizar información útil** (cambios de estado, errores, eventos importantes)
3. **Optimizar uso de RAM** (buffer de logs limitado)
4. **Facilitar debugging** con logs significativos
5. **Consistencia** en formato y niveles

---

## 📊 Niveles de Log

### Definición de Niveles

```cpp
LOG_LEVEL_ERROR   // Errores críticos que requieren atención inmediata
LOG_LEVEL_WARNING // Problemas potenciales, no bloquean funcionamiento
LOG_LEVEL_INFO    // Eventos importantes del ciclo de vida del sistema
LOG_LEVEL_DEBUG   // Información detallada para debugging (solo desarrollo)
```

### Cuándo Usar Cada Nivel

#### ❌ ERROR (solo problemas graves)
```cpp
// ✅ SÍ - Error crítico que afecta funcionalidad
Log::error("Failed to create mutex");
Log::error("Temperature sensor disconnected");
Log::error("MQTT connection failed after 5 retries");
Log::error("EEPROM commit failed");

// ❌ NO - Estado esperado o recuperable
Log::error("WiFi disconnected");  // ← Es un estado transitorio, usar warning
Log::error("Button press detected");  // ← NO es error!
```

#### ⚠️ WARNING (problemas no críticos)
```cpp
// ✅ SÍ - Situaciones anormales pero recuperables
Log::warn("No WiFi credentials found, entering config mode");
Log::warn("MQTT disconnected, retrying in 5s");
Log::warn("Temperature reading out of expected range");

// ❌ NO - Logs informativos normales
Log::warn("System initialized");  // ← Usar info
```

#### ℹ️ INFO (eventos significativos)
```cpp
// ✅ SÍ - Cambios de estado importantes
Log::info("System initialized successfully");
Log::info("WiFi connected: IP %s", ip);
Log::info("MQTT connected");
Log::info("Relay turned ON");
Log::info("Entering config mode");

// ❌ NO - Eventos frecuentes o repetitivos
Log::info("Temperature: %.2f°C");  // ← Si se lee cada 5s, genera spam!
Log::info("WiFi status check");  // ← Demasiado frecuente
```

#### 🔍 DEBUG (solo desarrollo)
```cpp
// ✅ SÍ - Información de debugging detallada
Log::debug("Connecting... Status: %s (%d)", statusStr, status);
Log::debug("Published to %s: %s", topic, payload);
Log::debug("Button press detected, waiting for release");

// ⚠️ IMPORTANTE: En producción, cambiar LOG_LEVEL a INFO para eliminar debug logs
```

---

## 🚫 Anti-Patrones (Logs a EVITAR)

### 1. Logs en Loops Frecuentes

**❌ MAL - Spam cada ciclo**:
```cpp
void temperatureSensorTask() {
    while (true) {
        float temp = readSensor();
        Log::info("Temperature: %.2f°C", temp);  // ← Se ejecuta cada 5s = SPAM!
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

**✅ BIEN - Log solo en cambios significativos**:
```cpp
void temperatureSensorTask() {
    float lastLoggedTemp = -999.0f;
    const float TEMP_CHANGE_THRESHOLD = 0.5f;
    
    while (true) {
        float temp = readSensor();
        
        // Solo loguear si cambia significativamente
        if (abs(temp - lastLoggedTemp) >= TEMP_CHANGE_THRESHOLD) {
            Log::info("Temperature: %.2f°C", temp);
            lastLoggedTemp = temp;
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### 2. Logs en Estados Transitorios

**❌ MAL - Log cada segundo durante conexión**:
```cpp
while (WiFi.status() != WL_CONNECTED) {
    Log::info("Connecting to WiFi...");  // ← Se repite cada segundo
    vTaskDelay(pdMS_TO_TICKS(1000));
}
```

**✅ BIEN - Log al inicio y al completar**:
```cpp
Log::info("Connecting to WiFi: %s", ssid);
while (WiFi.status() != WL_CONNECTED) {
    Log::debug("WiFi status: %s", getStatusString(WiFi.status()));  // ← Solo en debug
    vTaskDelay(pdMS_TO_TICKS(1000));
}
Log::info("WiFi connected: IP %s", WiFi.localIP().toString().c_str());
```

### 3. Logs Redundantes en Success

**❌ MAL - Log de éxito innecesario en operaciones frecuentes**:
```cpp
bool result = saveSensorReading(value);
if (result) {
    Log::info("Reading saved successfully");  // ← Redundante si se hace cada 5s
}
```

**✅ BIEN - Log solo en error (success es esperado)**:
```cpp
bool result = saveSensorReading(value);
if (!result) {
    Log::error("Failed to save reading to storage");
}
// Si success, no hay log (es el comportamiento esperado)
```

**⚠️ EXCEPCIÓN IMPORTANTE - Escrituras EEPROM**:
```cpp
// EEPROM tiene ciclos limitados (~100,000 escrituras)
// SIEMPRE loguear escrituras para monitorear frecuencia y detectar abuso

bool result = saveMaxTemperature(temp);
if (result) {
    Log::info("Temperature %.2f saved to EEPROM", temp);  // ← MANTENER - CRÍTICO
} else {
    Log::error("Failed to save temperature to EEPROM");
}

// Razón: Necesitamos detectar si el backend envía comandos demasiado frecuentes
// Si vemos muchos logs de "saved to EEPROM", indica problema de diseño en backend
```

### 4. Logs en Callbacks Frecuentes

**❌ MAL - Log en cada MQTT publish**:
```cpp
void publishTelemetry() {
    mqttPublish(topic, payload);
    Log::info("Published to %s: %s", topic, payload);  // ← Si se publica cada 5s = SPAM
}
```

**✅ BIEN - Log solo en errores o con debug**:
```cpp
void publishTelemetry() {
    bool success = mqttPublish(topic, payload);
    if (!success) {
        Log::error("Failed to publish to %s", topic);
    } else {
        Log::debug("Published to %s", topic);  // ← Solo visible en debug mode
    }
}
```

---

## ✅ Patrones Recomendados

### 1. Log con Rate Limiting

**Para eventos frecuentes que necesitan monitoreo**:

```cpp
// Relay timer - log cada 5 segundos, no cada segundo
uint32_t lastLoggedSecond = 0;
const uint32_t LOG_INTERVAL_SECONDS = 5;

while (relayActive) {
    uint32_t elapsedSeconds = getElapsedSeconds();
    uint32_t logInterval = elapsedSeconds / LOG_INTERVAL_SECONDS;
    
    if (logInterval > 0 && logInterval != lastLoggedSecond) {
        Log::info("Relay ON: %lu/%lu s | Temp: %.1f°C", 
                  elapsedSeconds, maxTime, temperature);
        lastLoggedSecond = logInterval;
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000));
}
```

### 2. Log con Change Detection

**Para valores que cambian gradualmente**:

```cpp
float lastLoggedTemp = -999.0f;
bool sensorErrorLogged = false;

while (true) {
    float temp = readTemperature();
    
    // Error case - log solo una vez
    if (temp == -127.0f) {
        if (!sensorErrorLogged) {
            Log::error("Temperature sensor ERROR: -127°C (disconnected)");
            sensorErrorLogged = true;
        }
    }
    // Normal case - log solo si cambio significativo
    else {
        sensorErrorLogged = false;
        if (abs(temp - lastLoggedTemp) >= 0.5f) {
            Log::info("Temperature: %.2f°C", temp);
            lastLoggedTemp = temp;
        }
    }
    
    vTaskDelay(pdMS_TO_TICKS(5000));
}
```

### 3. Log con State Transitions

**Para máquinas de estado**:

```cpp
SystemState lastLoggedState = SYSTEM_STATE_UNKNOWN;

void stateManagerTask() {
    while (true) {
        SystemState currentState = getSystemState();
        
        // Log solo en cambios de estado
        if (currentState != lastLoggedState) {
            Log::info("State transition: %s → %s", 
                      getStateName(lastLoggedState), 
                      getStateName(currentState));
            lastLoggedState = currentState;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### 4. Log con Contexto en Errores

**Proporcionar información útil para debugging**:

```cpp
// ❌ MAL - Poco contexto
Log::error("Connection failed");

// ✅ BIEN - Contexto completo
Log::error("Failed to connect to MQTT broker %s:%d after %d attempts. Error: %d", 
           broker, port, retries, errorCode);
```

---

## 📏 Formato Estándar

### Mensajes de Inicialización

```cpp
// Módulo iniciado exitosamente
Log::info("Temperature sensor initialized on pin %d", SENSOR_PIN);
Log::info("MQTT Handler initialized for device type '%s'", deviceType);

// Error de inicialización
Log::error("Failed to initialize display at address 0x%02X", I2C_ADDR);
```

### Mensajes de Estado

```cpp
// Cambio de estado
Log::info("WiFi connected: IP %s", ip);
Log::info("Entering config mode");
Log::info("Relay turned %s. Reason: %s", state ? "ON" : "OFF", reason);

// Estado periódico (con rate limiting)
Log::info("Relay ON: %lu/%lu s | Remaining: %lu s | Temp: %.1f°C", 
          elapsed, maxTime, remaining, temp);
```

### Mensajes de Error

```cpp
// Error con contexto
Log::error("Failed to %s. Error: %d", action, errorCode);
Log::error("Temperature sensor ERROR: %.2f°C (sensor disconnected)", temp);

// Error de timeout
Log::error("Failed to connect to MQTT after %d attempts", maxRetries);
```

---

## ✅ CASO ESPECIAL: Parámetros de Configuración en EEPROM

### Por Qué Mantener Logs de Configuración

**Los parámetros de configuración son eventos importantes** que deben registrarse:

```cpp
// relay_controller.cpp - Usuario cambia configuración
void handleMaxTemperatureCommand(const char* payload) {
    float temp = atof(payload);
    saveMaxTemperature(temp);  // ← GRABA EN EEPROM
    Log::info("Temperature %.2f saved to EEPROM", temp);  // ← MANTENER
}
```

**Razón para mantener logs**:
- ✅ Son eventos **poco frecuentes** (días/semanas entre cambios)
- ✅ Configuraciones importantes del usuario
- ✅ Útiles para troubleshooting ("¿cuándo cambió la temperatura?")
- ✅ No generan spam ni desgaste de EEPROM

### Diferencia con Datos Frecuentes

**❌ MAL - Loguear lecturas frecuentes**:
```cpp
// Cada 5 segundos
void temperatureSensorTask() {
    float temp = readSensor();
    saveToStorage(temp);  // ← NO hacer esto frecuentemente
    Log::info("Temperature %.2f saved", temp);  // ← SPAM
}
```

**✅ BIEN - Loguear configuración**:
```cpp
// Una vez cada varios días
void handleMaxTemperatureCommand(const char* payload) {
    float maxTemp = atof(payload);
    saveMaxTemperature(maxTemp);  // ← OK, poco frecuente
    Log::info("Max temperature %.2f saved to EEPROM", maxTemp);  // ← OK
}
```

### Cuándo SÍ Implementar Protección

Solo si detectas en producción:
- Escrituras > 10/hora de configuración → Backend mal diseñado
- Solución: Rate limiting o change detection

Para uso normal de configuración (< 5 cambios/día):
- ✅ Logs actuales son correctos
- ✅ No requiere optimización

---

## 🔧 Optimización de Recursos

### Buffer de Logs (FreeRTOS Queue)

```cpp
// Configuración actual
QueueHandle_t g_logMessageQueue;
g_logMessageQueue = xQueueCreate(10, sizeof(LogMessage));  // 10 mensajes buffer

// Estructura de mensaje
typedef struct {
    LogLevel level;
    char message[128];  // 128 bytes por mensaje
} LogMessage;

// Total RAM: 10 * 128 = 1280 bytes (1.28 KB)
```

**Recomendaciones**:
- ✅ Mantener buffer pequeño (10 mensajes suficiente)
- ✅ Limitar tamaño de mensaje a 128 bytes
- ✅ No loguear payloads MQTT completos (usar primeros N caracteres)
- ✅ Usar `Log::debug()` para información detallada (se elimina en producción)

### Reducción de Spam

**Antes de optimización** (logs por minuto en operación normal):
```
Temperature sensor:  12 logs/min  (cada 5s)
WiFi status check:   60 logs/min  (cada 1s)
MQTT publish:        12 logs/min  (cada 5s)
Relay timer:         60 logs/min  (cada 1s)
Total:              ~144 logs/min = 2.4 logs/segundo
```

**Después de optimización**:
```
Temperature sensor:   2 logs/min  (solo cambios > 0.5°C)
WiFi status:          1 log/min   (solo cambios de estado)
MQTT publish:         0 logs/min  (solo errores)
Relay timer:         12 logs/min  (cada 5s, no cada 1s)
Total:              ~15 logs/min = 0.25 logs/segundo (90% reducción)
```

---

## 🎓 Checklist de Revisión

Antes de hacer commit, revisar cada `Log::` en el código:

- [ ] **¿Es un error real?** → Usar `Log::error()`
- [ ] **¿Ocurre cada ciclo del loop?** → Aplicar rate limiting o change detection
- [ ] **¿Es información de debugging?** → Usar `Log::debug()` en lugar de `info()`
- [ ] **¿Es un estado esperado?** → No loguear (ej: relay OFF es normal)
- [ ] **¿Tiene contexto suficiente?** → Incluir valores, IDs, códigos de error
- [ ] **¿Es redundante con otro log cercano?** → Consolidar en uno solo
- [ ] **¿Aporta valor para troubleshooting?** → Si no, eliminar

---

## 🔄 Plan de Refactorización

### Prioridad ALTA 🔴

1. **Temperature Sensor** (`temperature_sensor.cpp`)
   - ✅ Ya implementado: Change detection con threshold 0.5°C
   - ✅ Ya implementado: Error logging solo una vez
   - Estado: COMPLETO

2. **Relay Controller** (`relay_controller.cpp`)
   - ✅ Ya implementado: Timer logs cada 5s (no cada 1s)
   - ⚠️ Revisar: Logs de buzzer test (muchos durante startup)
   - ⚠️ Optimizar: Logs de melody playback (Game Over, Success)

3. **MQTT Handler** (`mqtt_handler.cpp`)
   - ⚠️ Cambiar: `Log::info()` por `Log::debug()` en publish success
   - ⚠️ Mantener: `Log::error()` solo en publish fail
   - ⚠️ Revisar: Health check logging (¿cada cuánto?)

### Prioridad MEDIA 🟡

4. **WiFi Connect** (`wifi_connect.cpp`)
   - ⚠️ Cambiar: Connecting loop de `Log::debug()` es correcto
   - ✅ Mantener: Connection success/fail en `info`/`error`

5. **EEPROM Config** (`eeprom_config.cpp`)
   - ⚠️ Eliminar: `Log::info()` en save/load exitosos (solo si error)
   - ⚠️ Mantener: Errors con contexto

6. **WiFi Config Mode** (`wifi_config_mode.cpp`)
   - ⚠️ Reducir: Muchos `Log::info()` durante config mode
   - ✅ Mantener: AP started, credentials saved

### Prioridad BAJA 🟢

7. **LED Manager** (`led_manager.cpp`)
   - ✅ Ya implementado: `Log::debug()` para cambios de LED (correcto)
   - ✅ Solo initialization en `Log::info()`

8. **Button Manager** (`button_manager.cpp`)
   - ✅ Ya implementado: `Log::info()` en long/short press (correcto)
   - ✅ `Log::debug()` durante wait (correcto)

9. **System State** (`system_state.cpp`)
   - ✅ Initialization logs correctos
   - ✅ State transitions en `Log::info()` correcto

---

## 📝 Ejemplos de Refactorización

### Ejemplo 1: MQTT Publish

**Antes**:
```cpp
bool published = mqttClient.publish(msg.topic, msg.payload, msg.retain);
if (published) {
    Log::info("Published to %s: %s", msg.topic, msg.payload);  // ← SPAM
} else {
    Log::error("Failed to publish to %s", msg.topic);
}
```

**Después**:
```cpp
bool published = mqttClient.publish(msg.topic, msg.payload, msg.retain);
if (published) {
    Log::debug("Published to %s", msg.topic);  // ← Solo en debug mode
} else {
    Log::error("Failed to publish to %s. MQTT State: %d", msg.topic, mqttClient.state());
}
```

### Ejemplo 2: EEPROM Save

**Antes**:
```cpp
if (result) {
    Log::info("Temperature %.2f saved to EEPROM.", temperature);
} else {
    Log::error("Failed to commit temperature to EEPROM.");
}
```

**Después**:
```cpp
if (!result) {
    Log::error("Failed to save temperature %.2f to EEPROM", temperature);
}
// Success es esperado, no requiere log
```

### Ejemplo 3: Buzzer Test

**Antes**:
```cpp
void testBuzzer() {
    Log::info("Testing buzzer on GPIO %d...", BUZZER_PIN);
    Log::info("Test 1: Digital toggle...");
    // ... código ...
    Log::info("Test 2: Low tone 100Hz MAX POWER...");
    // ... código ...
    Log::info("Test 3: Mid tone 1000Hz MAX POWER...");
    // ... 5 tests más ...
    Log::info("Buzzer test completed. Did you hear anything?");
}
```

**Después**:
```cpp
void testBuzzer() {
    Log::info("Buzzer test started on GPIO %d", BUZZER_PIN);
    Log::debug("Test 1: Digital toggle");
    // ... código ...
    Log::debug("Test 2: Low tone 100Hz");
    // ... código ...
    Log::debug("Test 3: Mid tone 1000Hz");
    // ... 5 tests más ...
    Log::info("Buzzer test completed");
}
```

---

## 🎯 Métricas de Éxito

### Objetivos Cuantificables

- **Reducción de logs**: De ~144 logs/min a ~15 logs/min (90% reducción)
- **Uso de RAM**: Mantener buffer en 10 mensajes (suficiente post-optimización)
- **Ratio debug/info**: 70% debug (desarrollo), 30% info/error (producción)
- **Logs en errores**: 100% coverage (todos los errores logueados con contexto)

### Revisión Post-Implementación

1. Ejecutar sistema en hardware durante 5 minutos
2. Contar logs por categoría (error/warning/info/debug)
3. Verificar que logs aportan información útil
4. Confirmar que no hay spam (repeticiones < 5s)
5. Validar que errores tienen contexto suficiente

---

**Aprobado por**: gaesca04 (ingeniero informático)  
**Implementación**: MICA Ecosystem Team  
**Próxima Revisión**: Post-refactorización de logs
