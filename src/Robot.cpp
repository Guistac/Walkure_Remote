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
            Serial.println("——— Robot Connected");
        }
    }
    if(b_connected && millis() - lastReceivedTimeMillis > timeoutDelayMillis){
        b_connected = false;
        Serial.println("——— Robot Disconnected");
    }
}



bool Robot::sendProcessData(){

    if(!Remote::radio.canSend()) return false;

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

    if(false){
        Serial.print("Frame: ");
        for(int i = 10; i >= 0; i--){
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

    return b_success;

}



bool Robot::receiveProcessData(){
    uint8_t incomingFrame[11];
    if(!Remote::radio.receive(incomingFrame, 11)) return false;

    uint16_t calculatedCRC = calcCRC16(incomingFrame, 9);
    uint16_t receivedCRC = incomingFrame[9] | (incomingFrame[10] << 8);

    b_frameReceiveBlinker = !b_frameReceiveBlinker;
    b_receiverFrameCorrupted = receivedCRC != calculatedCRC;

    if(calculatedCRC != receivedCRC) return false;

    uint8_t expectedNodeID = int(floor(Remote::radio.getFrequency() * 10)) % 255;
    uint8_t nodeID = incomingFrame[0];

    if(expectedNodeID != nodeID){
        Serial.printf("Frame received from wrong nodeID %i and not %i\n", nodeID, expectedNodeID);
        return false;
    }

    uint8_t robotStatusWord = incomingFrame[1];
    robotState = State(robotStatusWord & 0xF);
    uint8_t speedModeDisplay = (robotStatusWord >> 2) & 0x3;
    
    uint8_t motorStatusWord = incomingFrame[2];
    frontLeft_alarm =       motorStatusWord & 0x1;
    backLeft_alarm =        motorStatusWord & 0x2;
    frontRight_alarm =      motorStatusWord & 0x4;
    backRight_alarm =       motorStatusWord & 0x8;
    frontLeft_enabled =     motorStatusWord & 0x10;
    backLeft_enabled =      motorStatusWord & 0x20;
    frontRight_enabled =    motorStatusWord & 0x40;
    backRight_enabled =     motorStatusWord & 0x80;
    
    int8_t xVelocity_i8 = incomingFrame[3];
    int8_t yVelocity_i8 = incomingFrame[4];
    int8_t rVelocity_i8 = incomingFrame[5];
    xVelocity = map(float(xVelocity_i8), -127.0, 127.0, -1.0, 1.0);
    yVelocity = map(float(yVelocity_i8), -127.0, 127.0, -1.0, 1.0);
    rVelocity = map(float(rVelocity_i8), -127.0, 127.0, -1.0, 1.0);

    robotRxSignalStrength = incomingFrame[6] - 150;
    uint8_t batteryVoltage = incomingFrame[7];
    uint8_t messageCounter = incomingFrame[8];

    remoteRxSignalStrength = Remote::radio.getSignalStrength();

    if(sendCounter == messageCounter){
        uint32_t sendToReceiveTimeMicros = micros() - lastSendTimeMicros;
        lastMessageRoundTripTimeMillis = double(sendToReceiveTimeMicros) / 1000.0;
        Serial.printf("msg#%i round trip: %.1fms\n", messageCounter, lastMessageRoundTripTimeMillis);
    }

    return true;

}


