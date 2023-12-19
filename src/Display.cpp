#include "Display.h"
#include "Remote.h"


bool Display::onSetup(){

    display->setTextSize(1);
    display->setTextWrap(false);
    display->setTextColor(WHITE);

    return true;
}




static float readings[20000];
static int readingCount = 0;
static uint32_t lastReadingTime = 2000;
static uint32_t readingInterval = 2000;
static uint32_t oldestReadingTime;
static uint32_t newestReadingTime;
static float lowestReadingVoltage;
static float highestReadingVoltage;

struct Vector2{
  Vector2(){}
  Vector2(int x_, int y_) : x(x_), y(y_){}
  int x = 0;
  int y = 0;
};

Vector2 readingToDisplayCoordinate(float reading, int index){
  Vector2 output;
  output.x = map(index, 0, readingCount-1, 0, 128);
  output.y = map(reading, lowestReadingVoltage, highestReadingVoltage, 31.0, 0.0);
  return output;
};


void drawInputDeviceState(Adafruit_SSD1305* display, int x, int y){

  display->writePixel(x, Remote::ioDevices.leftLedButton.isButtonPressed() ? y+2 : y+1, WHITE);
  display->writePixel(x+1, Remote::ioDevices.leftPushButton.isButtonPressed() ? y+2 : y+1, WHITE);
  switch(Remote::ioDevices.speedToggleSwitch.getSwitchState()){
    case 0:
      display->writePixel(x+2, y+1, WHITE);
      break;
    case 1:
      display->writePixel(x+2, y, WHITE);
      break;
    case 2:
      display->writePixel(x+2, y+2, WHITE);
      break;
  }
  display->writePixel(x+3, Remote::ioDevices.eStopButton.isButtonPressed() ? y+1: y, WHITE);
  display->writePixel(x+4, Remote::ioDevices.modeToggleSwitch.getSwitchState() ? y : y+1, WHITE);
  display->writePixel(x+5, Remote::ioDevices.rightPushButton.isButtonPressed() ? y+2 : y+1, WHITE);
  display->writePixel(x+6, Remote::ioDevices.rightLedButton.isButtonPressed() ? y+2 : y+1, WHITE);

  auto drawJoystick = [&](int x, int y, int w, float val){
    auto indicator = map(val, -1.0, 1.0, x, x + w);
    display->drawLine(x, y, x + w, y, WHITE);
    display->drawLine(indicator, y-1, indicator, y+1, WHITE);
  };

  drawJoystick(x+8, y+1, 10, Remote::ioDevices.leftJoystick.getXValue());
  drawJoystick(x+20, y+1, 10, Remote::ioDevices.leftJoystick.getYValue());
  drawJoystick(x+32, y+1, 10, Remote::ioDevices.rightJoystick.getXValue());
  drawJoystick(x+44, y+1, 10, Remote::ioDevices.rightJoystick.getYValue());

}


