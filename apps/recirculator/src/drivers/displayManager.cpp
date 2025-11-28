#include "displayManager.h"

#include "config.h"
#include "eeprom_config.h"
#include "temperature_sensor.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Log.h>

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
    while (true) {
        float maxTemperature = getStoredMaxTemperature();
        if (isnan(maxTemperature)) {
            maxTemperature = 30.0; // Default value if not set
        }
        display.clearDisplay();
        // Títol
        display.setTextSize(1.8);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 1);
        display.println("Recirculador d'aigua");
        // Separador 
        display.drawLine(0, 15, SCREEN_WIDTH, 15, SSD1306_WHITE);
        // Temperatura en gran 
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

        // Estat sistema sota la temperatura
        display.setTextSize(1);
        display.setCursor(0, 46);
        if (digitalRead(RELAY_PIN) == HIGH) {
            display.println("Sistema: ON");
        } else {
            display.println("Sistema: OFF");
        }

        // Segell de marca i proximament pressió
        display.setCursor(0, 56);
        display.print("T.Max: ");
        display.print(maxTemperature, 2);

        display.display();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
