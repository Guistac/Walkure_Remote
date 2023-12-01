#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1305.h>

#include "Utilities.h"

class Display{
private:

    uint8_t chipSelectPin = 1;
    uint8_t dcPin = 0;
    uint8_t resetPin = 2;
    float refreshRate_Hz = 50.0;

public:

    Display(){
        display = new Adafruit_SSD1305(128, 32, &SPI, dcPin, resetPin, chipSelectPin, 7000000UL);
        refreshIntervalMicros = 1000000.0 / refreshRate_Hz;
    }

    bool initialize(){
        if(!display->begin()){
            Serial.println("Unable to initialize OLED");
            return false;
        }
        display->clearDisplay();
        display->fillScreen(BLACK);
        display->display();

        if(!onSetup()){
            Serial.println("Display user setup failed.");
            return false;
        }

        Serial.println("Initialized display.");
        return true;
    }

    void update(){
        uint32_t nowMicros = micros();
        if(nowMicros - lastRefreshMicros < refreshIntervalMicros) return;
        lastRefreshMicros = nowMicros;
        
        onUpdate();
        display->display();
    }

    bool onSetup();
    void onUpdate();

private:

    Adafruit_SSD1305* display;
    uint32_t lastRefreshMicros = UINT32_MAX;
    uint32_t refreshIntervalMicros;
};