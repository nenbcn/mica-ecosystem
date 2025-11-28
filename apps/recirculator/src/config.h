// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <Log.h>
#include <stdint.h>  // For uint32_t type

// Access Point Constants
constexpr char AP_SSID[] = "MICA-Recirculator";
constexpr char AP_PASSWORD[] = "12345678";

// Hardware Pins - ESP32 Configuration
#ifdef ESP32_C3
    // ESP32-C3 Pin Definitions
    #define BUTTON_PIN 9              // D9
    #define RELAY_PIN 8               // D8
    #define TEMPERATURE_SENSOR_PIN 2  // D0
    #define BUZZER_PIN 20             // D7 (GPIO20) - Passivo, requereix PWM
    #define SDA_PIN 6                 // D4
    #define SCL_PIN 7                 // D5
    #define PRESSURE_SENSOR_PIN 3     // D1 (future)
    #define SENSOR_PIN 21             // Generic sensor
    
    // NeoPixel configuration
    #define NEOPIXEL_PIN 5            // D3 pin (GPIO05)
    #define NEOPIXEL_COUNT 1          // Single NeoPixel
    
    // Legacy LED pins (for compatibility)
    #define GREEN_LED_PIN 4
    #define RED_LED_PIN 10
    #define BLUE_LED_PIN 5
#else
    // ESP32 WROOM Pin Definitions
    #define BUTTON_PIN 13
    #define RELAY_PIN 12
    #define TEMPERATURE_SENSOR_PIN 4
    #define BUZZER_PIN 18
    #define SDA_PIN 21                // D22 o D21
    #define SCL_PIN 22
    #define SENSOR_PIN 22
    
    // Standard LED pins
    #define GREEN_LED_PIN 27
    #define RED_LED_PIN 4
    #define BLUE_LED_PIN 15
#endif

// OTA Constants
constexpr char firmwareUrl[] = "https://ota.mica.eco/firmware.bin";

#endif