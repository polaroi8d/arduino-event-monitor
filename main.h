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

/** TIME */
struct Time {
  uint8_t seconds = 0;
  uint8_t minutes = 0;
  uint8_t hours = 0;
  uint8_t days = 0;
  uint8_t months = 0;
};
#endif
