// ota_manager.h

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

/**
 * @brief Initializes the OTA Manager.
 *
 * This function sets up the secure client for HTTPS OTA updates using certificate
 * fingerprint verification.
 */
void initializeOTAManager();

/**
 * @brief FreeRTOS task for OTA update process.
 * @param pvParameters Task parameters (not used)
 * 
 * This task triggers the OTA update and handles the result.
 * On success, device reboots automatically.
 * On failure or no update available, returns to CONNECTED_MQTT state.
 */
void otaTask(void *pvParameters);

/**
 * @brief Triggers the OTA update process.
 *
 * This function performs an HTTPS-based OTA update using the HTTPUpdate library.
 * It handles errors robustly and logs the outcome. On success, the device will reboot.
 */
void triggerOTAUpdate();

#endif