#pragma once

#include <Arduino.h>

class SimpleTimer{
public:
    SimpleTimer(){ startMicros = micros(); }
    uint32_t getTimeMicros(){ return micros() - startMicros; }
    double getTimeMillis(){ return double(micros() - startMicros) / 1000.0; }
    double getTimeSeconds(){ return double(micros() - startMicros) / 1000.0; }
    uint32_t startMicros;
};


class TaskTimer{
public:

    TaskTimer(float frequencyHz){
        triggerInterval_Micros = 1000000.0 / frequencyHz;
    }

    bool shouldTrigger(){
        uint32_t now_Micros = micros();
        if(now_Micros - lastTrigger_Micros > triggerInterval_Micros){
            lastTrigger_Micros = now_Micros;
            return true;
        }
        return false;
    }

private:

    uint32_t triggerInterval_Micros = 0;
    uint32_t lastTrigger_Micros = UINT32_MAX;
};