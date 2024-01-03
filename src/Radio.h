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

public:

    Radio(){ rf95 = new RF95(chipSelect_pin, interrupt_pin); }

    bool initialize(float bw, int sf, float freqMHz){

        bandwidthKHz = bw;
        spreadingFactor = sf;
        frequencyMHz = freqMHz;

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
        }else Serial.printf("Set Radio Frequency to %.3fMHz\n", frequencyMHz);

        rf95->setPromiscuous(true);

        Serial.println("Initialized Radio.");

        return true;
    }


    bool send(uint8_t* buffer, uint8_t length){
        if(length <= 4) return false;

        handleTxTimeout();

        //if we are already in TX Mode and try to send a packet we will get stuck in waitPacketSent()
        if(rf95->isInTxMode()) return false;

        //used for measuring frame send time when debugging
        uint32_t startMicros = micros();

        rf95->setHeaderTo(buffer[0]);
        rf95->setHeaderFrom(buffer[1]);
        rf95->setHeaderId(buffer[2]);
        rf95->setHeaderFlags(buffer[3], 255);

        uint8_t* payloadBuffer = buffer + 4;
        uint8_t payloadSize = length - 4; 

        if(rf95->send(payloadBuffer, payloadSize)){
            //mark down send start time to check for send timeout later
            lastSendRequest_millis = millis();   

            if(false){
                rf95->waitPacketSent();
                uint32_t time = micros() - startMicros;
                float time_ms = float(time) / 1000.0;
                Serial.printf("Send time: %.2fms %i\n", time_ms, millis());
            }

            if(false){
                Serial.print("Outgoing Frame: ");
                for(int i = length - 1; i >= 0; i--){
                    bool b0 = buffer[i] & 0x1;
                    bool b1 = buffer[i] & 0x2;
                    bool b2 = buffer[i] & 0x4;
                    bool b3 = buffer[i] & 0x8;
                    bool b4 = buffer[i] & 0x10;
                    bool b5 = buffer[i] & 0x20;
                    bool b6 = buffer[i] & 0x40;
                    bool b7 = buffer[i] & 0x80;
                    Serial.printf("%i%i%i%i%i%i%i%i ", b7, b6, b5, b4, b3, b2, b1, b0);
                }
                Serial.println("");
            }

            return true;
        }

        return false;
    }


    bool receive(uint8_t* buffer, uint8_t length){
        handleTxTimeout();

        //if we are still in TX Mode, the radio library will refuse to switch to RX Mode
        if(rf95->isInTxMode()) return false;

        uint8_t* payloadBuffer = buffer + 4;
        uint8_t receivedLength = length - 4;
        if(!rf95->recv(payloadBuffer, &receivedLength)) return false;
        if(receivedLength != length - 4) return false;

        buffer[0] = rf95->headerTo();
        buffer[1] = rf95->headerFrom();
        buffer[2] = rf95->headerId();
        buffer[3] = rf95->headerFlags();

        if(false){
            Serial.print("Incoming Frame: ");
            for(int i = length - 1; i >= 0; i--){
                bool b0 = buffer[i] & 0x1;
                bool b1 = buffer[i] & 0x2;
                bool b2 = buffer[i] & 0x4;
                bool b3 = buffer[i] & 0x8;
                bool b4 = buffer[i] & 0x10;
                bool b5 = buffer[i] & 0x20;
                bool b6 = buffer[i] & 0x40;
                bool b7 = buffer[i] & 0x80;
                Serial.printf("%i%i%i%i%i%i%i%i ", b7, b6, b5, b4, b3, b2, b1, b0);
            }
            Serial.println("");
        }

        return true;
    }


    int16_t getSignalStrength(){ return rf95->lastRssi(); }

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

    float getFrequency(){ return frequencyMHz; }

    void setFrequency(float newFrequencyMHz){
        if(newFrequencyMHz > 525) newFrequencyMHz = 525;
        else if(newFrequencyMHz < 410) newFrequencyMHz = 410;
        if(rf95->setFrequency(newFrequencyMHz)) frequencyMHz = newFrequencyMHz;
    }

private:

    //Make our own class derived from the RadioHead radio class
    //this way we can peek into the protected _mode variable
    class RF95 : public RH_RF95{
    public:
        RF95(uint8_t cs, uint8_t irq) : RH_RF95(cs, irq){}
        bool isInTxMode(){ return _mode == RHMode::RHModeTx; }
    };
    RF95* rf95;

    float frequencyMHz; //read by eeprom on startup
    float bandwidthKHz; //62.5->500.0KHz
    int spreadingFactor; //7-12

    float savedFrequencyMHz; //for checking if the current frequency was saved to EEPROM

    //sometimes the radio library misses an interrupt
    //this can cause the radio stay in TXMode and loop in waitPacketSend() or not be able to receive
    //a quick fix for this is to manually set the radio mode back to idle mode
    //we keep track of the time we spend in TXMode, and if that takes too long we force the radio back to idle mode
    uint32_t lastSendRequest_millis = 0;
    uint32_t sendTimeout_millis = 100;
    void handleTxTimeout(){
        if(rf95->isInTxMode() && millis() - lastSendRequest_millis > sendTimeout_millis){
            rf95->setModeIdle();
            lastSendRequest_millis = millis();

            static int counter = 0;
            counter++;
            Serial.printf("%i Radio Recovery Attempt N°%i\n", millis(), counter);
        }
    }

};