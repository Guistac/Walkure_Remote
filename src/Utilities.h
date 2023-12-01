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