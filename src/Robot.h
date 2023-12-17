#pragma once
#include <Arduino.h>

class Robot{
private:

    float sendFrequencyHz = 25.0; //seems to be the max at 500Khz Bandwith

public:

    Robot(){
        sendIntervalMicros = 1000000.0 / sendFrequencyHz;
    }

    void update();

    enum class State{
        DISABLING = 0x1,
        DISABLED = 0x2,
        ENABLING = 0x3,
        ENABLED = 0x4,
        EMERGENCY_STOPPING = 0x5,
        EMERGENCY_STOPPED = 0x6
    };

    bool b_connected = false;
    float xVelocity = 0.0;
    float yVelocity = 0.0;
    float rVelocity = 0.0;
    float batteryVoltage = 0.0;
    State robotState = State::DISABLED;
    bool frontLeft_alarm = false;
    bool backLeft_alarm = false;
    bool frontRight_alarm = false;
    bool backRight_alarm = false;
    bool frontLeft_enabled = false;
    bool backLeft_enabled = false;
    bool frontRight_enabled = false;
    bool backRight_enabled = false;

    int16_t robotRxSignalStrength = 0;
    int16_t remoteRxSignalStrength = 0;

    bool b_frameSendBlinker = false;
    bool b_frameReceiveBlinker = false;
    bool b_receiverFrameCorrupted = false;
    float lastMessageRoundTripTimeMillis = 0.0;

private:

    void sendProcessData();
    void receiveProcessData();

    uint32_t lastSendTimeMicros = UINT32_MAX;
    uint32_t sendIntervalMicros;
    uint8_t sendCounter = 0;

    uint32_t lastReceivedTimeMillis = UINT32_MAX;
    uint32_t timeoutDelayMillis = 500;

};