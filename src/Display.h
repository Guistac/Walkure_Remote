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
        
        //disable interrupts while using the spi bus
        //else we might interfere with the radio interrupt routine which uses spi
        //cli();
        display->display();
        //sei();
    }

    bool onSetup();
    void onUpdate();

    void Display::drawMecanumWheel(uint32_t& animationOffset, int x, int y, int w, int h, bool o, float v, bool alm, bool ena);

private:

    Adafruit_SSD1305* display;
    uint32_t lastRefreshMicros = UINT32_MAX;
    uint32_t refreshIntervalMicros;

    uint32_t fl_offset = UINT32_MAX / 2;
    uint32_t fr_offset = UINT32_MAX / 2;
    uint32_t bl_offset = UINT32_MAX / 2;
    uint32_t br_offset = UINT32_MAX / 2;
};