#pragma once

#include <Arduino.h>
#include <RH_RF95.h>

#include "Utilities.h"

class Radio{
private:

    uint8_t reset_pin = 7;
    uint8_t chipSelect_pin = 10;
    uint8_t interrupt_pin = 8;
    float frequencyMHz = 445.0; //360.0->565.0Mhz
    float bandwidthKHz = 125.0; //62.5->500.0KHz

public:

    Radio(){
        rf95 = new RH_RF95(chipSelect_pin, interrupt_pin);
    }

    bool initialize(){        
        pinMode(reset_pin, OUTPUT);
        digitalWrite(reset_pin, LOW);
        delay(10);
        digitalWrite(reset_pin, HIGH);
        delay(10);

        if(!rf95->init()) {
            Serial.println("Unable to initialize radio.");
            return false;
        }

        rf95->setSignalBandwidth(bandwidthKHz * 1000);   //smaller bandwidths are better for range, larger bandwidths are better for speed
        rf95->setTxPower(23, false);  //23dBm is max
        rf95->setPayloadCRC(false);   //we'll do our own CRC manually
        rf95->setCodingRate4(5);      //5->8, default==5 lower is faster, higher is better for range, radios of different values seem to communicate with each other
        rf95->setSpreadingFactor(7);  //6->12 default==7 lower is faster, higher is better for range, 6 doesn't seem to work, haven't tested higher values since they are real slow

        if(!rf95->setFrequency(frequencyMHz)){
            Serial.printf("Could not set radio Frequency to %.1fMHz\n", frequencyMHz);
            return false;
        }

        Serial.println("Initialized Radio.");

        return true;
    }

    bool send(uint8_t* buffer, uint8_t length){
        return rf95->send(buffer, length);
    }

    bool receive(uint8_t* buffer, uint8_t length){
        uint8_t receivedLength = length;
        bool b_frameReceived = rf95->recv(buffer, &receivedLength);
        return b_frameReceived && receivedLength == length;
    }

    int16_t getSignalStrength(){ return rf95->lastRssi(); }

    float getFrequency(){ return frequencyMHz; }

private:
    RH_RF95* rf95;
};