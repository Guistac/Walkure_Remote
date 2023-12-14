#pragma once

#include <Arduino.h>
#include <RH_RF95.h>

#include "Utilities.h"

#include <EEPROM.h>

class Radio{
private:

    uint8_t reset_pin = 7;
    uint8_t chipSelect_pin = 10;
    uint8_t interrupt_pin = 8;
    float bandwidthKHz; //62.5->500.0KHz
    int spreadingFactor; //7-12
    float frequencyMHz; //read by eeprom on startup
    float savedFrequencyMHz;

    enum class Bandwidth{
        KHZ_125,
        KHZ_250,
        KHZ_500
    };

    enum class SpreadingFactor{
        SF_7,
        SF_8,
        SF_9
    };

    enum class CodingRate{
        CR_5,
        CR_6,
        CR_7,
        CR_8
    };

public:

    Radio(){
        rf95 = new RH_RF95(chipSelect_pin, interrupt_pin);
    }

    bool saveFrequency(){
        if(savedFrequencyMHz != frequencyMHz){
            savedFrequencyMHz = frequencyMHz;
            uint32_t frequencyKHz = frequencyMHz * 1000.0;
            EEPROM.write(0x30, uint8_t(frequencyKHz & 0xFF));
            EEPROM.write(0x31, uint8_t((frequencyKHz >> 8) & 0xFF));
            EEPROM.write(0x32, uint8_t((frequencyKHz >> 16) & 0xFF));
            EEPROM.write(0x33, uint8_t((frequencyKHz >> 24) & 0xFF));
            return true;
        }
        return false;
    }

    bool initialize(float bw, int sf){

        rf95->waitCAD();    

        uint8_t data[4];
        data[0] = EEPROM.read(0x30);
        data[1] = EEPROM.read(0x31);
        data[2] = EEPROM.read(0x32);
        data[3] = EEPROM.read(0x33);
        uint32_t frequencyKHz = *(uint32_t*)data;
        float loadedFrequencyMHz = float(frequencyKHz) / 1000.0;
        savedFrequencyMHz = loadedFrequencyMHz;
        setFrequency(loadedFrequencyMHz);
        Serial.printf("Loaded Radio Frequency %.3fMHz\n", loadedFrequencyMHz);

        bandwidthKHz = bw;
        spreadingFactor = sf;

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
        rf95->setSpreadingFactor(spreadingFactor);  //6->12 default==7 lower is faster, higher is better for range, 6 works only with implicit mode which is not available, haven't tested higher values since they are real slow

        if(!rf95->setFrequency(frequencyMHz)){
            Serial.printf("Could not set radio Frequency to %.1fMHz\n", frequencyMHz);
            return false;
        }

        Serial.println("Initialized Radio.");

        return true;
    }

    bool send(uint8_t* buffer, uint8_t length){
        if(true){
            return rf95->send(buffer, length);
        }else{
            uint32_t startMicros = micros();
            bool b_success = rf95->send(buffer, length);
            rf95->waitPacketSent();
            uint32_t time = micros() - startMicros;
            float time_ms = float(time) / 1000.0;
            Serial.printf("Send time: %.2fms\n", time_ms);
            return b_success;
        }
    }

    bool receive(uint8_t* buffer, uint8_t length){
        uint8_t receivedLength = length;
        bool b_frameReceived = rf95->recv(buffer, &receivedLength);
        //if(b_frameReceived) Serial.printf("Reception Frequency Offset : %i Hz\n", rf95->frequencyError());
        return b_frameReceived && receivedLength == length;
    }

    int16_t getSignalStrength(){ return rf95->lastRssi(); }

    float getFrequency(){ return frequencyMHz; }

    void setFrequency(float newFrequencyMHz){
        if(newFrequencyMHz > 525) newFrequencyMHz = 525;
        else if(newFrequencyMHz < 410) newFrequencyMHz = 410;

        if(rf95->setFrequency(newFrequencyMHz)) frequencyMHz = newFrequencyMHz;

        rf95->setThisAddress(33);
        rf95->setHeaderTo(34);
        rf95->setHeaderFrom(35);
        rf95->setHeaderFlags(36);
        rf95->setHeaderId(37);
    }

private:
    RH_RF95* rf95;
};