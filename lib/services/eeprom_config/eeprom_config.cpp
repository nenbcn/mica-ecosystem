// eeprom_config.cpp
// EEPROM Configuration Module
// Purpose: Persistent storage for WiFi credentials, temperature/time configuration
// Architecture: Mutex-protected EEPROM access with validation flags
// Thread-Safety: Uses eepromMutex for all read/write operations
// Dependencies: EEPROM library, FreeRTOS semaphores

#include "eeprom_config.h"

// Third-party libraries
#include <Arduino.h>
#include <EEPROM.h>
#include <Log.h>

// System headers
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdint.h>
#include <string.h>

// Mutex to protect EEPROM access
SemaphoreHandle_t eepromMutex = NULL;

// Initialize EEPROM
bool eepromInitialize() {
    if (!validateEEPROMSize()) {
        return false;
    }

    if (!EEPROM.begin(EEPROM_SIZE)) {
        Log::error("Failed to initialize EEPROM.");
        return false;
    }

    eepromMutex = xSemaphoreCreateMutex();
    if (eepromMutex == NULL) {
        Log::error("Failed to create EEPROM mutex.");
        return false;
    }

    Serial.println("[INFO] EEPROM initialized successfully."); // LogMessage still not initialized
    return true;
}

uint32_t getStoredMaxTime() {
    uint32_t maxTime = 0;
    if (!loadMaxTime(maxTime)) {
        return 120; // Default: 120 seconds (2 minutes)
    }
    return maxTime;
}

// Validate EEPROM Size
bool validateEEPROMSize() {
    int requiredSize = FLAG_ADDR + 1; // Last used address

    if (EEPROM_SIZE < requiredSize) {
        Log::error("EEPROM_SIZE (%d) is insufficient. Required: %d\n", EEPROM_SIZE, requiredSize);
        return false;
    }

    Log::info("EEPROM_SIZE (%d) is sufficient. Required: %d\n", EEPROM_SIZE, requiredSize);
    return true;
}

