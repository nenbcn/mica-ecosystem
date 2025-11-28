// relay_controller.cpp
// Relay Controller Module
// Purpose: Centralized relay state management with safety timers and MQTT integration
// Architecture: FreeRTOS task monitors temperature/time limits, MQTT callbacks handle commands
// Thread-Safety: Single source of truth for relay state (isRelayPhysicallyOn)
// Dependencies: mqtt_handler, temperature_sensor, eeprom_config, system_state

#include "relay_controller.h"

// Project headers (alphabetically)
#include "config.h"
#include "device_id.h"
#include "eeprom_config.h"
#include "mqtt_handler.h"
#include "system_state.h"
#include "temperature_sensor.h"

// Third-party libraries
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Log.h>

// System headers
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

// Static variables for relay state management
static bool isRelayPhysicallyOn = false;

//------------------------------------------------------------------------------
// MQTT Command Handlers - Called when messages arrive on subscribed topics
//------------------------------------------------------------------------------

static void handleMaxTemperatureCommand(const char* topic, const char* payload, unsigned int length)
{
    float temp = atof(payload);
    if (saveMaxTemperature(temp))
    {
        Log::info("Temperature %.2f received and saved from MQTT.", temp);
    }
    else
    {
        Log::error("Failed to save temperature from MQTT.");
    }
}

static void handleMaxTimeCommand(const char* topic, const char* payload, unsigned int length)
{
    uint32_t maxTime = (uint32_t)atol(payload);
    if (maxTime > 0 && maxTime <= 3600)
    {
        if (saveMaxTime(maxTime))
        {
            Log::info("Max time %lu seconds received and saved from MQTT.", maxTime);
        }
        else
        {
            Log::error("Failed to save max time from MQTT.");
        }
    }
    else
    {
        Log::error("Invalid max time received via MQTT: %lu (must be 1-3600 seconds)", maxTime);
    }
}

static void handlePowerStateCommand(const char* topic, const char* payload, unsigned int length)
{
    if (strcmp(payload, "ON") == 0)
    {
        notifySystemState(EVENT_RELAY_ON);
        Log::info("Power state set to ON via MQTT");
    }
    else if (strcmp(payload, "OFF") == 0)
    {
        notifySystemState(EVENT_RELAY_OFF);
        Log::info("Power state set to OFF via MQTT");
    }
    else
    {
        Log::error("Invalid power state received via MQTT: %s", payload);
    }
}

//------------------------------------------------------------------------------
// Relay Controller Initialization - Register MQTT subscriptions
//------------------------------------------------------------------------------

void initializeRelayController()
{
    // Construct command topics and register handlers
    char maxTempTopic[128];
    char maxTimeTopic[128];
    char powerStateTopic[128];
    
    snprintf(maxTempTopic, sizeof(maxTempTopic), 
             "mica/dev/command/recirculator/%s/max-temperature", getDeviceId());
    snprintf(maxTimeTopic, sizeof(maxTimeTopic), 
             "mica/dev/command/recirculator/%s/max-time", getDeviceId());
    snprintf(powerStateTopic, sizeof(powerStateTopic), 
             "mica/dev/command/recirculator/%s/power-state", getDeviceId());
    
    // Register MQTT subscriptions with callbacks
    mqttSubscribe(maxTempTopic, handleMaxTemperatureCommand);
    mqttSubscribe(maxTimeTopic, handleMaxTimeCommand);
    mqttSubscribe(powerStateTopic, handlePowerStateCommand);
    
    Log::info("Relay controller MQTT subscriptions registered");
}

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978

// Super Mario Bros - Success melody (when temperature reached)
int successMelody[] = {
  NOTE_E7, NOTE_E7, 0, NOTE_E7, 0, NOTE_C7, NOTE_E7, 0, NOTE_G7, 0, 0, 0,
  NOTE_G6, 0, 0, 0, NOTE_C7, 0, 0, NOTE_G6, 0, 0, NOTE_E6, 0, NOTE_A6, 0, NOTE_B6, 0, NOTE_AS6, NOTE_A6, 0
};
int successDurations[] = {
  125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125,
  125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125
};
int numSuccessNotes = sizeof(successMelody) / sizeof(successMelody[0]);

// Super Mario Bros - Game Over melody (when timeout)
int gameOverMelody[] = {
  NOTE_C5, NOTE_G4, NOTE_E4, NOTE_A4, NOTE_B4, NOTE_A4, NOTE_GS4, NOTE_AS4, NOTE_GS4,
  NOTE_G4, NOTE_D4, NOTE_E4
};
int gameOverDurations[] = {
  250, 250, 250, 250, 250, 250, 250, 250, 250,
  250, 250, 500
};
int numGameOverNotes = sizeof(gameOverMelody) / sizeof(gameOverMelody[0]);

// Custom tone function - just a wrapper to tone() with proper timing
void playToneMaxPower(int pin, int frequency, int duration) {
    tone(pin, frequency, duration);
    vTaskDelay(pdMS_TO_TICKS(duration));
    noTone(pin);
}

