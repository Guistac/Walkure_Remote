#pragma once

#include <Arduino.h>

class IODevice{
public:

    virtual void initialize() = 0;
    virtual void updateInputs() = 0;
    virtual void updateOutputs() = 0;

    static void setup(){
        analogReadResolution(12);
        analogWriteResolution(8);
    }

    static int getAnalogReadResolution(){
       return 4095;
    }

    static int getAnalogWriteResolution(){
        return 255;
    }

};




class PushButton : public IODevice{
public:

    PushButton(uint8_t inputPin_, uint8_t inputPinConfiguration_, bool invert_){
        inputPin = inputPin_;
        invert = invert_;
        switch(inputPinConfiguration_){
            case INPUT:
            case INPUT_PULLDOWN:
            case INPUT_PULLUP:
                inputPinConfiguration = inputPinConfiguration_;
                break;
            default:
                inputPinConfiguration = INPUT;
                break;
        }
    }

    virtual void initialize() override {
        pinMode(inputPin, inputPinConfiguration);
    }

    virtual void updateInputs() override {
        uint32_t nowMillis = millis();
        if(nowMillis - lastInputEdgeMillis > debounceDelayMillis){
            bool newButtonState = digitalRead(inputPin);
            if(inputPinConfiguration == INPUT_PULLUP) newButtonState = !newButtonState;
            if(invert) newButtonState = !newButtonState;
            if(newButtonState != buttonState){
                buttonState = newButtonState;
                lastInputEdgeMillis = nowMillis;
            }
        }
    };

    virtual void updateOutputs() override {}

    bool isButtonPressed() { return buttonState; }

private:
    uint8_t inputPin;
    uint8_t inputPinConfiguration;
    bool invert;
    bool buttonState;
    uint32_t lastInputEdgeMillis = UINT32_MAX;
    uint32_t debounceDelayMillis = 50;
};



class PushButtonWithLED : public IODevice{
public:

    PushButtonWithLED(uint8_t buttonPin_, uint8_t buttonPinConfiguration_, uint8_t ledPin_){
        buttonPin = buttonPin_;
        ledPin = ledPin_;
        switch(buttonPinConfiguration){
            case INPUT:
            case INPUT_PULLDOWN:
            case INPUT_PULLUP:
                buttonPinConfiguration = buttonPinConfiguration_;
                break;
            default:
                buttonPinConfiguration = INPUT;
                break;
        }
    }

    virtual void initialize() override {
        pinMode(buttonPin, buttonPinConfiguration);
        pinMode(ledPin, OUTPUT);
        analogWrite(ledPin, ledBrightness);
    };

    virtual void updateInputs() override {
        uint32_t nowMillis = millis();
        if(nowMillis - lastInputEdgeMillis >= debounceDelayMillis){
            bool newButtonState = digitalRead(buttonPin);
            if(buttonPinConfiguration == INPUT_PULLUP) newButtonState = !newButtonState;
            if(newButtonState != buttonState){
                buttonState = newButtonState;
                lastInputEdgeMillis = nowMillis;
            }
        }
    };

    virtual void updateOutputs() override {
        analogWrite(ledPin, ledBrightness);
    }

    bool isButtonPressed(){ return buttonState; }
    void setLedBrightness(float brightness){
        if(brightness >= 1.0) ledBrightness = getAnalogWriteResolution();
        if(brightness <= 0.0) ledBrightness = 0;
        else ledBrightness = map(brightness, 0.0, 1.0, 0, getAnalogWriteResolution());
    }
    void turnLedOn(){ ledBrightness = getAnalogWriteResolution(); }
    void turnLedOff(){ ledBrightness = 0; }
    void setLed(bool isOn){
        if(isOn) ledBrightness = getAnalogWriteResolution();
        else ledBrightness = 0;
    }

private:
    uint8_t buttonPin;
    uint8_t ledPin;
    uint8_t buttonPinConfiguration;
    uint32_t debounceDelayMillis = 50;
    uint32_t lastInputEdgeMillis = UINT32_MAX;
    bool buttonState;
    uint8_t ledBrightness = 0;
};



class ToggleSwitch : public IODevice{
public:

    ToggleSwitch(uint8_t inputPin_, uint8_t inputPinConfiguration_, bool invert_){
        inputPin = inputPin_;
        invert = invert_;
        switch(inputPinConfiguration_){
            case INPUT:
            case INPUT_PULLDOWN:
            case INPUT_PULLUP:
                inputPinConfiguration = inputPinConfiguration_;
                break;
            default:
                inputPinConfiguration = INPUT;
                break;
        }
    }

    virtual void initialize() override {
        pinMode(inputPin, inputPinConfiguration);
    }

    virtual void updateInputs() override {
        uint32_t nowMillis = millis();
        if(nowMillis - lastInputEdgeMillis >= debounceDelayMillis){
            bool newSwitchState = digitalRead(inputPin);
            if(invert) newSwitchState = !newSwitchState;
            if(inputPinConfiguration == INPUT_PULLUP) newSwitchState = !newSwitchState;
            if(switchState != newSwitchState){
                switchState = newSwitchState;
                lastInputEdgeMillis = nowMillis;
            }
        }
    }

    virtual void updateOutputs() override {}

    bool getSwitchState(){ return switchState; }

private:
    uint8_t inputPin;
    uint8_t inputPinConfiguration;
    bool invert;
    bool switchState;
    uint32_t lastInputEdgeMillis = UINT32_MAX;
    uint32_t debounceDelayMillis = 50;
};



