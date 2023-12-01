#include "Remote.h"

namespace Remote{

    Display display;
    RemoteIODevices ioDevices;
    Radio radio;
    Robot robot;

    void initialize(){
        //while(!Serial){}
        //Serial.println("==================== REMOTE START ========================");

        if(!display.initialize()) Serial.println("Display failed to initialize.");
        if(!ioDevices.initialize()) Serial.println("io Devices failed to initialize.");
        if(!radio.initialize()) Serial.println("Radio failed to initialize.");
    }


    void update(){
        ioDevices.updateInputs();
        robot.update();
        display.update();


        float timeSeconds = float(millis()) / 1000.0;

        switch(robot.robotState){
            case Robot::State::ENABLED:
                ioDevices.leftLedButton.setLed(false);
                ioDevices.rightLedButton.setLed(true);
                break;
            case Robot::State::ENABLING:
                ioDevices.rightLedButton.setLedBrightness(map(sin(timeSeconds * 30.0), -1.f, 1.f, 0.f, 1.f));
                ioDevices.leftLedButton.setLed(false);
                break;
            case Robot::State::DISABLED:
                ioDevices.leftLedButton.setLed(true);
                ioDevices.rightLedButton.setLed(false);
                break;
            case Robot::State::DISABLING:
                ioDevices.leftLedButton.setLedBrightness(map(sin(timeSeconds * 30.0), -1.f, 1.f, 0.f, 1.f));
                ioDevices.rightLedButton.setLed(false);
                break;
            case Robot::State::EMERGENCY_STOPPED:
                ioDevices.leftLedButton.setLed(millis() % 500 > 250);
                ioDevices.rightLedButton.setLed(millis() % 500 < 250);
                break;
            case Robot::State::EMERGENCY_STOPPING:
                ioDevices.leftLedButton.setLed(millis() % 100 > 50);
                ioDevices.rightLedButton.setLed(millis() % 100 < 50);
                break;
        }
        if(!robot.b_connected){


            ioDevices.leftLedButton.setLedBrightness(map(sin(timeSeconds * 2.0), 0.92, 1.0, 0.0, 0.5));
            ioDevices.rightLedButton.setLedBrightness(map(sin(timeSeconds * 2.0 - PI / 16.0), 0.92, 1.0, 0.0, 0.5));
        }



        ioDevices.updateOutputs();
    }

};