#include <SoftwareSerial.h>
#include "Log.h"
#include "S25FLx.h"
#include "dht.h"

#define PHOTORESIS_PIN 1
#define DHT11_PIN 4
#define HALL_PIN 6
#define RX_PIN 8
#define TX_PIN 7

SoftwareSerial BLESerial(TX_PIN, RX_PIN); // TX, RX
dht DHT;

/** CONFIG VARIABLES */
// TODO: This would be saved in the Flash memory 
unsigned char type;
unsigned char sensorMode;
unsigned char tresholdMode;
unsigned short sensorTreshold;
unsigned long freqy;
byte tresholdFlag = 0;

/** SENSORS VARIABLES */
int phoVal;
int hallVal;
int dhtVal;
int tmpSensor;

/** FOR TIME VARIABLES */
unsigned long timeNow = 0;
unsigned long timeLast = 0;
unsigned char startingHour = 20;
unsigned char seconds = 50;
unsigned char minutes = 0;
unsigned char hours = 0;
unsigned char days = 0;

// BLE Serial
char recieveBuff;

/** TIMER INTERRUPT */
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
  Log.notice("******** CONFIG WIZARD: ********"CR);
  Log.notice("    TYPE: %d"CR, type);
  Log.notice("    SENSOR MODE: %d"CR, sensorMode);
  Log.notice("    SENSOR TRESHOLD VALUE: %d"CR, sensorTreshold);
  Log.notice("    SENSOR TRESHOLD MODE: %c"CR, tresholdMode);
  Log.notice("    FREQUENCY %d SECOND"CR, (freqy/1000));
  Log.notice("********************************"CR);
}

void sendConfig() {
  BLESerial.println("******** CONFIG WIZARD: ********"CR);
  BLESerial.print("    TYPE:");
  BLESerial.println(type);
  BLESerial.print("    SENSOR MODE:");
  BLESerial.println(sensorMode);
  BLESerial.print("    SENSOR TRESHOLD VALUE:");
  BLESerial.println(sensorTreshold);
  BLESerial.print("    FREQUENCY (IN SECONDS)");
  BLESerial.println((freqy/1000));
  BLESerial.println("********************************"CR);
}

void status(char status[]) {
  if (status = "ready") {
    BLESerial.println("STATUS:READY");
  }
}

