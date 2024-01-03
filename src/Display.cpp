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
    if(val == 0.0) display->drawPixel(indicator, y, BLACK);
  };

  drawJoystick(x+8, y+1, 10, Remote::ioDevices.leftJoystick.getXValue());
  drawJoystick(x+20, y+1, 10, Remote::ioDevices.leftJoystick.getYValue());
  drawJoystick(x+32, y+1, 10, Remote::ioDevices.rightJoystick.getXValue());
  drawJoystick(x+44, y+1, 10, Remote::ioDevices.rightJoystick.getYValue());

}

void Display::drawMecanumWheel(uint32_t& animationOffset, int x, int y, int w, int h, bool o, float v, bool alm, bool ena){

  display->drawRect(x, y, w, h, WHITE);

  ena = true;
  alm = false;

  if(alm){
    if(millis() % 500 > 250) return;
    display->drawLine(x+2, y+3, x+2, y+h-3, WHITE);
    display->drawLine(x+w-3, y+3, x+w-3, y+h-3, WHITE);
    display->drawLine(x+3, y+2, x+w-4, y+2, WHITE);
    display->drawLine(x+2, y+(w/2)+1, x+w-3, y+(w/2)+1, WHITE);
    animationOffset = UINT32_MAX / 2;
    return;
  }
  else if(!ena){
    display->drawLine(x+2, y+2, x+w-3, y+h-3, WHITE);
    display->drawLine(x+w-3, y+2, x+2, y+h-3, WHITE);
    animationOffset = UINT32_MAX / 2;
    return;
  }

  int increment = v * 200;
  animationOffset += increment;

  int spacing = 6;
  int offset = (animationOffset / 100) % spacing;

  if(o){
    for(int i = y - w + offset + 1; i < y + h; i += spacing){
      int yStart = i;
      int yEnd = i + h - 1;
      int xStart = x;
      int xEnd = x + h - 1;
      
      if(yStart < y){
        xStart += y - yStart;
        yStart = y;
      }
      if(yEnd > y + h - 1){
        xEnd -= yEnd - y - h + 1;
        yEnd = y + h - 1;
      }
      if(xEnd > x + w - 1){
        yEnd -= xEnd - x - w + 1;
        xEnd = x + w - 1;
      }
      display->drawLine(xStart, yStart, xEnd, yEnd, WHITE);
    }
  }
  else{
    for(int i = y + offset; i < y + h + w - 1; i += spacing){
      int yStart = i;
      int yEnd = i - h;
      int xStart = x;
      int xEnd = x + h;
      if(yStart > y + h - 1){
        xStart += yStart - y - h + 1;
        yStart = y + h - 1;
      }
      if(yEnd < y){
        xEnd -= y - yEnd;
        yEnd = y;
      }
      if(xEnd > x + w - 1){
        yEnd += xEnd - x - w + 1;
        xEnd = x + w - 1;
      }
      display->drawLine(xStart, yStart, xEnd, yEnd, WHITE);
    }
  }

  

}

void Display::onUpdate(){

  display->clearDisplay();

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
      }
    }



    display->setCursor(50, 0);
    display->printf("%i%%", int(Remote::ioDevices.batteryReading.getLevel() * 100.0));

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

    float upLinkQuality = map(float(Remote::robot.robotRxSignalStrength), -120, -30, 0.0, 1.0);
    float downLinkQuality = map(float(Remote::robot.remoteRxSignalStrength), -120, -30, 0.0, 1.0);

    display->setCursor(0,11);
    display->printf("U");
    horizontalProgressBar(8, 11, 46, 6, upLinkQuality);
    display->setCursor(0,20);
    display->printf("D");
    horizontalProgressBar(8, 20, 46, 6, downLinkQuality);

    drawInputDeviceState(display, 0, 29);

    Vector2 rectMin(56,8);
    Vector2 rectSize(8, 24);

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

  auto drawMoveDirection = [&](int x, int y, int s, float v, float h, float r){
    //display->drawRect(x,y,s,s,WHITE);
    float cx = x + float(s) * 0.5;
    float cy = y + float(s) * 0.5;
    float dx = cx + v * s / 2.0;
    float dy = cy + h * s / 2.0;
    display->drawLine(int(cx), int(cy), int(dx), int(dy), WHITE);
    display->fillCircle(dx, dy, 2, WHITE);

    float rad = s / 2.0;
    int maxDiv = 64;
    float anglePerDiv = 180.0 / maxDiv;
    int divisions = r * maxDiv;
    if(divisions < 0) divisions = -divisions;
    Vector2 points[divisions + 1];
    if(r > 0){
      for(int i = 0; i <= divisions; i++){
        float angle = (90.0 - i * anglePerDiv) * PI / 180.0;

        points[i] = Vector2(cx + rad * cos(angle), cy + rad * sin(angle));
      }
    }
    else{
      for(int i = 0; i <= divisions; i++){
        float angle = (90.0 + i * anglePerDiv) * PI / 180.0;
        points[i] = Vector2(cx + rad * cos(angle), cy + rad * sin(angle));
      }
    }
    for(int i = 0; i < divisions; i++){
      display->drawLine(points[i].x, points[i].y, points[i+1].x, points[i+1].y, WHITE);
    }
    display->fillCircle(points[divisions].x, points[divisions].y, 1, WHITE);
  };

  if(Remote::robot.b_connected)
    drawMoveDirection(72, 0, 32, Remote::robot.xVelocity, Remote::robot.yVelocity, Remote::robot.rVelocity);
  else {
    drawMoveDirection(72, 0, 32, sin(float(millis()) / 1000.0), cos(float(millis()) / 1000.0), sin(float(millis()) / 1000.0));
  }
  drawMecanumWheel(fl_offset, 105, 0, 10, 15, true, Remote::robot.fl_vel, Remote::robot.frontLeft_alarm, Remote::robot.frontLeft_enabled);
  drawMecanumWheel(fr_offset, 117, 0, 10, 15, false, Remote::robot.fr_vel, Remote::robot.frontRight_alarm, Remote::robot.frontRight_enabled);
  drawMecanumWheel(bl_offset, 105, 17, 10, 15, false, Remote::robot.bl_vel, Remote::robot.backLeft_alarm, Remote::robot.backLeft_enabled);
  drawMecanumWheel(br_offset, 117, 17, 10, 15, true, Remote::robot.br_vel, Remote::robot.backRight_alarm, Remote::robot.backRight_enabled);

}