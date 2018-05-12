#ifndef _Main_h
#define _Main_h

enum SensorTypes {
  temperature,
  distance,
  photoresistor,
  humidity
};

enum SensorModes {
  event,
  sampling
};

enum ThresholdModes {
  minimum,
  maximum,
  level
};

#endif
