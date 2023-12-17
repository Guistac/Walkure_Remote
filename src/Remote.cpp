#include "Remote.h"

namespace Remote{

    Display display;
    RemoteIODevices ioDevices;
    Radio radio;
    Robot robot;

    TaskTimer buttonEvent(10.0);

    void initialize(Configuration config){

        if(true){
            while(!Serial){
                pinMode(13, OUTPUT);
                digitalWrite(13, HIGH);
                delay(500);
                digitalWrite(13, LOW);
                delay(500);
            }
            Serial.println("==================== REMOTE START ========================");
        }

        if(!display.initialize()) Serial.println("Display failed to initialize.");
        if(!ioDevices.initialize()) Serial.println("io Devices failed to initialize.");
        if(!radio.initialize(config.bandwidthKHz, config.spreadingFactor)) Serial.println("Radio failed to initialize.");
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

        if(buttonEvent.shouldTrigger()){
            int frequencyIncrements = radio.getFrequency() * 10;
            int adjustement;

            switch(ioDevices.speedToggleSwitch.getSwitchState()){
                case 0: adjustement = 10; break;
                case 1: adjustement = 100; break;
                case 2: adjustement = 1; break;
                default: adjustement = 0; break;
            }

            if(ioDevices.rightPushButton.isButtonPressed()){
                frequencyIncrements += adjustement;
                float newFrequencyMHz = float(frequencyIncrements) / 10.0;
                radio.setFrequency(newFrequencyMHz);
                Serial.printf("set frequency to %.6fMHz\n", radio.getFrequency());
            }
            else if(ioDevices.leftPushButton.isButtonPressed()) {
                frequencyIncrements -= adjustement;
                float newFrequencyMHz = float(frequencyIncrements) / 10.0;
                radio.setFrequency(newFrequencyMHz);
                Serial.printf("set frequency to %.6fMHz\n", radio.getFrequency());
            }
            if(ioDevices.eStopButton.isButtonPressed() && ioDevices.leftLedButton.isButtonPressed() && ioDevices.rightLedButton.isButtonPressed()){
                if(radio.saveFrequency()) Serial.println("Saved Frequency too EEPROM");;
            }
        }


        ioDevices.updateOutputs();

    }

};