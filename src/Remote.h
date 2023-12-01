#pragma once

#include "RemoteIODevices.h"
#include "Display.h"
#include "Radio.h"
#include "Robot.h"

#include <vector>

namespace Remote{

    extern Display display;
    extern RemoteIODevices ioDevices;
    extern Radio radio;
    extern Robot robot;

    void initialize();
    void update();

}