void timelapsing() {
  timeNow = millis()/1000;
  seconds = timeNow - timeLast;
  
  if (seconds == 60) { // MINUTES
    timeLast = timeNow;
    minutes += 1;
  }
  
  if (minutes == 60){  // HOURS
    minutes = 0;
    hours += 1;
  }
  
  if (hours == 24){ // DAYS
    hours = 0;
    days += 1;
  }
  
    Serial.print("The time is:   ");
    Serial.print(days);
    Serial.print("/");
    Serial.print(hours);
    Serial.print(":");
    Serial.print(minutes);
    Serial.print(":");
    Serial.println(seconds);
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
        sendConfig();
        status("ready");
        break;
      case 'w':
        Log.notice(" SENSOR SET: TEMPERATURE"CR);
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
      case 'h':
        Log.notice(" SENSOR SET: HUMIDITY"CR);
        type = 3;
        status("ready");
        break;
      case 'e':
        Log.notice(" MODE: EVENT"CR);
        sensorMode = 2;
        status("ready");
        break;
      case 's':
        Log.notice(" MODE: SAMPLING"CR);
        sensorMode = 1;
        status("ready");
        break;
      case '-':
        Log.notice(" TRESHOLD MODE: EDGE"CR);
        tresholdMode = '-';
        BLESerial.println("The treshold mode is MIN");
        status("ready");
        break;
      case '+':
        Log.notice(" TRESHOLD  MODE: MAX"CR);
        tresholdMode = '+';
        BLESerial.println("The treshold mode is MAX");
        status("ready");
        break;
      case '/':
        Log.notice(" TRESHOLD  MODE: LEVEL CHANGE TRIGGERED"CR);
        tresholdMode = '/';
        BLESerial.println("The treshold mode is level change triggered.");
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
          case 0: // TEMPERATURE SENSOR SETUP
            DHT.read11(DHT11_PIN);
            sensorTreshold = DHT.temperature;
            BLESerial.print("Temperature: ");
            break;
          case 1: // DISTANCE SENSOR SETUP
            sensorTreshold = digitalRead(HALL_PIN);
            BLESerial.print("Hall sensor: ");
            break;
          case 2:  // LIGHT SENSOR SETUP
            sensorTreshold = analogRead(PHOTORESIS_PIN);
            BLESerial.print("Light: ");
            break;
          case 3: // HUMIDITY SENSOR SETUP
            DHT.read11(DHT11_PIN);
            sensorTreshold = DHT.humidity;
            BLESerial.print("Humidity: ");
            break;
          default:
            Log.warning("This sensor type is not defined yet"CR);
            break;
        }
        BLESerial.println(sensorTreshold);
        
        // The duplicated for the treshold placer
        BLESerial.print("SENSOR TRESHOLD:");
        BLESerial.println(sensorTreshold);
        status("ready");
        break;
      case 'r':
        if (sensorMode == 1) { //SAMPLING MODE
          configInfo();
          Log.notice("****** START SAMPLING MODE ******"CR);
          prevMillis = 0;
          while (recieveBuff != 'p') {
            currentMillis = millis();
            if (currentMillis - prevMillis >= freqy ) {
              prevMillis = currentMillis;
              Log.notice("Interrupt occured."CR);
              switch (type) {
                // TODO IMPLEMENT THE FLASH MEMORY SAVE 
                case 0: // TEMPERATURE SENSOR SETUP
                  DHT.read11(DHT11_PIN);
                  BLESerial.print("Temperature: ");
                  BLESerial.println(DHT.temperature);
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
                case 3: // HUMIDITY SENSOR SETUP
                  DHT.read11(DHT11_PIN);
                  BLESerial.print("Humidity: ");
                  BLESerial.println(DHT.humidity);
                  break;
              }
           }
           if (BLESerial.available()) {
              recieveBuff = BLESerial.read();  // BLE recieve buffer
           }
          }
          status("ready");
        } else if (sensorMode == 2) {
          Log.notice("****** START EVENT BASE MODE ******"CR);
          BLESerial.print("The treshold sensore value:");
          BLESerial.println(sensorTreshold);
          prevMillis = 0;
          while (recieveBuff != 'p') {
            currentMillis = millis();
            if (currentMillis - prevMillis >= freqy ) {
              prevMillis = currentMillis;
              // Log.notice("Interrupt occured."CR);
              switch (type) {
                case 0: // WEATHER SENSOR SETUP
                  DHT.read11(DHT11_PIN);
                  tmpSensor = DHT.temperature;
                  break;
                case 1: // DISTANCE SENSOR SETUP
                  tmpSensor = digitalRead(HALL_PIN);
                  break;
                case 2:  // LIGHT SENSOR SETUP
                  tmpSensor = analogRead(PHOTORESIS_PIN);
                  break;
                case 3:
                  DHT.read11(DHT11_PIN);
                  tmpSensor = DHT.humidity;
                  break;
              }

              switch(tresholdMode) {
                case '/':
                  if (tmpSensor < sensorTreshold && tresholdFlag == 0) {
                    Serial.println("SENSOR < TRESHOLD");
                    tresholdFlag = 1;
                  } else if (tmpSensor >= sensorTreshold && tresholdFlag == 1) {
                    Serial.println("SENSOR > TRESHOLD");
                    tresholdFlag = 0;
                  } else {
                    Serial.println("No changes...");
                    }
                  break;
                case '+':
                  if (tmpSensor > sensorTreshold) {
                    BLESerial.print("[MAX]: The sensor value: ");
                    BLESerial.println(tmpSensor);
                  }
                  break;
                case '-':
                  if (tmpSensor < sensorTreshold) {
                    BLESerial.print("[MIN]: The sensor value: ");
                    BLESerial.println(tmpSensor);
                  }
                  break;
                default:
                  BLESerial.print("There is no such a treshold mode.");
                  Log.notice("WARNING: TRESHOLD MODE NOT FOUND"CR);
                  break;
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
