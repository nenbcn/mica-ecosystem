// display_manager.cpp
// Display Manager Module
// Purpose: Manages SSD1306 OLED display for showing temperature, system status, and configuration
// Architecture: FreeRTOS task that updates display every 1 second
// Thread-Safety: Reads temperature and relay state from thread-safe accessors
// Dependencies: Adafruit_SSD1306, temperature_sensor, relay_controller, eeprom_config

#include "display_manager.h"

// Project headers (alphabetically)
#include "config.h"
#include "eeprom_config.h"
#include "temperature_sensor.h"

// Third-party libraries
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Log.h>
#include <Wire.h>

// System headers
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool initializeDisplayManager() {
    Wire.begin(SDA_PIN, SCL_PIN);
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Log::error("Display not found");
        return false;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Iniciant...");
    display.display();
    return true;
}

void displayManagerTask(void *pvParameters) {
    constexpr float DEFAULT_MAX_TEMPERATURE = 30.0f; // Default target temperature in Celsius
    
    while (true) {
        float maxTemperature = getStoredMaxTemperature();
        if (isnan(maxTemperature)) {
            maxTemperature = DEFAULT_MAX_TEMPERATURE;
        }
        display.clearDisplay();
        // Title
        display.setTextSize(1.8);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 1);
        display.println("Recirculador d'aigua");
        // Separator line
        display.drawLine(0, 15, SCREEN_WIDTH, 15, SSD1306_WHITE);
        // Temperature in large font 
        display.setTextSize(2);
        display.setCursor(0, 22);
        display.print("T: ");
        float currentTemp = getLatestTemperature();
        if (currentTemp == -127.0f) {
            display.print("ERROR");
        } else {
            display.print(currentTemp, 1);
            display.print("C");
        }

        // System status below temperature
        display.setTextSize(1);
        display.setCursor(0, 46);
        if (digitalRead(RELAY_PIN) == HIGH) {
            display.println("Sistema: ON");
        } else {
            display.println("Sistema: OFF");
        }

        // Brand and future pressure info
        display.setCursor(0, 56);
        display.print("T.Max: ");
        display.print(maxTemperature, 2);

        display.display();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
