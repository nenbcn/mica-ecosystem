#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

// Display Manager Module
// Purpose:
// Handles the initialization and management of the device display.

/**
 * @brief Initializes the display (sets up hardware, clears screen, etc).
 * @return true if initialization is successful, false otherwise.
 */
bool initializeDisplayManager();

/**
 * @brief FreeRTOS task to manage display updates.
 * - Updates display at regular intervals or on demand.
 * - Shows relevant system or telemetry data.
 */
void displayManagerTask(void *pvParameters);

#endif // DISPLAY_MANAGER_H