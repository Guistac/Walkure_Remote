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
}



void Robot::sendProcessData(){

    uint8_t outgoingFrame[12];
    uint8_t* processDataBuffer = &outgoingFrame[0]; //8 bytes in here
    uint8_t* safetyDataBuffer = &outgoingFrame[8]; //4 bytes in here


    //———— Process Data Formatting

    bool b_shouldDisable = Remote::ioDevices.leftLedButton.isButtonPressed();
    bool b_shouldEnable = Remote::ioDevices.rightLedButton.isButtonPressed();
    bool b_speedToggle = Remote::ioDevices.leftToggleSwitch.getSwitchState();
    uint8_t modeToggle = Remote::ioDevices.centerToggleSwitch.getSwitchState();
    bool b_modeToggle = Remote::ioDevices.rightToggleSwitch.getSwitchState();

    uint8_t controlWord = 0x0;
    if(b_shouldDisable)         controlWord |= 0x1;
    if(b_shouldEnable)          controlWord |= 0x2;
    if(b_speedToggle)           controlWord |= 0x4;
    if(modeToggle == 1)         controlWord |= 0x8;
    else if(modeToggle == 2)    controlWord |= 0x10;
    if(b_modeToggle)            controlWord |= 0x20;

    processDataBuffer[0] = controlWord;
    processDataBuffer[1] = map(Remote::ioDevices.leftJoystick.getXValue(), -1.0, 1.0, 0, 255);
    processDataBuffer[2] = map(Remote::ioDevices.leftJoystick.getYValue(), -1.0, 1.0, 0, 255);
    processDataBuffer[3] = map(Remote::ioDevices.rightJoystick.getXValue(), -1.0, 1.0, 0, 255);
    processDataBuffer[4] = map(Remote::ioDevices.rightJoystick.getYValue(), -1.0, 1.0, 0, 255);

    sendCounter++;
    processDataBuffer[5] = sendCounter;

    uint16_t processDataCRC = calcCRC16(processDataBuffer, 6);
    processDataBuffer[6] = processDataCRC;
    processDataBuffer[7] = processDataCRC >> 8;


    //———— Safety Data Formatting
 
    uint16_t safetyNumber;
    if(Remote::ioDevices.eStopButton.isButtonPressed()) safetyNumber = Safety::generateSafeNumber();
    else safetyNumber = Safety::generateClearNumber();

    safetyDataBuffer[0] = safetyNumber;
    safetyDataBuffer[1] = safetyNumber >> 8;
    uint16_t safetyDataCRC = calcCRC16(safetyDataBuffer, 2);
    safetyDataBuffer[2] = safetyDataCRC;
    safetyDataBuffer[3] = safetyDataCRC >> 8;


    //———— Send Frame

    Remote::radio.send(outgoingFrame, 12);
    b_frameSendBlinker = !b_frameSendBlinker;

    if(false){
        Serial.print("Frame: ");
        for(int i = 11; i >= 0; i--){
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
    uint8_t incomingFrame[10];
    if(!Remote::radio.receive(incomingFrame, 10)) return;

    uint16_t calculatedCRC = calcCRC16(incomingFrame, 8);
    uint16_t receivedCRC = incomingFrame[8] | (incomingFrame[9] << 8);

    b_frameReceiveBlinker = !b_frameReceiveBlinker;
    b_receiverFrameCorrupted = receivedCRC != calculatedCRC;

    if(calculatedCRC != receivedCRC) return;

    lastReceivedTimeMillis = millis();
    if(!b_connected){
        b_connected = true;
        Serial.println("——— Robot Connected");
    }

    uint8_t robotStatusWord = incomingFrame[0];
    robotState = State(robotStatusWord & 0xF);
    bool b_modeDisplay = robotStatusWord & 0x10;
    bool b_speedModeDisplay = robotStatusWord & 0x20;
    
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
    uint8_t messageCounter = incomingFrame[7];

    remoteRxSignalStrength = Remote::radio.getSignalStrength();

    if(sendCounter == messageCounter){
        uint32_t sendToReceiveTimeMicros = micros() - lastSendTimeMicros;
        lastMessageRoundTripTimeMillis = double(sendToReceiveTimeMicros) / 1000.0;
        //Serial.printf("msg#%i round trip: %.1fms\n", messageCounter, lastMessageRoundTripTimeMillis);
    }

}


