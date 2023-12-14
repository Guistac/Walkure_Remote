#include "Remote.h"


Remote::Configuration bed_remote = {
  .bandwidthKHz = 250.0,
  .spreadingFactor = 7
};

Remote::Configuration desk_remote = {
  .bandwidthKHz = 125.0,
  .spreadingFactor = 7
};

Remote::Configuration fridge_remote = {
  .bandwidthKHz = 125.0,
  .spreadingFactor = 7
};

Remote::Configuration closet_remote = {
  .bandwidthKHz = 125.0,
  .spreadingFactor = 7
};


void setup(){  
  Remote::initialize(bed_remote);
  //Remote::initialize(desk_remote);
  //Remote::initialize(fridge_remote);
  //Remote::initialize(closet_remote);
}

void loop(){
  Remote::update();
}