#include <Arduino.h>

#include "ioDevice.h"

class RemoteIODevices{
public:

    //display
    //0 1 2

    //radio
    //7 8 10

    //io devices
    //3 4 5 6 9 14 15 16 17 18 19 20 21 22 23

    //spi bus
    //11 12 13

    PushButton eStopButton =                PushButton(4, INPUT_PULLUP, true);

    PushButtonWithLED leftLedButton =       PushButtonWithLED(22, INPUT_PULLUP, 19);
    PushButton leftPushButton =             PushButton(21, INPUT_PULLUP, false);
    TwoWayToggleSwitch speedToggleSwitch = TwoWayToggleSwitch(3, 20, INPUT_PULLUP);
    ToggleSwitch modeToggleSwitch =         ToggleSwitch(5, INPUT_PULLUP, false);
    PushButton rightPushButton =            PushButton(9, INPUT_PULLUP, false);
    PushButtonWithLED rightLedButton =      PushButtonWithLED(6, INPUT_PULLUP, 18);

    Joystick2Axis leftJoystick =            Joystick2Axis(16, 17, true, false);
    Joystick2Axis rightJoystick =           Joystick2Axis(15, 14, false, false);

    BatteryLevel batteryReading =           BatteryLevel(23, 2.0, 2.5, 4.2);

private:

    int deviceCount = 10;
    IODevice* ioDevices[10] = {
        &eStopButton,
        &leftLedButton,
        &rightLedButton,
        &leftPushButton,
        &rightPushButton,
        &speedToggleSwitch,
        &modeToggleSwitch,
        &leftJoystick,
        &rightJoystick,
        &batteryReading
    };

public:

    bool initialize(){
        IODevice::setup();
        for(int i = 0; i < deviceCount; i++) ioDevices[i]->initialize();

        Serial.println("Initialized IO-devices.");
        return true;
    }

    void updateInputs(){
        for(int i = 0; i < deviceCount; i++) ioDevices[i]->updateInputs();
        
        if(false){
            Serial.printf("%.2f %.2f %.2f %.2f %i %i %i %i %i %i %i\n",
                leftJoystick.getXValue(),
                leftJoystick.getYValue(),
                rightJoystick.getXValue(),
                rightJoystick.getYValue(),
                eStopButton.isButtonPressed(),
                leftLedButton.isButtonPressed(),
                leftPushButton.isButtonPressed(),
                speedToggleSwitch.getSwitchState(),
                modeToggleSwitch.getSwitchState(),
                rightPushButton.isButtonPressed(),
                rightLedButton.isButtonPressed());
        }
        
    }

    void updateOutputs(){
        for(int i = 0; i < 8; i++) ioDevices[i]->updateOutputs();
    }

};