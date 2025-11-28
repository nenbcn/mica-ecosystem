// temperature_sensor.cpp
// Temperature Sensor Module
// Purpose: Reads DS18B20 temperature sensor and publishes telemetry via MQTT
// Architecture: FreeRTOS task with mutex-protected state, publishes every 5 seconds
// Thread-Safety: Uses temperatureMutex for thread-safe temperature access
// Dependencies: DallasTemperature, OneWire, mqtt_handler, system_state

#include "temperature_sensor.h"

// Project headers (alphabetically)
#include "config.h"
#include "device_id.h"
#include "mqtt_handler.h"
#include "system_state.h"

// Third-party libraries
#include <Arduino.h>
#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <Log.h>
#include <OneWire.h>

// System headers
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <stdint.h>

const uint32_t TEMPERATURE_READ_INTERVAL = 5000; // ms - read every 5 seconds

static OneWire oneWire(TEMPERATURE_SENSOR_PIN);
static DallasTemperature sensors(&oneWire);
static float latestTemperature = 0.0f;
static SemaphoreHandle_t temperatureMutex = NULL;

bool initializeTemperatureSensor()
{
    sensors.begin();
    Log::info("Found %d DS18B20 devices", sensors.getDeviceCount());
    temperatureMutex = xSemaphoreCreateMutex();
    if (temperatureMutex == NULL)
    {
        Log::error("Failed to create temperature mutex.");
        for (int i = 0; i <= 3; i++)
        {
            digitalWrite(BUZZER_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(1000));
            digitalWrite(BUZZER_PIN, LOW);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        return false;
    }
    Log::info("DS18B20 temperature sensor initialized on pin %d.", TEMPERATURE_SENSOR_PIN);
    return true;
}

void temperatureSensorTask(void *pvParameters)
{
    static float lastLoggedTemp = -999.0f;
    const float TEMP_CHANGE_THRESHOLD = 0.5f;

    while (true)
    {
        sensors.requestTemperatures();
        float temp = sensors.getTempCByIndex(0);
        if (xSemaphoreTake(temperatureMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            latestTemperature = temp;
            xSemaphoreGive(temperatureMutex);
        }

        if (temp == -127.0f)
        {
            // Sensor error - log it prominently
            if (lastLoggedTemp != -127.0f)
            {
                Log::error("Temperature sensor ERROR: -127°C (sensor disconnected or failed)");
                lastLoggedTemp = temp;
            }
        }
        else if (abs(temp - lastLoggedTemp) >= TEMP_CHANGE_THRESHOLD)
        {
            Log::info("Temperature: %.2f°C", temp);
            lastLoggedTemp = temp;
        }

        // Publish temperature via MQTT if connected (including errors to notify backend)
        SystemState currentState = getSystemState();
        if (currentState == SYSTEM_STATE_CONNECTED_MQTT)
        {
            // Construct topic: mica/dev/telemetry/recirculator/{deviceId}/temperature
            char topic[128];
            snprintf(topic, sizeof(topic), "mica/dev/telemetry/recirculator/%s/temperature", getDeviceId());
            
            // Construct JSON payload
            DynamicJsonDocument doc(128);
            doc["deviceId"] = getDeviceId();
            doc["temperature"] = temp;
            doc["uptime"] = millis();
            String jsonString;
            serializeJson(doc, jsonString);
            
            // Publish using generic MQTT function
            mqttPublish(topic, jsonString.c_str(), true); // retain = true
        }

        vTaskDelay(pdMS_TO_TICKS(TEMPERATURE_READ_INTERVAL));
    }
}

float getLatestTemperature()
{
    static uint32_t lastBuzzerErrorTime = 0;
    static int errorBuzzCount = 0;
    float temp = 0.0f;

    // Read temperature in a thread-safe manner
    if (xSemaphoreTake(temperatureMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        temp = latestTemperature;
        if (temp == -127.0f)
        {
            uint32_t now = millis();
            if (errorBuzzCount < 3 && (now - lastBuzzerErrorTime > 60000 || lastBuzzerErrorTime == 0))
            {
                digitalWrite(BUZZER_PIN, HIGH);
                vTaskDelay(pdMS_TO_TICKS(1000));
                digitalWrite(BUZZER_PIN, LOW);
                errorBuzzCount++;
                lastBuzzerErrorTime = now;
            }
        }
        else if (errorBuzzCount > 0 && (millis() - lastBuzzerErrorTime > 300000))
        {
            // If 5 minutes have passed without error, reset the counter
            errorBuzzCount = 0;
            lastBuzzerErrorTime = 0;
        }
        xSemaphoreGive(temperatureMutex);
    }

    return temp;
}