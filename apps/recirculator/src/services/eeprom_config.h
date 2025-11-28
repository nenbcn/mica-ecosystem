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
#define TEMP_ADDR 200            // Nova adreça per guardar la temperatura
#define FLAG_TEMP_ADDR 204       // Adreça per la flag de temperatura
#define FLAG_TEMP_VALID 0xB5     // Flag de validació per la temperatura
#define MAX_TIME_ADDR 208        // Adreça per guardar el temps màxim (en segons)
#define FLAG_MAX_TIME_ADDR 212   // Adreça per la flag de temps màxim
#define FLAG_MAX_TIME_VALID 0xC5 // Flag de validació per el temps màxim

// Variable per guardar la temperatura màxima
float getStoredMaxTemperature();

// Variable per guardar el temps màxim de funcionament
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
 * @brief Guarda la temperatura a l'EEPROM.
 * @param temperature Temperatura a guardar (float).
 * @return true si s'ha guardat correctament, false altrament.
 */
bool saveMaxTemperature(float temperature);

/**
 * @brief Llegeix la temperatura guardada a l'EEPROM.
 * @param temperature Variable on es guardarà la temperatura llegida.
 * @return true si s'ha llegit correctament, false altrament.
 */
bool loadMaxTemperature(float &temperature);

/**
 * @brief Guarda el temps màxim a l'EEPROM.
 * @param maxTimeSeconds Temps màxim en segons (uint32_t).
 * @return true si s'ha guardat correctament, false altrament.
 */
bool saveMaxTime(uint32_t maxTimeSeconds);

/**
 * @brief Llegeix el temps màxim guardat a l'EEPROM.
 * @param maxTimeSeconds Variable on es guardarà el temps llegit.
 * @return true si s'ha llegit correctament, false altrament.
 */
bool loadMaxTime(uint32_t &maxTimeSeconds);

#endif