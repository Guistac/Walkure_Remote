#include <Arduino.h>

#include "ioDevice.h"

class RemoteIODevices{
public:

    //display
    //0 1 2

    //radio
    //7 8 x9x 10

    //io devices
    //3 4 5 6 14 15 16 17 18 19 20 21 22 23

    //spi bus
    //11 12 13

    PushButton eStopButton =                PushButton(4, INPUT_PULLUP, true);
    PushButtonWithLED leftLedButton =       PushButtonWithLED(22, INPUT_PULLUP, 19);
    PushButtonWithLED rightLedButton =      PushButtonWithLED(6, INPUT_PULLUP, 18);
    ToggleSwitch leftToggleSwitch =         ToggleSwitch(21, INPUT_PULLUP, false);
    ToggleSwitch rightToggleSwitch =        ToggleSwitch(5, INPUT_PULLUP, false);
    TwoWayToggleSwitch centerToggleSwitch = TwoWayToggleSwitch(3, 20, INPUT_PULLUP);
    Joystick2Axis leftJoystick =            Joystick2Axis(16, 17, true, false);
    Joystick2Axis rightJoystick =           Joystick2Axis(15, 14, false, false);
    BatteryLevel batteryReading =           BatteryLevel(23, 2.0, 2.5, 4.2);

private:

    int deviceCount = 9;
    IODevice* ioDevices[9] = {
        &eStopButton,
        &leftLedButton,
        &rightLedButton,
        &leftToggleSwitch,
        &rightToggleSwitch,
        &centerToggleSwitch,
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
    }

    void updateOutputs(){

/*
        if(eStopButton.isButtonPressed()){
            float timeSeconds = float(millis()) / 1000.f;
            leftLedButton.setLedBrightness((sin(timeSeconds * 3.0 + PI) + 1.0) / 2.0);
            rightLedButton.setLedBrightness((sin(timeSeconds * 3.0) + 1.0) / 2.0);
        }
        else{
            leftLedButton.setLed(leftLedButton.isButtonPressed());
            rightLedButton.setLed(rightLedButton.isButtonPressed());
        }
*/
        for(int i = 0; i < 8; i++) ioDevices[i]->updateOutputs();
    }

};