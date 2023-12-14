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

    struct Configuration{
        float bandwidthKHz = 125.0;
        int spreadingFactor = 7;
    };

    void initialize(Configuration config);
    void update();

}