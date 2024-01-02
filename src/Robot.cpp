#include "Robot.h"
#include "Remote.h"
#include "Safety.h"

#include "CRC.h"

void Robot::update(){
    uint32_t nowMicros = micros();
    if(nowMicros - lastSendTimeMicros >= sendIntervalMicros) {
        if(sendProcessData()) lastSendTimeMicros = nowMicros;
    }
    if(receiveProcessData()){
        lastReceivedTimeMillis = millis();
        if(!b_connected){
            b_connected = true;
            Serial.printf("%i ——— Robot Connected\n", millis());
        }
    }
    if(b_connected && millis() - lastReceivedTimeMillis > timeoutDelayMillis){
        b_connected = false;
        Serial.printf("%i ——— Robot Disconnected\n", millis());
    }
}



bool Robot::sendProcessData(){

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

    sendCounter++;

    uint16_t safetyNumber;
    if(Remote::ioDevices.eStopButton.isButtonPressed()) safetyNumber = Safety::generateSafeNumber();
    else safetyNumber = Safety::generateClearNumber();

    uint8_t outgoingFrame[11];
    uint8_t nodeID = int(floor(Remote::radio.getFrequency() * 10)) % 255;
    outgoingFrame[0] = nodeID;
    outgoingFrame[1] = controlWord;
    outgoingFrame[2] = map(Remote::ioDevices.leftJoystick.getXValue(), -1.0, 1.0, 0, 255);
    outgoingFrame[3] = map(Remote::ioDevices.leftJoystick.getYValue(), -1.0, 1.0, 0, 255);
    outgoingFrame[4] = map(Remote::ioDevices.rightJoystick.getXValue(), -1.0, 1.0, 0, 255);
    outgoingFrame[5] = map(Remote::ioDevices.rightJoystick.getYValue(), -1.0, 1.0, 0, 255);
    outgoingFrame[6] = sendCounter;
    outgoingFrame[7] = safetyNumber;
    outgoingFrame[8] = safetyNumber >> 8;

    uint16_t crc = calcCRC16(outgoingFrame, 9);
    outgoingFrame[9] = crc;
    outgoingFrame[10] = crc >> 8;


    //———— Send Frame

    bool b_success = Remote::radio.send(outgoingFrame, 11);
    if(b_success) b_frameSendBlinker = !b_frameSendBlinker;

    return b_success;
}



bool Robot::receiveProcessData(){
    uint8_t incomingFrame[14];
    if(!Remote::radio.receive(incomingFrame, 14)) return false;

    uint16_t calculatedCRC = calcCRC16(incomingFrame, 12);
    uint16_t receivedCRC = incomingFrame[12] | (incomingFrame[13] << 8);

    b_frameReceiveBlinker = !b_frameReceiveBlinker;
    b_receiverFrameCorrupted = receivedCRC != calculatedCRC;

    if(calculatedCRC != receivedCRC) return false;

    uint8_t expectedNodeID = int(floor(Remote::radio.getFrequency() * 10)) % 255;
    uint8_t nodeID = incomingFrame[0];

    if(expectedNodeID != nodeID){
        Serial.printf("Frame received from wrong nodeID %i and not %i\n", nodeID, expectedNodeID);
        return false;
    }

    uint8_t messageCounter = incomingFrame[1];
    if(sendCounter == messageCounter){
        uint32_t sendToReceiveTimeMicros = micros() - lastSendTimeMicros;
        lastMessageRoundTripTimeMillis = double(sendToReceiveTimeMicros) / 1000.0;
        Serial.printf("%i msg#%i round trip: %.1fms\n", millis(), messageCounter, lastMessageRoundTripTimeMillis);
    }

    robotRxSignalStrength = incomingFrame[2] - 150;
    remoteRxSignalStrength = Remote::radio.getSignalStrength();

    uint8_t robotStatusWord = incomingFrame[3];
    robotState = State(robotStatusWord & 0xF);
    
    uint8_t motorStatusWord = incomingFrame[4];
    frontLeft_alarm =       motorStatusWord & 0x1;
    backLeft_alarm =        motorStatusWord & 0x2;
    frontRight_alarm =      motorStatusWord & 0x4;
    backRight_alarm =       motorStatusWord & 0x8;
    frontLeft_enabled =     motorStatusWord & 0x10;
    backLeft_enabled =      motorStatusWord & 0x20;
    frontRight_enabled =    motorStatusWord & 0x40;
    backRight_enabled =     motorStatusWord & 0x80;
    
    int8_t xVelocity_i8 = incomingFrame[5];
    int8_t yVelocity_i8 = incomingFrame[6];
    int8_t rVelocity_i8 = incomingFrame[7];
    xVelocity = map(float(xVelocity_i8), -127.0, 127.0, -1.0, 1.0);
    yVelocity = map(float(yVelocity_i8), -127.0, 127.0, -1.0, 1.0);
    rVelocity = map(float(rVelocity_i8), -127.0, 127.0, -1.0, 1.0);

    auto getWheelVelEightBits = [](uint8_t raw)->float{
        int8_t integer = raw - 128;
        float norm = float(integer) / 128.0;
        return norm;
    };
    fl_vel = getWheelVelEightBits(incomingFrame[8]);
    bl_vel = getWheelVelEightBits(incomingFrame[9]);
    fr_vel = getWheelVelEightBits(incomingFrame[10]);
    br_vel = getWheelVelEightBits(incomingFrame[11]);

    return true;
    

}


