// eeprom_config.h
#ifndef EEPROM_CONFIG_H
#define EEPROM_CONFIG_H

#include <Arduino.h>
#include <stdint.h>

// EEPROM Configuration Module
// Purpose:
// Handles storage and retrieval of Wi-Fi credentials in EEPROM.
// Provides functions for saving, loading, clearing, and validating credentials.

// Constants
#define EEPROM_SIZE 512         // Total EEPROM size
#define SSID_ADDR 0             // Address for SSID storage
#define PASS_ADDR 64            // Address for Password storage
#define FLAG_ADDR 128           // Address for validation flag
#define FLAG_VALID 0xA5         // Validation flag value
#define MAX_CRED_LENGTH 64      // Maximum length for SSID and Password
#define TEMP_ADDR 200            // Address for storing max temperature
#define FLAG_TEMP_ADDR 204       // Address for temperature validation flag
#define FLAG_TEMP_VALID 0xB5     // Validation flag for temperature
#define MAX_TIME_ADDR 208        // Address for storing max time (in seconds)
#define FLAG_MAX_TIME_ADDR 212   // Address for max time validation flag
#define FLAG_MAX_TIME_VALID 0xC5 // Validation flag for max time

// Get stored maximum temperature from EEPROM
float getStoredMaxTemperature();

// Get stored maximum operation time from EEPROM
uint32_t getStoredMaxTime();

/**
 * @brief Initializes EEPROM and the associated mutex.
 * @return true if initialization is successful, false otherwise.
 */
bool eepromInitialize();

/**
 * @brief Validates if the EEPROM size is sufficient.
 * @return true if size is sufficient, false otherwise.
 */
bool validateEEPROMSize();

/**
 * @brief Saves Wi-Fi credentials to EEPROM.
 * @param ssid The SSID to save.
 * @param password The password to save.
 * @return true if credentials are saved successfully, false otherwise.
 */
bool saveCredentials(const String &ssid, const String &password);

/**
 * @brief Loads Wi-Fi credentials from EEPROM.
 * @param ssid Variable to store the loaded SSID.
 * @param password Variable to store the loaded password.
 * @return true if credentials are loaded successfully, false otherwise.
 */
bool loadCredentials(String &ssid, String &password);

/**
 * @brief Clears Wi-Fi credentials from EEPROM.
 */
void clearCredentials();

/**
 * @brief Prints the contents of EEPROM.
 */
void printEEPROMContents();

/**
 * @brief Save maximum temperature to EEPROM.
 * @param temperature Temperature to save (float)
 * @return true if saved successfully, false otherwise
 */
bool saveMaxTemperature(float temperature);

/**
 * @brief Load maximum temperature from EEPROM.
 * @param temperature Variable where the temperature will be stored
 * @return true if loaded successfully, false otherwise
 */
bool loadMaxTemperature(float &temperature);

/**
 * @brief Save maximum time to EEPROM.
 * @param maxTimeSeconds Maximum time in seconds (uint32_t)
 * @return true if saved successfully, false otherwise
 */
bool saveMaxTime(uint32_t maxTimeSeconds);

/**
 * @brief Load maximum time from EEPROM.
 * @param maxTimeSeconds Variable where the time will be stored
 * @return true if loaded successfully, false otherwise
 */
bool loadMaxTime(uint32_t &maxTimeSeconds);

#endif