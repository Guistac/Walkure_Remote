#include "Robot.h"
#include "Remote.h"
#include "Safety.h"

#include "CRC.h"

void Robot::update(){
    uint32_t nowMicros = micros();
    if(nowMicros - lastSendTimeMicros >= sendIntervalMicros) {
        lastSendTimeMicros = nowMicros;
        sendProcessData();
    }
    receiveProcessData();
    if(b_connected && millis() - lastReceivedTimeMillis > timeoutDelayMillis){
        b_connected = false;
        Serial.println("——— Robot Disconnected");
    }
    timeoutNormalized = float(millis() - lastReceivedTimeMillis) / float(timeoutDelayMillis);
}



void Robot::sendProcessData(){

    uint8_t outgoingFrameSize = 7;
    uint8_t outgoingFrame[outgoingFrameSize];

    //———— Process Data Formatting

    bool b_shouldDisable = Remote::ioDevices.leftLedButton.isButtonPressed();
    bool b_shouldEnable = Remote::ioDevices.rightLedButton.isButtonPressed();
    uint8_t speedSelection = Remote::ioDevices.speedToggleSwitch.getSwitchState();
    bool b_modeToggle = Remote::ioDevices.modeToggleSwitch.getSwitchState();
    bool b_leftButton = Remote::ioDevices.leftPushButton.isButtonPressed();
    bool b_rightButton = Remote::ioDevices.rightPushButton.isButtonPressed();

    uint8_t controlWord = 0x0;
    if(b_shouldDisable)         controlWord |= 0x1;
    if(b_shouldEnable)          controlWord |= 0x2;
    controlWord |= (speedSelection & 0x3) << 2;
    if(b_modeToggle)            controlWord |= 0x10;
    if(b_leftButton)            controlWord |= 0x20;
    if(b_rightButton)           controlWord |= 0x40;

    uint16_t safetyNumber;
    if(Remote::ioDevices.eStopButton.isButtonPressed()) safetyNumber = Safety::generateSafeNumber();
    else safetyNumber = Safety::generateClearNumber();

    outgoingFrame[0] = controlWord;
    outgoingFrame[1] = map(Remote::ioDevices.leftJoystick.getXValue(), -1.0, 1.0, 0, 255);
    outgoingFrame[2] = map(Remote::ioDevices.leftJoystick.getYValue(), -1.0, 1.0, 0, 255);
    outgoingFrame[3] = map(Remote::ioDevices.rightJoystick.getXValue(), -1.0, 1.0, 0, 255);
    outgoingFrame[4] = map(Remote::ioDevices.rightJoystick.getYValue(), -1.0, 1.0, 0, 255);
    outgoingFrame[5] = safetyNumber;
    outgoingFrame[6] = safetyNumber >> 8;

    //———— Send Frame

    if(Remote::radio.send(outgoingFrame, outgoingFrameSize)) b_frameSendBlinker = !b_frameSendBlinker;

    if(false){
        Serial.print("Frame: ");
        for(int i = outgoingFrameSize-1; i >= 0; i--){
            bool b0 = outgoingFrame[i] & 0x1;
            bool b1 = outgoingFrame[i] & 0x2;
            bool b2 = outgoingFrame[i] & 0x4;
            bool b3 = outgoingFrame[i] & 0x8;
            bool b4 = outgoingFrame[i] & 0x10;
            bool b5 = outgoingFrame[i] & 0x20;
            bool b6 = outgoingFrame[i] & 0x40;
            bool b7 = outgoingFrame[i] & 0x80;
            Serial.printf("%i%i%i%i%i%i%i%i ", b7, b6, b5, b4, b3, b2, b1, b0);
        }
        Serial.println("");
    }

}



void Robot::receiveProcessData(){

    uint8_t incomingFrameSize = 7;
    uint8_t incomingFrame[incomingFrameSize];
    Radio::ReceptionResult result = Remote::radio.receive(incomingFrame, incomingFrameSize);

    switch(result){
        case Radio::ReceptionResult::NOTHING_RECEIVED:
            return;
        case Radio::ReceptionResult::BAD_CRC:
            b_frameReceiveBlinker = !b_frameReceiveBlinker;
            b_receiverFrameCorrupted = true;
            return;
        case Radio::ReceptionResult::BAD_NODEID:
            b_frameReceiveBlinker = !b_frameReceiveBlinker;
            b_receiverFrameCorrupted = true;
            return;
        case Radio::ReceptionResult::GOOD_RECEPTION:
            b_frameReceiveBlinker = !b_frameReceiveBlinker;
            b_receiverFrameCorrupted = false;
            break;
    }

    lastReceivedTimeMillis = millis();
    if(!b_connected){
        b_connected = true;
        Serial.println("——— Robot Connected");
    }

    uint8_t robotStatusWord = incomingFrame[0];
    robotState = State(robotStatusWord & 0xF);
    uint8_t speedModeDisplay = (robotStatusWord >> 2) & 0x3;
    
    uint8_t motorStatusWord = incomingFrame[1];
    frontLeft_alarm =       motorStatusWord & 0x1;
    backLeft_alarm =        motorStatusWord & 0x2;
    frontRight_alarm =      motorStatusWord & 0x4;
    backRight_alarm =       motorStatusWord & 0x8;
    frontLeft_enabled =     motorStatusWord & 0x10;
    backLeft_enabled =      motorStatusWord & 0x20;
    frontRight_enabled =    motorStatusWord & 0x40;
    backRight_enabled =     motorStatusWord & 0x80;
    
    int8_t xVelocity_i8 = incomingFrame[2];
    int8_t yVelocity_i8 = incomingFrame[3];
    int8_t rVelocity_i8 = incomingFrame[4];
    xVelocity = map(float(xVelocity_i8), -127.0, 127.0, -1.0, 1.0);
    yVelocity = map(float(yVelocity_i8), -127.0, 127.0, -1.0, 1.0);
    rVelocity = map(float(rVelocity_i8), -127.0, 127.0, -1.0, 1.0);

    robotRxSignalStrength = incomingFrame[5] - 150;
    uint8_t batteryVoltage = incomingFrame[6];

    remoteRxSignalStrength = Remote::radio.getSignalStrength();

}


