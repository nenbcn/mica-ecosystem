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
 * @return float temperature in Celsius
 */
float getLatestTemperature();

#endif // TEMPERATURE_SENSOR_H