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
 * @brief Activa el relay físicamente y publica el estado por MQTT.
 * Esta es la única función que debe usarse para encender el relay.
 * @return true si se activó correctamente
 */
bool activateRelay();

/**
 * @brief Desactiva el relay físicamente y publica el estado por MQTT.
 * Esta es la única función que debe usarse para apagar el relay.
 * @param reason Razón del apagado (para logging): "manual", "timeout", "temperature"
 * @return true si se desactivó correctamente
 */
bool deactivateRelay(const char* reason);

/**
 * @brief Consulta si el relay está actualmente activo.
 * @return true si el relay está encendido, false si está apagado
 */
bool isRelayActive();

#endif