// wifi_connect.cpp
// WiFi Connection Module
// Purpose: Manages WiFi station mode connection with auto-reconnect
// Architecture: FreeRTOS task monitors connection status, loads credentials from EEPROM
// Thread-Safety: Uses wifiMutex (currently unused but reserved)
// Dependencies: WiFi library, eeprom_config, system_state

#include "wifi_connect.h"

#include "eeprom_config.h"
#include "system_state.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <WiFi.h>
#include <Log.h>

// Internal Variables
static SemaphoreHandle_t wifiMutex = NULL;

// Initialize WiFi Connection
bool initializeWiFiConnection() {
    WiFi.mode(WIFI_STA);
    //WiFi.setSleep(true); // Optional power saving
    wifiMutex = xSemaphoreCreateMutex();
    if (wifiMutex == NULL) {
        Log::error("Failed to create WiFi mutex.");
        return false;
    }
    Log::info("WiFi hardware initialized in station mode.");
    return true;
}

// WiFi Connect Task
void wifiConnectTask(void *pvParameters) {
    while (true) {
        // Check if already connected
        if (WiFi.status() == WL_CONNECTED) {
            notifySystemState(EVENT_WIFI_CONNECTED);
            vTaskDelay(pdMS_TO_TICKS(5000)); // Pause before checking again
            continue;
        }

        Log::warn("Wi-Fi disconnected. Attempting to reconnect...");

        String ssid, password;

        if (loadCredentials(ssid, password)) {
            if (ssid.isEmpty() || password.isEmpty()) {
                Log::warn("No Wi-Fi credentials found in EEPROM.");
                notifySystemState(EVENT_NO_PARAMETERS_EEPROM);
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }

            Log::info("Attempting to connect to SSID: %s", ssid.c_str());
            WiFi.disconnect(true);
            vTaskDelay(pdMS_TO_TICKS(100));
            WiFi.begin(ssid.c_str(), password.c_str());

            unsigned long startTime = millis();
            const unsigned long timeout = 15000;

            // Wait for connection
            while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeout) {
                wl_status_t status = WiFi.status();
                const char* statusStr = "UNKNOWN";
                switch(status) {
                    case WL_IDLE_STATUS: statusStr = "IDLE"; break;
                    case WL_NO_SSID_AVAIL: statusStr = "NO_SSID_AVAILABLE"; break;
                    case WL_SCAN_COMPLETED: statusStr = "SCAN_COMPLETED"; break;
                    case WL_CONNECTED: statusStr = "CONNECTED"; break;
                    case WL_CONNECT_FAILED: statusStr = "CONNECT_FAILED"; break;
                    case WL_CONNECTION_LOST: statusStr = "CONNECTION_LOST"; break;
                    case WL_DISCONNECTED: statusStr = "DISCONNECTED"; break;
                }
                Log::debug("Connecting... Status: %s (%d)", statusStr, status);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }

            // Check the result
            if (WiFi.status() == WL_CONNECTED) {
                Log::info("Connected to Wi-Fi! IP Address: %s", WiFi.localIP().toString().c_str());
                notifySystemState(EVENT_WIFI_CONNECTED);
            } else {
                Log::error("Failed to connect to Wi-Fi.");
                notifySystemState(EVENT_WIFI_FAIL_CONNECT);
            }
        } else {
            Log::warn("No credentials found in EEPROM.");
            notifySystemState(EVENT_NO_PARAMETERS_EEPROM); // TODO: Se usa la misma que en ssid y password empty, comprobar que se quiera hacer así
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Pause before next attempt
    }
}