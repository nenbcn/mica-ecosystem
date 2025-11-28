// button_manager.cpp
// Button Manager Module
// Purpose: Generic button handler with debouncing and long-press detection
// Architecture: FreeRTOS task polls button state, sends events to system_state
// Thread-Safety: ISR-safe, uses task notifications for event communication
// Dependencies: system_state (for event notifications only)

#include "button_manager.h"

#include "config.h"
#include "system_state.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Log.h>

// Constants
const unsigned long LONG_PRESS_TIME = 5000;  // Time to consider a long press (ms)
const unsigned long DEBOUNCE_TIME = 50;      // Minimum time to avoid bounces (ms)

// Global Variables
static unsigned long buttonPressStart = 0; // Timestamp for long press detection
static unsigned long lastButtonCheck = 0;  // Timestamp for debounce

// Interrupt Service Routine (ISR) for button events
// NOTE: Minimal ISR - only updates timestamp
void IRAM_ATTR buttonISR() {
    // Just set a timestamp - actual handling in buttonTask
    buttonPressStart = millis();
}


// ========================================================
// Initialize Button Manager
void initializeButtonManager() {
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Configurar el botón como entrada con pull-down
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, CHANGE); // Asociar la ISR al pin del botón para eventos de cambio
    Log::info("Button Manager initialized. Waiting for button events.");
}

// Button Task
void buttonTask(void *pvParameters) {
    bool longPressSent = false; // Avoid multiple long press notifications
    unsigned long pressStartTime = 0;

    while (true) {
        unsigned long currentMillis = millis();

        // Read button state
        int buttonState = digitalRead(BUTTON_PIN);

        // Button is pressed (LOW due to pull-up)
        if (buttonState == LOW) {
            if (pressStartTime == 0) {
                pressStartTime = currentMillis; // Save press start time
                longPressSent = false; // Reset long press detection
                Log::debug("Button press detected. Waiting to verify long press...");
            }

            // Check if long press time has passed and hasn't been sent yet
            if (!longPressSent && (currentMillis - pressStartTime >= LONG_PRESS_TIME)) {
                Log::info("Long button press detected (5 seconds).");
                notifySystemState(EVENT_LONG_PRESS_BUTTON);
                longPressSent = true; // Avoid repeated sends
            }
        } else { // Button released
            if (pressStartTime != 0 && !longPressSent) {
                // Short press detected - notify system
                Log::info("Short button press detected.");
                notifySystemState(EVENT_SHORT_PRESS_BUTTON);
            }
            pressStartTime = 0; // Reset press time
            longPressSent = false; // Reset for next press
        }
        
        vTaskDelay(pdMS_TO_TICKS(50)); // Small delay to avoid excessive CPU usage
    }
}