// Save Credentials
bool saveCredentials(const String &ssid, const String &password) {
    if (eepromMutex == NULL) {
        Log::error("EEPROM mutex not initialized.");
        return false;
    }

    if (xSemaphoreTake(eepromMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        Log::info("Attempting to save credentials: SSID=%s, Password=%s", ssid.c_str(), password.c_str());

        if (ssid.length() > MAX_CRED_LENGTH || password.length() > MAX_CRED_LENGTH) {
            Log::error("Credentials exceed maximum length.");
            xSemaphoreGive(eepromMutex);
            return false;
        }

        EEPROM.write(FLAG_ADDR, FLAG_VALID);
        for (int i = 0; i < MAX_CRED_LENGTH; ++i) {
            EEPROM.write(SSID_ADDR + i, (i < ssid.length()) ? ssid[i] : 0);
            EEPROM.write(PASS_ADDR + i, (i < password.length()) ? password[i] : 0);
        }

        if (EEPROM.commit()) {
            Log::info("Credentials saved successfully in EEPROM.");
            xSemaphoreGive(eepromMutex);
            return true;
        } else {
            Log::error("Failed to commit changes to EEPROM.");
            xSemaphoreGive(eepromMutex);
            return false;
        }
    } else {
        Log::error("Could not acquire EEPROM mutex.");
        return false;
    }
}

bool loadCredentials(String &ssid, String &password) {
    if (eepromMutex == NULL) {
        Log::error("EEPROM mutex not initialized.");
        return false;
    }

    if (xSemaphoreTake(eepromMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (EEPROM.read(FLAG_ADDR) != FLAG_VALID) {
            Log::warn("No valid credentials found in EEPROM.");
            xSemaphoreGive(eepromMutex);
            return false;
        }

        char ssidBuffer[MAX_CRED_LENGTH + 1];
        char passBuffer[MAX_CRED_LENGTH + 1];
        for (int i = 0; i < MAX_CRED_LENGTH; ++i) {
            ssidBuffer[i] = EEPROM.read(SSID_ADDR + i);
            passBuffer[i] = EEPROM.read(PASS_ADDR + i);
        }
        ssidBuffer[MAX_CRED_LENGTH] = '\0';
        passBuffer[MAX_CRED_LENGTH] = '\0';

        ssid = String(ssidBuffer);
        password = String(passBuffer);

        Log::info("Loaded credentials: SSID=%s, Password=%s", ssid.c_str(), password.c_str());
        xSemaphoreGive(eepromMutex);
        return true;
    } else {
        Log::error("Could not acquire EEPROM mutex.");
        return false;
    }
}

// Clear Credentials
void clearCredentials() {
    if (eepromMutex == NULL) {
        Log::error("EEPROM mutex not initialized.");
        return;
    }

    if (xSemaphoreTake(eepromMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        EEPROM.write(FLAG_ADDR, 0xFF);
        for (int i = 0; i < MAX_CRED_LENGTH; ++i) {
            EEPROM.write(SSID_ADDR + i, 0);
            EEPROM.write(PASS_ADDR + i, 0);
        }
        EEPROM.commit();
        Log::info("Credentials cleared in EEPROM.");
        xSemaphoreGive(eepromMutex);
    } else {
        Log::error("Could not acquire EEPROM mutex.");
    }
}

// Print EEPROM Contents
void printEEPROMContents() {
    if (eepromMutex == NULL) {
        Log::error("EEPROM mutex not initialized.");
        return;
    }

    if (xSemaphoreTake(eepromMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        Log::info("EEPROM Contents:");
        Log::info("  FLAG_ADDR: %02X", EEPROM.read(FLAG_ADDR));

        char ssidBuffer[MAX_CRED_LENGTH + 1];
        for (int i = 0; i < MAX_CRED_LENGTH; ++i) {
            char c = EEPROM.read(SSID_ADDR + i);
            if (c == 0) break;
            ssidBuffer[i] = c;
        }
        ssidBuffer[MAX_CRED_LENGTH] = '\0';
        Log::info("  SSID: %s", ssidBuffer);

        char passBuffer[MAX_CRED_LENGTH + 1];
        for (int i = 0; i < MAX_CRED_LENGTH; ++i) {
            char c = EEPROM.read(PASS_ADDR + i);
            if (c == 0) break;
            passBuffer[i] = c;
        }
        passBuffer[MAX_CRED_LENGTH] = '\0';
        Log::info("\nPassword: %s", passBuffer);

        xSemaphoreGive(eepromMutex);
    } else {
        Log::error("Could not acquire EEPROM mutex.");
    }
}

float getStoredMaxTemperature() {
    float temp = NAN;
    loadMaxTemperature(temp);
    return temp;
}

// Save temperature to EEPROM
bool saveMaxTemperature(float temperature) {
    if (eepromMutex == NULL) {
        Log::error("EEPROM mutex not initialized.");
        return false;
    }
    if (xSemaphoreTake(eepromMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        EEPROM.write(FLAG_TEMP_ADDR, FLAG_TEMP_VALID);
        byte *tempPtr = (byte*)&temperature;
        for (int i = 0; i < sizeof(float); ++i) {
            EEPROM.write(TEMP_ADDR + i, tempPtr[i]);
        }
        bool ok = EEPROM.commit();
        xSemaphoreGive(eepromMutex);
        if (ok) {
            Log::info("Temperature %.2f saved to EEPROM.", temperature);
        } else {
            Log::error("Failed to commit temperature to EEPROM.");
        }
        return ok;
    } else {
        Log::error("Could not acquire EEPROM mutex.");
        return false;
    }
}

// Read temperature from EEPROM
bool loadMaxTemperature(float &temperature) {
    if (eepromMutex == NULL) {
        Log::error("EEPROM mutex not initialized.");
        return false;
    }
    if (xSemaphoreTake(eepromMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (EEPROM.read(FLAG_TEMP_ADDR) != FLAG_TEMP_VALID) {
            xSemaphoreGive(eepromMutex);
            return false;
        }
        byte tempBytes[sizeof(float)];
        for (int i = 0; i < sizeof(float); ++i) {
            tempBytes[i] = EEPROM.read(TEMP_ADDR + i);
        }
        memcpy(&temperature, tempBytes, sizeof(float));
        xSemaphoreGive(eepromMutex);
        // Log::debug("Loaded temperature from EEPROM: %.2f", temperature);
        return true;
    } else {
        Log::error("Could not acquire EEPROM mutex.");
        return false;
    }
}

// Save max time to EEPROM
bool saveMaxTime(uint32_t maxTimeSeconds) {
    if (eepromMutex == NULL) {
        Log::error("EEPROM mutex not initialized.");
        return false;
    }
    if (xSemaphoreTake(eepromMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        EEPROM.write(FLAG_MAX_TIME_ADDR, FLAG_MAX_TIME_VALID);
        byte *timePtr = (byte*)&maxTimeSeconds;
        for (int i = 0; i < sizeof(uint32_t); ++i) {
            EEPROM.write(MAX_TIME_ADDR + i, timePtr[i]);
        }
        bool ok = EEPROM.commit();
        xSemaphoreGive(eepromMutex);
        if (ok) {
            Log::info("Max time %lu seconds saved to EEPROM.", maxTimeSeconds);
        } else {
            Log::error("Failed to commit max time to EEPROM.");
        }
        return ok;
    } else {
        Log::error("Could not acquire EEPROM mutex.");
        return false;
    }
}

// Read max time from EEPROM
bool loadMaxTime(uint32_t &maxTimeSeconds) {
    if (eepromMutex == NULL) {
        Log::error("EEPROM mutex not initialized.");
        return false;
    }
    if (xSemaphoreTake(eepromMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (EEPROM.read(FLAG_MAX_TIME_ADDR) != FLAG_MAX_TIME_VALID) {
            xSemaphoreGive(eepromMutex);
            return false;
        }
        byte timeBytes[sizeof(uint32_t)];
        for (int i = 0; i < sizeof(uint32_t); ++i) {
            timeBytes[i] = EEPROM.read(MAX_TIME_ADDR + i);
        }
        memcpy(&maxTimeSeconds, timeBytes, sizeof(uint32_t));
        xSemaphoreGive(eepromMutex);
        return true;
    } else {
        Log::error("Could not acquire EEPROM mutex.");
        return false;
    }
}