#include <SoftwareSerial.h>
#include "Log.h"
#include "main.h"
#include "dht.h"

#define PHOTORESIS_PIN 1
#define DHT11_PIN 4
#define HALL_PIN 6
#define RX_PIN 8
#define TX_PIN 7

SoftwareSerial BLESerial(TX_PIN, RX_PIN); // TX, RX
dht DHT;

/** CONFIG VARIABLES */
int type;
int mode;
unsigned short sensorTreshold;
unsigned long freqy;

/** SENSORS VARIABLES */
int phoVal;
int hallVal;
int dhtVal;
int tmpSensor;

// BLE Serial
char recieveBuff;

// for time interrupt
unsigned long currentMillis;
unsigned long prevMillis;

void setup() {
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Serial.begin(9600);
  BLESerial.begin(9600);

  configInfo();

  pinMode(PHOTORESIS_PIN, INPUT);
  pinMode(HALL_PIN, INPUT);
}

void configInfo() {
  Log.notice("CONFIG WIZARD:"CR);
  Log.notice("    TYPE: %d"CR, type);
  Log.notice("    MODE: %d\n\n"CR, mode);
}

void status(char status[]) {
  if (status = "ready") {
    BLESerial.println("STATUS:READY");
  }
}

void readBLE() {
  if (BLESerial.available()) {
    recieveBuff = BLESerial.read();  // BLE recieve buffer
    Log.notice(" RECIEVED BUFFER: %c"CR, recieveBuff);
  }
}

void loop() {
  if (BLESerial.available()) {
    recieveBuff = BLESerial.read();  // BLE recieve buffer

    Log.notice("[RECIEVED BUFFER: %c]"CR, recieveBuff);

    switch (recieveBuff) {
      case 'c':
        configInfo();
        status("ready");
        break;
      case 'w':
        Log.notice(" SENSOR SET: WEATHER"CR);
        type = 0;
        status("ready");
        break;
      case 'd':
        Log.notice(" SENSOR SET: DISTANCE"CR);
        type = 1;
        status("ready");
        break;
      case 'l':
        Log.notice(" SENSOR SET: LIGHT"CR);
        type = 2;
        status("ready");
        break;
      case 'e':
        Log.notice(" MODE: EVENT"CR);
        mode = 2;
        status("ready");
        break;
      case 's':
        Log.notice(" MODE: SAMPLING"CR);
        mode = 1;
        status("ready");
        break;
      case 'T':
        freqy = 0;
        recieveBuff = "";
        // recieve frequency -> magic parser 
        while(recieveBuff != ':'){
          readBLE();
          if(recieveBuff == ':'){
            break;
          }
          if((recieveBuff - '0') == 0) {
            freqy *= 10;
          } else {
            freqy += (recieveBuff - '0');
          }
        }
        Log.notice(" FREQUENCY: %d"CR, freqy);
        status("ready");
        break;
      case 'p':
        Log.notice(" PROGRAM STOPPED"CR);
        status("ready");
        break;
      case 'q':
        Log.notice("TRESHOLD SAMPLING IS PROCCESED"CR);
        switch(type) {
          case 0: // WEATHER SENSOR SETUP
            dhtVal = DHT.read11(DHT11_PIN);
            BLESerial.print("Temperature: ");
            BLESerial.println(DHT.temperature);
            BLESerial.print("Humidity: ");
            BLESerial.println(DHT.humidity);
            break;
          case 1: // DISTANCE SENSOR SETUP
            hallVal = digitalRead(HALL_PIN);
            BLESerial.print("Hall sensor: ");
            BLESerial.println(hallVal);
            break;
          case 2:  // LIGHT SENSOR SETUP
            sensorTreshold = analogRead(PHOTORESIS_PIN);
            BLESerial.print("Light: ");
            BLESerial.println(sensorTreshold);
            break;
          default:
            Log.warning("This sensor type is not defined yet"CR);
            break;
        }
        status("ready");
        break;
      case 'r':
        if (mode == 1) { //SAMPLING MODE
          configInfo();
          Log.notice("****** START SAMPLING MODE ******"CR);
          prevMillis = 0;
          while (recieveBuff != 'p') {
            currentMillis = millis();
            if (currentMillis - prevMillis >= freqy ) {
              prevMillis = currentMillis;
              Log.notice("Interrupt occured."CR);
              switch (type) {
                case 0: // WEATHER SENSOR SETUP
                  dhtVal = DHT.read11(DHT11_PIN);
                  BLESerial.print("Temperature: ");
                  BLESerial.println(DHT.temperature);
                  BLESerial.print("Humidity: ");
                  BLESerial.println(DHT.humidity);
                  break;
                case 1: // DISTANCE SENSOR SETUP
                  hallVal = digitalRead(HALL_PIN);
                  BLESerial.print("Hall sensor: ");
                  BLESerial.println(hallVal);
                  break;
                case 2:  // LIGHT SENSOR SETUP
                  phoVal = analogRead(PHOTORESIS_PIN);
                  BLESerial.print("Light: ");
                  BLESerial.println(phoVal);
                  break;
              }
           }
           if (BLESerial.available()) {
              recieveBuff = BLESerial.read();  // BLE recieve buffer
           }
          }
          status("ready");
        } else if (mode == 2) {
          configInfo();
          Log.notice("****** START EVENT BASE MODE ******"CR);
          BLESerial.print("The treshold sensore value:");
          BLESerial.println(sensorTreshold);
          prevMillis = 0;
          while (recieveBuff != 'p') {
            currentMillis = millis();
            if (currentMillis - prevMillis >= freqy ) {
              prevMillis = currentMillis;
              Log.notice("Interrupt occured."CR);
              switch (type) {
                case 0: // WEATHER SENSOR SETUP
                  tmpSensor = DHT.read11(DHT11_PIN);
                  //BLESerial.print("Temperature: ");
                  break;
                case 1: // DISTANCE SENSOR SETUP
                  tmpSensor = digitalRead(HALL_PIN);
                  break;
                case 2:  // LIGHT SENSOR SETUP
                  tmpSensor = analogRead(PHOTORESIS_PIN);
                  break;
              }

              if (tmpSensor > sensorTreshold) {
                BLESerial.println("The sensor value occured the treshold maximum: ");
                BLESerial.println(tmpSensor);
              }
            }
            
            if (BLESerial.available()) {
                recieveBuff = BLESerial.read();  // BLE recieve buffer
            }
          }
         status("ready");
        } else {
          BLESerial.print("Other modes not ready now...");
        }
        break;
    default:
        Log.notice(" DEFAULT RECIEVED BUFFER: %c"CR, recieveBuff);
        break;
    }
  }
  recieveBuff = "";
}
