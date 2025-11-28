#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

/**
 * @brief Initialize relay controller and register MQTT command subscriptions
 * Must be called after MQTT connection is established
 */
void initializeRelayController();

/**
 * @brief FreeRTOS task to control relay based on temperature.
 * - Monitors temperature and compares with max temperature from EEPROM.
 * - Automatically stops after 2 minutes or when max temperature is reached.
 * - Plays buzzer melodies on stop conditions.
 */
void relayControllerTask(void *pvParameters);

/**
 * @brief Activates the relay physically and publishes the state via MQTT.
 * 
 * This is the centralized function for turning on the relay. It:
 * - Sets GPIO pin HIGH
 * - Updates internal state flag
 * - Publishes power state to MQTT with retain=true
 * - Starts safety timer in relay controller task
 * 
 * @return true if activated successfully, false if already active
 * @note Thread-safe: Can be called from any task
 * @note Idempotent: Multiple calls ignored if already active
 */
bool activateRelay();

/**
 * @brief Deactivates the relay physically and publishes the state via MQTT.
 * 
 * This is the centralized function for turning off the relay. It:
 * - Sets GPIO pin LOW
 * - Updates internal state flag
 * - Publishes power state to MQTT with retain=true
 * - Logs the deactivation reason
 * 
 * @param reason Reason for shutdown (for logging): "manual", "timeout", "temperature", "command", "button"
 * @return true if deactivated successfully, false if already inactive
 * @note Thread-safe: Can be called from any task
 * @note Idempotent: Multiple calls ignored if already inactive
 */
bool deactivateRelay(const char* reason);

/**
 * @brief Checks if the relay is currently active.
 * @return true if relay is on, false if relay is off
 * @note Thread-safe: Reads static bool (atomic on single-core ESP32)
 * @note Use this instead of digitalRead(RELAY_PIN) for consistency
 */
bool isRelayActive();

#endif