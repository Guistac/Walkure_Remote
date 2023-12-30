#pragma once

#include <Arduino.h>
#include <RH_RF95.h>

#include "Utilities.h"

#include <EEPROM.h>

class RF95 : public RH_RF95{
    public:
    RF95(uint8_t cs, uint8_t irq) : RH_RF95(cs, irq){}
    virtual bool modeWillChange(RHMode mode) override {

        //Serial.printf("%i ", millis());
        switch(mode){
            case RHGenericDriver::RHMode::RHModeIdle:
                if(currentMode == Mode::SENDING){
                    uint32_t sendTime_micros = micros() - sendStart_micros;
                    //double sendTime_millis = double(sendTime_micros) / 1000.0;
                    //Serial.printf("Done Sending (took %.2fms)\n", sendTime_millis);
                }else if(currentMode == Mode::RECEIVING){
                    uint32_t receiveTime_micros = micros() - receiveStart_micros;
                    //double receiveTime_millis = double(receiveTime_micros) / 1000.0;
                    //Serial.printf("Done Receiving (took %.2fms)\n", receiveTime_millis);
                }
                currentMode = Mode::SLEEP;
                break;
            case RHGenericDriver::RHMode::RHModeRx:
                currentMode = Mode::RECEIVING;
                receiveStart_micros = micros();
                //Serial.println("Start Receiving");
                break;
            case RHGenericDriver::RHMode::RHModeTx:
                currentMode = Mode::SENDING;
                sendStart_micros = micros();
                //Serial.println("Start Sending");
                break;
            default:
                //Serial.printf("Mode %i\n", mode);
                break;
        }

        return true;
    }

    enum class Mode{
        SLEEP,
        SENDING,
        RECEIVING
    };
    Mode currentMode = Mode::RECEIVING;
    uint32_t sendStart_micros;
    uint32_t receiveStart_micros;
};

class Radio{
private:

    uint8_t reset_pin = 7;
    uint8_t chipSelect_pin = 10;
    uint8_t interrupt_pin = 8;
    float bandwidthKHz; //62.5->500.0KHz
    int spreadingFactor; //7-12
    float frequencyMHz; //read by eeprom on startup
    float savedFrequencyMHz;

public:

    Radio(){
        rf95 = new RF95(chipSelect_pin, interrupt_pin);
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

    bool canSend(){ return rf95->currentMode != RF95::Mode::SENDING; }
    bool canReceive(){ return rf95->currentMode != RF95::Mode::SENDING; }

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
        if(!canSend()) {
            //sometimes the radio library misses an interrupt
            //this can cause the radio stay in send mode and stay stuck in waitPacketSend()
            //a quick fix for this is to manually set the radio mode back to idle mode
            //ideally we should have a send timeout counter that only does this mode reset when necessary
            static int counter = 0;
            Serial.printf("%i Radio Recovery Attempt N°%i\n", millis(), counter);
            counter++;
            rf95->setModeIdle();
            return false;
        }

        uint32_t startMicros = micros();
        bool b_success = rf95->send(buffer, length);

        if(false){
            rf95->waitPacketSent();
            uint32_t time = micros() - startMicros;
            float time_ms = float(time) / 1000.0;
            Serial.printf("Send time: %.2fms %i\n", time_ms, millis());
        }

        return b_success;
    }

    bool receive(uint8_t* buffer, uint8_t length){
        if(!canReceive()) {
            return false;
        }
        
        uint8_t receivedLength = length;
        bool b_frameReceived = rf95->recv(buffer, &receivedLength);
        return b_frameReceived && receivedLength == length;
    }

    int16_t getSignalStrength(){ return rf95->lastRssi(); }

    float getFrequency(){ return frequencyMHz; }

    void setFrequency(float newFrequencyMHz){
        if(newFrequencyMHz > 525) newFrequencyMHz = 525;
        else if(newFrequencyMHz < 410) newFrequencyMHz = 410;
        if(rf95->setFrequency(newFrequencyMHz)) frequencyMHz = newFrequencyMHz;
    }

private:
    RF95* rf95;
};