class TwoWayToggleSwitch : public IODevice{
public:

    TwoWayToggleSwitch(uint8_t inputPin1_, uint8_t inputPin2_, uint8_t inputPinConfiguration_){
        inputPin1 = inputPin1_;
        inputPin2 = inputPin2_;
        switch(inputPinConfiguration_){
            case INPUT:
            case INPUT_PULLDOWN:
            case INPUT_PULLUP:
                inputPinConfiguration = inputPinConfiguration_;
                break;
            default:
                inputPinConfiguration = INPUT;
                break;
        }
    }

    virtual void initialize() override {
        pinMode(inputPin1, inputPinConfiguration);
        pinMode(inputPin2, inputPinConfiguration);
    }

    virtual void updateInputs() override {
        uint32_t nowMillis = millis();
        if(nowMillis - lastInputEdgeMillis > debounceTimeMillis){
            bool pin1 = digitalRead(inputPin1);
            bool pin2 = digitalRead(inputPin2);
            if(inputPinConfiguration == INPUT_PULLUP){
                pin1 = !pin1;
                pin2 = !pin2;
            }
            uint8_t newSwitchState = readingsToState(pin1, pin2);
            if(newSwitchState != switchState){
                switchState = newSwitchState;
                lastInputEdgeMillis = millis();
            }
        }
    }

    virtual void updateOutputs() override {}

    uint8_t getSwitchState(){ return switchState; }

private:

    uint8_t readingsToState(bool reading1, bool reading2){
        if(reading1 && reading2) return 3;
        else if(reading1) return 1;
        else if(reading2) return 2;
        else return 0;
    }

    uint8_t inputPin1;
    uint8_t inputPin2;
    uint8_t inputPinConfiguration;
    uint8_t switchState;
    uint32_t lastInputEdgeMillis = UINT32_MAX;
    uint32_t debounceTimeMillis = 50;
};



class Joystick2Axis : public IODevice{
public:

    Joystick2Axis(uint8_t xAxisPin_, uint8_t yAxisPin_, bool invertX_, bool invertY_){
        xAxisPin = xAxisPin_;
        yAxisPin = yAxisPin_;
        invertX = invertX_;
        invertY = invertY_;
    }

    virtual void initialize() override {
        pinMode(xAxisPin, INPUT);
        pinMode(yAxisPin, INPUT);
    }

    virtual void updateInputs() override {
        uint32_t nowMillis = millis();
        if(nowMillis - lastReadingMillis >= readingDelayMillis){
            lastReadingMillis = nowMillis;
            xAxisValue = readingToValue(analogRead(xAxisPin), invertX);
            yAxisValue = readingToValue(analogRead(yAxisPin), invertY);
        }
    }

    virtual void updateOutputs() override {}

    float getXValue(){ return xAxisValue; }
    float getYValue(){ return yAxisValue; }

private:

    float readingToValue(uint16_t reading, bool b_invert){
        float normalized = map(float(reading), 0.0, getAnalogReadResolution(), -1.0, 1.0);
        float absolute = abs(normalized);
        bool sign = normalized > 0.0;
        float output;
        if(absolute > maxLevel) output = 1.0;
        else if(absolute < minLevel) return 0.0;
        else {
            output = map(absolute, minLevel, maxLevel, 0.0, 1.0);
            if(output > 1.0) output = 1.0;
            else if(output < 0.0) output = 0.0;
        }
        if(!sign) output = -output;
        if(b_invert) output = -output;
        return output;
    }

    uint8_t xAxisPin;
    uint8_t yAxisPin;
    bool invertX;
    bool invertY;
    float xAxisValue;
    float yAxisValue;
    float minLevel = 0.1;
    float maxLevel = 0.98;
    uint32_t lastReadingMillis = UINT32_MAX;
    uint32_t readingDelayMillis = 20;
};



class BatteryLevel : public IODevice{
public:

    BatteryLevel(uint8_t analogInputPin_, float voltageDividerFactor_, float minVoltage_, float maxVoltage_){
        inputPin = analogInputPin_;
        voltageDividerFactor = voltageDividerFactor_;
        minVoltage = minVoltage_;
        maxVoltage = maxVoltage_;
    }

    virtual void initialize() override {
        pinMode(inputPin, INPUT);
        voltage = readingToVoltage(analogRead(inputPin));
        level = voltageToLevel(voltage);
    }

    virtual void updateInputs() override {
        uint32_t nowMillis = millis();
        if(nowMillis - lastUpdateMillis >= updateDelayMillis){
            lastUpdateMillis = nowMillis;
            float newVoltage = readingToVoltage(analogRead(inputPin));
            voltage = voltage * smoothingFilter + newVoltage * (1.f - smoothingFilter);
            level = voltageToLevel(voltage);
        }
    }

    virtual void updateOutputs() override {}

    float getLevel(){ return level; }
    float getVoltage(){ return voltage; }

    float readingToVoltage(uint16_t reading){
        return 3.3 * voltageDividerFactor * float(reading) / getAnalogReadResolution();
    }

    float voltageToLevel(float voltage){
        float output = map(voltage, minVoltage, maxVoltage, 0.0, 1.0);
        if(output <= 0.0) return 0.0;
        else if(output >= 1.0) return 1.0;
        return output;
    }

private:

    uint8_t inputPin;
    float voltageDividerFactor;
    float minVoltage;
    float maxVoltage;
    float voltage;
    float level;
    float smoothingFilter = 0.99;
    uint32_t updateDelayMillis = 50;
    uint32_t lastUpdateMillis = UINT32_MAX;

};