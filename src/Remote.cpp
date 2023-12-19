#include "Remote.h"

namespace Remote{

    const uint8_t radio_reset_pin = 7;
    const uint8_t radio_chipSelect_pin = 10;
    const uint8_t radio_interrupt_pin = 8;

    Display display;
    RemoteIODevices ioDevices;
    Radio radio(Radio::TranscieverType::MASTER, Radio::FrequencyRange::MHZ_433, radio_reset_pin, radio_chipSelect_pin, radio_interrupt_pin);
    Robot robot;

    TaskTimer buttonEvent(10.0);

    void initialize(){

        if(false){
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
        if(!radio.initialize()) Serial.println("Radio failed to initialize.");
    }


    void update(){

        ioDevices.updateInputs();
        robot.update();
        display.update();

        if(buttonEvent.shouldTrigger()){
            if(ioDevices.eStopButton.isButtonPressed()){
                if(ioDevices.leftLedButton.isButtonPressed()){

                    float joystick = ioDevices.rightJoystick.getYValue();
                    float abs = joystick > 0.0 ? joystick : -joystick;

                    int adjustement;
                    if(abs < 0.05) adjustement = 0;
                    else if(abs < 0.25) adjustement = 100000;
                    else if(abs < 0.5) adjustement = 200000;
                    else if(abs < 0.75) adjustement = 500000;
                    else if(abs < 0.98) adjustement = 1000000;
                    else adjustement = 2000000;

                    if(joystick > 0.0) radio.setFrequency(radio.getFrequency() + adjustement);
                    else radio.setFrequency(radio.getFrequency() - adjustement);
                }
                if(ioDevices.leftLedButton.isButtonPressed() && ioDevices.rightLedButton.isButtonPressed()){
                    if(radio.saveFrequency()) Serial.println("Saved Frequency too EEPROM");
                }
            }
        }

        float timeSeconds = float(millis()) / 1000.0;
        if(ioDevices.eStopButton.isButtonPressed() && ioDevices.leftLedButton.isButtonPressed()){
            if(ioDevices.rightLedButton.isButtonPressed()){
                ioDevices.leftLedButton.setLedBrightness(map(sin(timeSeconds * 50.0), -1.0, 1.0, 0.0, 1.0));
                ioDevices.rightLedButton.setLedBrightness(map(sin(timeSeconds * 50.0), -1.0, 1.0, 0.0, 1.0));
            }
            else{
                ioDevices.leftLedButton.setLedBrightness(map(sin(timeSeconds * 20.0), -1.0, 1.0, 0.0, 1.0));
            }
        }
        else if(!robot.b_connected){
            ioDevices.leftLedButton.setLedBrightness(map(sin(timeSeconds * 2.0), 0.92, 1.0, 0.0, 0.5));
            ioDevices.rightLedButton.setLedBrightness(map(sin(timeSeconds * 2.0 - PI / 16.0), 0.92, 1.0, 0.0, 0.5));
        }else{
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
        }

        ioDevices.updateOutputs();

    }

};