void Display::onUpdate(){


  uint32_t nowMillis = millis();
  if(nowMillis - lastReadingTime >= readingInterval){
    lastReadingTime = nowMillis;

    float voltage = Remote::ioDevices.batteryReading.getVoltage();

    readings[readingCount] = voltage;

    if(readingCount == 0) {
      oldestReadingTime = nowMillis;
      lowestReadingVoltage = voltage;
      highestReadingVoltage = voltage;
    }else{
      if(voltage < lowestReadingVoltage) lowestReadingVoltage = voltage;
      if(voltage > highestReadingVoltage) highestReadingVoltage = voltage;
    }
    newestReadingTime = nowMillis;

    readingCount++;
  }

  display->clearDisplay();
   
   display->setCursor(0,0);
    if(!Remote::robot.b_connected) display->print("Offline");
    else{
      switch(Remote::robot.robotState){
        case Robot::State::DISABLED:
          display->printf("Disabled");
          break;
        case Robot::State::DISABLING:
          display->printf("Disabling...");
          break;
        case Robot::State::ENABLED:
          display->printf("Enabled");
          break;
        case Robot::State::ENABLING:
          display->printf("Enabling...");
          break;
        case Robot::State::EMERGENCY_STOPPED:
          display->printf("ESTOP");
          break;
        case Robot::State::EMERGENCY_STOPPING:
          display->printf("EStopping...");
          break;
        default:
          display->printf("?????");
          break;
      }
    }

    display->drawLine(127,0,127, Remote::robot.timeoutNormalized * 32, WHITE);

    display->setCursor(60, 0);
    display->printf("Battery:%i%%", int(Remote::ioDevices.batteryReading.getLevel() * 100.0));
    
    display->setCursor(0,12);
    display->printf("U:%idba\nD:%idba", Remote::robot.robotRxSignalStrength, Remote::robot.remoteRxSignalStrength);

    auto horizontalProgressBar = [&](int x, int y, int sizex, int sizey, float value){
      value = min(value, 1.0);
      value = max(value, 0.0);
      int maxX = x + sizex;
      int maxY = y + sizey;
      display->drawLine(x, y, maxX, y, WHITE);
      display->drawLine(x,y,x,maxY, WHITE);
      display->drawLine(maxX,y,maxX,maxY, WHITE);
      display->fillRect(x, y+1, (sizex-1) * value, sizey-2, WHITE);
    };

    float upLinkQuality = map(float(Remote::robot.robotRxSignalStrength), -80, -20, 0.0, 1.0);
    float downLinkQuality = map(float(Remote::robot.remoteRxSignalStrength), -80, -20, 0.0, 1.0);

    horizontalProgressBar(40, 11, 32, 6, upLinkQuality);
    horizontalProgressBar(40, 20, 32, 6, downLinkQuality);

    Vector2 rectMin(75,8);
    Vector2 rectSize(5, 24);

    if(Remote::robot.b_frameSendBlinker) display->fillRect(rectMin.x, rectMin.y, rectSize.x, rectSize.y, WHITE);
    rectMin.x += rectSize.x;
    if(Remote::robot.b_receiverFrameCorrupted){
      Vector2 rectMax(rectMin.x + rectSize.x, rectMin.y + rectSize.y);
      if(Remote::robot.b_frameReceiveBlinker){
        for(int i = rectMin.x; i < rectMax.x; i += 2) display->drawLine(i, rectMin.y, i, rectMax.y, WHITE);
      }
      else{
        for(int i = rectMin.y; i < rectMax.y; i += 2) display->drawLine(rectMin.x, i, rectMax.x, i, WHITE);
      }
    }else{
      if(Remote::robot.b_frameReceiveBlinker) display->fillRect(rectMin.x, rectMin.y, rectSize.x, rectSize.y, WHITE);
    }

    auto drawMotorSymbol = [&](bool alarm, bool enabled, int x, int y){
        if(alarm) {
          display->drawLine(x, y, x, y + 4, WHITE);
          display->drawLine(x, y, x + 3, y, WHITE);
          display->drawLine(x + 3, y, x + 3, y + 4, WHITE);
          display->drawLine(x, y + 2, x + 3, y + 2, WHITE);
        }
        else if(enabled){
          display->drawLine(x, y, x, y + 4, WHITE);
          display->drawLine(x, y, x + 3, y, WHITE);
          display->drawLine(x, y + 4, x + 3, y + 4, WHITE);
          display->drawLine(x, y + 2, x + 2, y + 2, WHITE);
        }
        else{
          display->drawLine(x, y, x, y + 4, WHITE);
          display->drawLine(x, y, x + 2, y, WHITE);
          display->drawLine(x, y + 4, x + 2, y + 4, WHITE);
          display->drawLine(x + 3, y + 1, x + 3, y+3, WHITE);
        }
      };

      //drawMotorSymbol(Remote::robot.frontLeft_alarm,  Remote::robot.frontLeft_enabled,  117, 10);
      //drawMotorSymbol(Remote::robot.backLeft_alarm,   Remote::robot.backLeft_enabled,   117, 16);
      //drawMotorSymbol(Remote::robot.frontRight_alarm, Remote::robot.frontRight_enabled, 122, 10);
      //drawMotorSymbol(Remote::robot.backRight_alarm,  Remote::robot.backRight_enabled,  122, 16);

      display->setCursor(86, 8);
      float rxGoodRatio = 100.0 * float(Remote::robot.rxCount) / float(Remote::robot.txCount);
      display->printf("%i %.2f", Remote::robot.disconnectionCount,rxGoodRatio);
      //display->printf("%.1f", Remote::radio.getLastRoundTripTime_ms());
      display->setCursor(86, 16);
      display->printf("%i", Remote::robot.txCount);
      display->setCursor(86, 24);
      display->printf("%.1fMHz", float(Remote::radio.getFrequency()) / 1000000.0);

      drawInputDeviceState(display, 0, 29);
      

      /*

  int switchState = Remote::ioDevices.speedToggleSwitch.getSwitchState();
  if(switchState != 0){

    float widthPerReading = 127.0 / float(readingCount);

    Vector2 previousCoord = readingToDisplayCoordinate(readings[0], 0);
    int previousXoff = 0;
    for(int i = 0; i < readingCount; i++){
      int xCoord = i * widthPerReading;
      if(xCoord < previousXoff) continue;

      Vector2 point = readingToDisplayCoordinate(readings[i], i);
      display->drawLine(previousCoord.x, previousCoord.y, point.x, point.y, WHITE);

      previousXoff = xCoord;
      previousCoord = point;
    }


    float xJ = Remote::ioDevices.rightJoystick.getXValue();
    float yJ = Remote::ioDevices.rightJoystick.getYValue();
    float dx = map(xJ, -1.0, 1.0, 0, 127);
    float dy = map(yJ, 1.0, -1.0, 0, 31);
    display->drawLine(dx, 0, dx, 31, WHITE);
    display->drawLine(0, dy, 127, dy, WHITE);

    float cursorTime = map(xJ, -1.0, 1.0, oldestReadingTime, newestReadingTime) / 1000.0;
    float cursorVoltage = map(yJ, -1.0, 1.0, lowestReadingVoltage, highestReadingVoltage);

    if(switchState == 1){
      display->setCursor(40, 0);
      display->printf("%.0fs %.3fV", cursorTime, cursorVoltage);
    }
    else{
      display->setCursor(40, 24);
      display->printf("%.0fs %.3fV", cursorTime, cursorVoltage);
    }




  }
  else{


    display->setCursor(60, 0);
    if(Remote::ioDevices.leftPushButton.isButtonPressed()) {
      display->printf("Batt:%.3fV", Remote::ioDevices.batteryReading.getVoltage());
      
       if(Remote::ioDevices.modeToggleSwitch.getSwitchState()){
        auto drawJoystick = [this](int x, int y, int r, float xV, float yV){
            display->fillRoundRect(x-r,y-r,2*r,2*r,2,WHITE);
            int xEnd = map(xV, -1.0, 1.0, x - r + 2, x + r - 3);
            int yEnd = map(yV, 1.0, -1.0, y - r + 2, y + r - 3);

            if(xV != 0.0 || yV != 0.0) {
                display->drawLine(x,y,xEnd,yEnd, BLACK);
                display->fillCircle(xEnd, yEnd, 2, BLACK);
            }
            else display->drawCircle(xEnd, yEnd, 2, BLACK);
      };
      drawJoystick(12,20,12,Remote::ioDevices.leftJoystick.getXValue(), Remote::ioDevices.leftJoystick.getYValue());
      drawJoystick(116,20,12,Remote::ioDevices.rightJoystick.getXValue(), Remote::ioDevices.rightJoystick.getYValue());

      display->setCursor(60, 16);
      display->printf("Tx:%i", Remote::robot.robotRxSignalStrength);
      display->setCursor(60, 24);
      display->printf("Rx:%i", Remote::robot.remoteRxSignalStrength);

      float xV = Remote::robot.xVelocity > 0.0 ? Remote::robot.xVelocity : -Remote::robot.xVelocity;
      float yV = Remote::robot.yVelocity > 0.0 ? Remote::robot.yVelocity : -Remote::robot.yVelocity;
      float rV = Remote::robot.rVelocity > 0.0 ? Remote::robot.rVelocity : -Remote::robot.rVelocity;

      display->drawLine(26, 31, 26, 31 - 16 * xV, WHITE);
      display->drawLine(28, 31, 28, 31 - 16 * yV, WHITE);
      display->drawLine(30, 31, 30, 31 - 16 * rV, WHITE);

    }else{

      display->setCursor(37, 8);
      display->printf("%i %i %i %i %i",
      Remote::ioDevices.leftLedButton.isButtonPressed(),
      Remote::ioDevices.leftPushButton.isButtonPressed(),
      Remote::ioDevices.speedToggleSwitch.getSwitchState(),
      Remote::ioDevices.modeToggleSwitch.getSwitchState(),
      Remote::ioDevices.rightPushButton.isButtonPressed());

      display->setCursor(0, 16);
      display->printf("X:%s%.3f", Remote::ioDevices.leftJoystick.getXValue() >= 0.0 ? "+" : "", Remote::ioDevices.leftJoystick.getXValue());
      display->setCursor(0, 24);
      display->printf("Y:%s%.3f", Remote::ioDevices.leftJoystick.getYValue() >= 0.0 ? "+" : "", Remote::ioDevices.leftJoystick.getYValue());
      display->setCursor(80, 16);
      display->printf("X:%s%.3f", Remote::ioDevices.rightJoystick.getXValue() >= 0.0 ? "+" : "", Remote::ioDevices.rightJoystick.getXValue());
      display->setCursor(80, 24);
      display->printf("Y:%s%.3f", Remote::ioDevices.rightJoystick.getYValue() >= 0.0 ? "+" : "", Remote::ioDevices.rightJoystick.getYValue());
    }  
      
    }
      */
      




/*
    display.setCursor(0,0);
    display.printf("Mi:%i/%i\n", successfullReceptions, failedReceptions);
    display.printf("Si:%i/%i\n", successfullSlaveTransmissions, failedSlaveReceptions);
    display.printf("Radio:%idb Batt:%i%%", lastRssi, batteryPercentInteger);
    display.setCursor(77, 0);
    display.printf("Tx:%iHz", int(transmissionFrequency.getFrequency()));
    display.setCursor(69, 8);
    display.printf("Rx:%i/%iHz", int(receptionFrequency.getFrequency()), int(decodingFrequency.getFrequency()));
    int width = map(lastRssi, -140, -30, 0, display.width() / 2);
    width = min(width, display.width() / 2.0);
    width = max(width, 0);
    display.fillRect(0, 24, width, 8, WHITE);
*/








}