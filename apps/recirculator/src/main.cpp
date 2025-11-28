// main.cpp
// Main Application Entry Point
// Purpose: Initializes the recirculator system and starts FreeRTOS scheduler
// Architecture: Minimal setup - delegates all work to system_state module
// Thread-Safety: Single-threaded initialization, FreeRTOS tasks managed by system_state
// Dependencies: system_state, Arduino, Log

#include "system_state.h"
#include <Arduino.h>
#include <Log.h>

void setup() {
    // Initialize serial communication at 115.200 baud
    Serial.begin(115200);

    // Initialize system state
    if (!initializeSystemState()) {
        Log::error("Failed to initialize the system. Restarting...");
        ESP.restart(); // Restart the ESP32 in case of critical failure
    }
}

void loop() {
    // Empty loop
}