/**
 * @brief Test del buzzer al inicio para verificar funcionamiento.
 * Ejecuta 5 pruebas con diferentes frecuencias.
 */
void testBuzzer() {
    Log::info("Buzzer test started on GPIO %d", BUZZER_PIN);
    
    // Test 1: Simple HIGH/LOW toggle at 1kHz (should click if buzzer is connected)
    Log::debug("Test 1: Digital toggle");
    for (int i = 0; i < 10; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(500);
        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(500);
    }
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Test 2: Low frequency tone (100 Hz - should be audible) with MAX POWER
    Log::debug("Test 2: Low tone 100Hz");
    playToneMaxPower(BUZZER_PIN, 100, 500);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Test 3: Medium frequency (1000 Hz) with MAX POWER
    Log::debug("Test 3: Mid tone 1000Hz");
    playToneMaxPower(BUZZER_PIN, 1000, 500);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Test 4: High frequency (2000 Hz) with MAX POWER
    Log::debug("Test 4: High tone 2000Hz");
    playToneMaxPower(BUZZER_PIN, 2000, 500);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Test 5: Very high frequency (4000 Hz) with MAX POWER
    Log::debug("Test 5: Very high tone 4000Hz");
    playToneMaxPower(BUZZER_PIN, 4000, 500);
    
    Log::info("Buzzer test completed");
}

/**
 * @brief Activa el relay físicamente y publica el estado por MQTT.
 * Esta es la ÚNICA función centralizada para encender el relay.
 */
bool activateRelay() {
    if (isRelayPhysicallyOn) {
        Log::debug("Relay already ON, ignoring duplicate activation.");
        return true;
    }
    
    digitalWrite(RELAY_PIN, HIGH);
    isRelayPhysicallyOn = true;
    Log::info("Relay turned ON.");
    
    // Construct topic and payload for power state
    char topic[128];
    snprintf(topic, sizeof(topic), "mica/dev/telemetry/recirculator/%s/power-state", getDeviceId());
    
    DynamicJsonDocument doc(128);
    doc["deviceId"] = getDeviceId();
    doc["state"] = "ON";
    doc["timestamp"] = millis();
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Publish using generic MQTT function
    mqttPublish(topic, jsonString.c_str(), true); // retain = true
    
    return true;
}

/**
 * @brief Desactiva el relay físicamente y publica el estado por MQTT.
 * Esta es la ÚNICA función centralizada para apagar el relay.
 */
bool deactivateRelay(const char* reason) {
    if (!isRelayPhysicallyOn) {
        Log::debug("Relay already OFF, ignoring duplicate deactivation.");
        return true;
    }
    
    digitalWrite(RELAY_PIN, LOW);
    isRelayPhysicallyOn = false;
    Log::info("Relay turned OFF. Reason: %s", reason);
    
    // Construct topic and payload for power state
    char topic[128];
    snprintf(topic, sizeof(topic), "mica/dev/telemetry/recirculator/%s/power-state", getDeviceId());
    
    DynamicJsonDocument doc(128);
    doc["deviceId"] = getDeviceId();
    doc["state"] = "OFF";
    doc["timestamp"] = millis();
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Publish using generic MQTT function
    mqttPublish(topic, jsonString.c_str(), true); // retain = true
    
    return true;
}

/**
 * @brief Consulta si el relay está actualmente activo.
 */
bool isRelayActive() {
    return isRelayPhysicallyOn;
}

