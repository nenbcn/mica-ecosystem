#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

// Temperature Sensor Module
// Purpose:
// Handles the reading and processing of data from the temperature sensor.

/**
 * @brief Initializes the temperature sensor (configura el pin, crea la cua, etc).
 * @return true if initialization is successful, false otherwise.
 */
bool initializeTemperatureSensor();

/**
 * @brief FreeRTOS task to process temperature readings.
 * - Reads temperature values at regular intervals.
 * - Buffers and sends temperature data to AWS IoT via MQTT.
 */
void temperatureSensorTask(void *pvParameters);

/**
 * @brief Get the latest temperature value (thread-safe).
 * @return float Temperature in Celsius, or -127.0 if sensor error
 * @note Thread-safe: Uses temperatureMutex for protected access
 * @note Triggers buzzer alarm on first 3 sensor errors (max 1 per minute)
 * @warning -127.0 indicates DS18B20 sensor disconnected or failed
 */
float getLatestTemperature();

#endif // TEMPERATURE_SENSOR_H