void relayControllerTask(void *pvParameters) {
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // Ensure relay starts OFF
    Log::info("Relay controller task started on pin %d.", RELAY_PIN);

    // Wait a bit for system to stabilize
    vTaskDelay(pdMS_TO_TICKS(500));

    #ifdef ESP32_C3
      // Initialize LEDC for buzzer
      Log::info("Initializing LEDC for buzzer on GPIO %d", BUZZER_PIN);
      // Use 8-bit resolution (standard for tone() function)
      int setupResult = ledcSetup(0, 2000, 8); // Channel 0, 2000 Hz, 8-bit resolution
      if (setupResult == 0) {
          Log::error("LEDC setup failed! GPIO %d may not support LEDC/PWM", BUZZER_PIN);
      } else {
          Log::info("LEDC setup OK with frequency: %d Hz", setupResult);
      }
      ledcAttachPin(BUZZER_PIN, 0);
      ledcWrite(0, 0); // Start with buzzer OFF
    #endif

    // Test buzzer at startup
    testBuzzer();

    // Default configuration constants
    constexpr uint32_t DEFAULT_MAX_TIME_SECONDS = 120; // Default: 2 minutes
    constexpr uint32_t STATUS_LOG_INTERVAL_SECONDS = 5; // Log status every 5 seconds
    
    TickType_t startTime = 0;
    bool timerStarted = false;
    uint32_t maxTimeSeconds = DEFAULT_MAX_TIME_SECONDS;
    TickType_t maxRunTime = pdMS_TO_TICKS(maxTimeSeconds * 1000);
    uint32_t lastLoggedSecond = 0; // Track last logged interval to avoid duplicate logs
    bool maxTempLoaded = false; // Flag to load max temp only once per relay activation
    float maxTemperature = 30.0f; // Default max temperature

    while (true) {
        bool relayState = isRelayActive();

        if (relayState) {
            // Relay is ON - start timer if not started
            if (!timerStarted) {
                startTime = xTaskGetTickCount();
                timerStarted = true;
                lastLoggedSecond = 0;
                maxTempLoaded = false; // Reset flag to load temp again
                maxTimeSeconds = getStoredMaxTime(); // Load from EEPROM
                maxRunTime = pdMS_TO_TICKS(maxTimeSeconds * 1000);
                activateRelay(); // Centralized function
                Log::info("Max time: %lu seconds", maxTimeSeconds);
            }

            float temp = getLatestTemperature();
            
            // Calculate elapsed and remaining time
            TickType_t elapsedTicks = xTaskGetTickCount() - startTime;
            uint32_t elapsedSeconds = elapsedTicks / pdMS_TO_TICKS(1000);
            uint32_t remainingSeconds = (maxTimeSeconds > elapsedSeconds) ? (maxTimeSeconds - elapsedSeconds) : 0;
            
            // Log and publish status every STATUS_LOG_INTERVAL_SECONDS
            uint32_t logInterval = elapsedSeconds / STATUS_LOG_INTERVAL_SECONDS; // Calculate which interval we're in
            if (logInterval > 0 && logInterval != lastLoggedSecond) {
                Log::info("Relay ON: %lu/%lu s | Remaining: %lu s | Temp: %.1f°C", 
                          elapsedSeconds, maxTimeSeconds, remainingSeconds, temp);
                
                // Publish relay timer to MQTT (only when relay is active)
                char timerTopic[128];
                snprintf(timerTopic, sizeof(timerTopic), "mica/dev/telemetry/recirculator/%s/relay-timer", getDeviceId());
                
                DynamicJsonDocument timerDoc(256);
                timerDoc["deviceId"] = getDeviceId();
                timerDoc["elapsed"] = elapsedSeconds;
                timerDoc["remaining"] = remainingSeconds;
                timerDoc["maxTime"] = maxTimeSeconds;
                timerDoc["timestamp"] = millis();
                String timerJson;
                serializeJson(timerDoc, timerJson);
                
                mqttPublish(timerTopic, timerJson.c_str(), false); // retain = false
                
                lastLoggedSecond = logInterval;
            }
            
            // Check timeout
            if (elapsedTicks >= maxRunTime) {
                deactivateRelay("timeout"); // Centralized function
                timerStarted = false;
                Log::info("Timeout reached after %lu seconds.", maxTimeSeconds);
                Log::info("Playing Game Over melody...");
                
                // Play Game Over melody (timeout - no success)
                for (int i = 0; i < 2; i++) {
                    Log::info("Game Over melody repetition %d/2", i + 1);
                    for (int j = 0; j < numGameOverNotes; j++) {
                        int duration = gameOverDurations[j];
                        playToneMaxPower(BUZZER_PIN, gameOverMelody[j], duration);
                        vTaskDelay(pdMS_TO_TICKS(50)); // Small pause between notes
                    }
                    if (i == 0) vTaskDelay(pdMS_TO_TICKS(500)); // Pause between repetitions
                }
                Log::info("Game Over melody completed.");
                
                notifySystemState(EVENT_RELAY_STOPPED);
                continue;
            }

            // Check max temperature (load only once per activation)
            if (!maxTempLoaded) {
                maxTemperature = getStoredMaxTemperature();
                if (isnan(maxTemperature)) {
                    Log::warn("No valid temperature found in EEPROM, using default 30°C");
                    maxTemperature = 30.0f;
                }
                maxTempLoaded = true;
                Log::info("Max temperature threshold: %.1f°C", maxTemperature);
            }
            
            if (temp > maxTemperature) {
                deactivateRelay("temperature"); // Centralized function
                timerStarted = false;
                Log::info("Target temperature %.2f°C reached.", temp);
                Log::info("Playing Success melody...");
                
                // Play Success melody (temperature reached - success!)
                for (int i = 0; i < 2; i++) {
                    Log::info("Success melody repetition %d/2", i + 1);
                    for (int j = 0; j < numSuccessNotes; j++) {
                        int duration = successDurations[j];
                        playToneMaxPower(BUZZER_PIN, successMelody[j], duration);
                        vTaskDelay(pdMS_TO_TICKS(30)); // Small pause between notes
                    }
                    if (i == 0) vTaskDelay(pdMS_TO_TICKS(500)); // Pause between repetitions
                }
                Log::info("Success melody completed.");
                
                notifySystemState(EVENT_RELAY_STOPPED);
                continue;
            }
        } else {
            // Relay is OFF
            if (timerStarted) {
                deactivateRelay("manual"); // Centralized function
                timerStarted = false;
                maxTempLoaded = false; // Reset for next activation
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
