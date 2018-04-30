#include <SoftwareSerial.h>
#include <SPI.h>
#include "src/S25FLx.h"
#include "src/dht.h"

#define PHOTORESIS_PIN 1
#define DHT11_PIN 4
#define HALL_PIN 6
#define RX_PIN 8
#define TX_PIN 7
#define PAGE_SIZE 256

SoftwareSerial BLESerial(TX_PIN, RX_PIN); // TX, RX
dht DHT;
flash FLASH;  //starts flash class and initilzes SPI

/** CONFIG */
// TODO: This would be saved in the Flash memory 
unsigned char type;
unsigned char sensorMode;
unsigned char tresholdMode;
unsigned short sensorTreshold;
unsigned long freqy;
byte tresholdFlag = 0;

/** SENSORS  */
int phoVal;
int hallVal;
int dhtVal;
int tmpSensor;

/** TIME */
unsigned long timeNow = 0;
unsigned long timeLast = 0;
unsigned char startingHour = 0;
unsigned char seconds = 0;
unsigned char minutes = 0;
unsigned char hours = 0;
unsigned char days = 0;
boolean tized;

/** FLASH MEMORY */
uint8_t writeBuffer[PAGE_SIZE];
uint8_t readBuffer[PAGE_SIZE];
int memoryIndex = 0;
int memoryPage = 0;

// BLE Serial
char recieveBuff;

/** TIMER INTERRUPT */
unsigned long currentMillis;
unsigned long prevMillis;

void setup() {
  Serial.begin(9600);
  BLESerial.begin(9600);
  delay(250);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  FLASH.waitforit(); // WAIT FOR INIT THE FLASH

  pinMode(PHOTORESIS_PIN, INPUT);
  pinMode(HALL_PIN, INPUT);
  pinMode(CS, OUTPUT);  // CHIP SELECT PIN MODE
  FLASH.erase_all();
  FLASH.waitforit(); // WAIT FOR INIT THE FLASH
  Serial.println(F("MIC is ready to use"));
}

void status(char status[]) {
  if (status = "ready") {
    BLESerial.println(F("STATUS:READY"));
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
  
    Serial.print(F("The time is:   "));
    Serial.print(days);
    Serial.print(F("/"));
    Serial.print(hours);
    Serial.print(F(":"));
    Serial.print(minutes);
    Serial.print(F(":"));
    Serial.println(seconds);
  }

void readBLE() {
  if (BLESerial.available()) {
    recieveBuff = BLESerial.read();  // BLE recieve buffer
    Serial.print(F(" RECIEVED BUFFER: "));
    Serial.println(recieveBuff);
  }
}

void loop() {
  int pageReadCounter = 0;
  
  if (BLESerial.available()) {
    recieveBuff = BLESerial.read();  // BLE recieve buffer
    Serial.print(F("[RECIEVED BUFFER]: "));
    Serial.println(recieveBuff);
    int j = 0;
    switch (recieveBuff) {
      case '_':
        Serial.println(F("Bluetooth is connected, ready to communicate."));
        status("ready");
        break;
      case 'c':
        BLESerial.println(F("******** CONFIG WIZARD: ********"));
        BLESerial.print(F("    TYPE:"));
        BLESerial.println(type);
        BLESerial.print(F("    SENSOR MODE:"));
        BLESerial.println(sensorMode);
        BLESerial.print(F("    SENSOR TRESHOLD VALUE:"));
        BLESerial.println(sensorTreshold);
        BLESerial.print(F("    FREQUENCY (IN SECONDS)"));
        BLESerial.println((freqy / 1000));
        BLESerial.println(F("********************************"));
        Serial.println(F("******** CONFIG WIZARD: ********"));
        Serial.print(F("    TYPE: "));
        Serial.println(type);
        Serial.print(F("    SENSOR MODE: "));
        Serial.println(sensorMode);
        Serial.print(F("    SENSOR TRESHOLD VALUE: "));
        Serial.println(sensorTreshold);
        Serial.print(F("    SENSOR TRESHOLD MODE: "));
        Serial.println(tresholdMode);
        Serial.print(F("    FREQUENCY: "));
        Serial.println(freqy / 1000);
        Serial.print(F("    USED MEMORY INDEX: "));
        Serial.println(memoryIndex);
        Serial.print(F("    USED MEMORY PAGE: "));
        Serial.println(memoryPage);
        Serial.print(F("    PAGE READ COUNTER: "));
        Serial.println(pageReadCounter);
        Serial.println(F("********************************"));
        status("ready");
        break;
      case 'w':
        Serial.println(F(" SENSOR SET: TEMPERATURE"));
        type = 0;
        status("ready");
        break;
      case 'd':
        Serial.println(F("SENSOR SET: DISTANCE"));
        type = 1;
        status("ready");
        break;
      case 'l':
        Serial.println(F("SENSOR SET: LIGHT"));
        type = 2;
        status("ready");
        break;
      case 'h':
        Serial.println(F("SENSOR SET: HUMIITY"));
        type = 3;
        status("ready");
        break;
      case 'e':
        Serial.println(F("MODE: EVENT"));
        sensorMode = 2;
        status("ready");
        break;
      case 's':
        Serial.println(F("MODE: SAMPLING"));
        sensorMode = 1;
        status("ready");
        break;
      case '-':
        Serial.println(F("TRESHOLD MODE: EDGE"));
        tresholdMode = '-';
        BLESerial.println(F("The treshold mode is MIN"));
        status("ready");
        break;
      case '+':
        Serial.println(F("TRESHOLD MODE: MAX"));
        tresholdMode = '+';
        BLESerial.println(F("The treshold mode is MAX"));
        status("ready");
        break;
      case '/':
        Serial.println(F("TRESHOLD MODE: LEVEL CHANGE TRIGGERED"));
        tresholdMode = '/';
        BLESerial.println(F("The treshold mode is level change triggered."));
        status("ready");
        break;
      case 'E':
        FLASH.erase_all();
        Serial.println(F("Ereased the flash memory."));
        BLESerial.println(F("Flash memory was ereased. Ready to write."));
        status("ready");
        break;
      case 'K':
        BLESerial.print("DATA:TRANSFER");
        memset(readBuffer, 0, 255);
        for(pageReadCounter=0; pageReadCounter<memoryPage; pageReadCounter++) {
          FLASH.read((pageReadCounter*PAGE_SIZE), readBuffer, PAGE_SIZE);
          FLASH.waitforit();
          Serial.print(F("pageCounter: "));
          Serial.print(pageReadCounter);
          Serial.print(F(" | memoryPage: "));
          Serial.print(memoryPage);
          Serial.print(" | (pageReadCounter*PAGE_SIZE): ");
          Serial.println((pageReadCounter*PAGE_SIZE));
          for(j=0;j<PAGE_SIZE;j++) {
            Serial.print(F("BUFFER["));
            Serial.print(j);
            Serial.print(F("]: "));
            Serial.println(readBuffer[j]);
            BLESerial.print("D:");
            BLESerial.println(readBuffer[j]);
            delay(50);
          }
        }
        if (memoryIndex != 0) {
          FLASH.read((pageReadCounter*PAGE_SIZE), readBuffer, memoryIndex);
          FLASH.waitforit();
          Serial.println(F("Reminder transfer:"));
          for(j=0;j<=memoryIndex;j++) {
            Serial.print(F("Export the BUFFER["));
            Serial.print(j);
            Serial.print(F("]: "));
            Serial.println(readBuffer[j]);
            BLESerial.print("D:");
            BLESerial.println(readBuffer[j]);
            delay(50);
          }
        }
        pageReadCounter = 0;
        Serial.println(F("Data transfer was finished."));
        BLESerial.print("DATA:END");
        status("ready");
        break;
      case 'C': // CLOCK TIME PARSING
        tized = true;
        while(1){
          readBLE();
          if(recieveBuff == '/') { break; }
          if (tized) {
            startingHour = (recieveBuff - '0') * 10;  //convert char to digit
            tized = false;
          } else { startingHour += (recieveBuff - '0'); }
        }
        tized = true;
        while (1){
          readBLE();
          if(recieveBuff == ':') { break; }
          if (tized) {
            minutes = (recieveBuff - '0') * 10;  //convert char to digit
            tized = false;
          } else { minutes += (recieveBuff - '0'); }
        }
        status("ready");
        BLESerial.println(F("Time is configured."));
        Serial.print(F("Time is configured:"));
        Serial.print(startingHour);
        Serial.print(F(":"));
        Serial.println(minutes);
        break;
      case 'T':
        freqy = 0;
        recieveBuff = "";
        // recieve frequency -> magic parser
        while (recieveBuff != ':') {
          readBLE();
          if (recieveBuff == ':') {
            break;
          }
          if ((recieveBuff - '0') == 0) {
            freqy *= 10;
          } else {
            freqy += (recieveBuff - '0');
          }
        }
        Serial.print(F("FREQUENCY: "));
        Serial.println(freqy);
        status("ready");
        break;
      case 'p':
        Serial.println(F("PROGRAM STOPPED"));
        status("ready");
        break;
      case 'q':
        Serial.println(F("TRESHOLD SAMPING IS PROCESSED"));
        switch (type) {
          case 0: // TEMPERATURE SENSOR SETUP
            DHT.read11(DHT11_PIN);
            sensorTreshold = DHT.temperature;
            BLESerial.print(F("Temperature: "));
            break;
          case 1: // DISTANCE SENSOR SETUP
            sensorTreshold = digitalRead(HALL_PIN);
            BLESerial.print(F("Hall sensor: "));
            break;
          case 2:  // LIGHT SENSOR SETUP
            sensorTreshold = analogRead(PHOTORESIS_PIN);
            BLESerial.print(F("Light: "));
            break;
          case 3: // HUMIDITY SENSOR SETUP
            DHT.read11(DHT11_PIN);
            sensorTreshold = DHT.humidity;
            BLESerial.print(F("Humidity: "));
            break;
          default:
            Serial.println(F("This sensor type is not defined yet"));
            break;
        }
        BLESerial.println(sensorTreshold);

        // The duplicated for the treshold placer
        BLESerial.print(F("SENSOR:TRESHOLD"));
        BLESerial.println(sensorTreshold);
        status("ready");
        break;
      case 'r':
        if (sensorMode == 1) { //SAMPLING MODE
          Serial.println(F("****** START SAMPLING MODE ******"));
          prevMillis = 0;
          while (recieveBuff != 'p') {
            currentMillis = millis();
            if (currentMillis - prevMillis >= freqy ) {
              prevMillis = currentMillis;
              Serial.println(F("Interrupt occured."));
              switch (type) {
                case 0: // TEMPERATURE SENSOR SETUP
                  DHT.read11(DHT11_PIN);
                  writeBuffer[memoryIndex] = DHT.temperature;
                  BLESerial.print(F("Temperature: "));
                  BLESerial.println(writeBuffer[memoryIndex]);
                  break;
                case 1: // DISTANCE SENSOR SETUP
                  writeBuffer[memoryIndex] = digitalRead(HALL_PIN);
                  BLESerial.print(F("Hall sensor: "));
                  BLESerial.println(writeBuffer[memoryIndex]);
                  break;
                case 2:  // LIGHT SENSOR SETUP
                  writeBuffer[memoryIndex] = (analogRead(PHOTORESIS_PIN)/2.8); // need it for to scale
                  phoVal = analogRead(PHOTORESIS_PIN);
                  BLESerial.print(F("Light: "));
                  BLESerial.println(writeBuffer[memoryIndex]);
                  break;
                case 3: // HUMIDITY SENSOR SETUP
                  DHT.read11(DHT11_PIN);
                  writeBuffer[memoryIndex] = DHT.humidity;
                  BLESerial.print(F("Humidity: "));
                  BLESerial.println(writeBuffer[memoryIndex]);
                  break;
              }
              if (memoryIndex == PAGE_SIZE-1) { // PAGE_SIZE + 1
                FLASH.write((memoryPage*PAGE_SIZE), writeBuffer, PAGE_SIZE);
                FLASH.waitforit();
                Serial.println(F("1 page was written on the Flash memory."));
                BLESerial.println(F("1 page was written on the Flash memory."));
                memoryIndex = 0;
                memoryPage++;
              } else {
                memoryIndex++;
              }
            }
            if (BLESerial.available()) {
              recieveBuff = BLESerial.read();  // BLE recieve buffer
            }
          }
          status("ready");
        } else if (sensorMode == 2) {
          Serial.println(F("****** START EVENT BASE MODE ******"));
          BLESerial.print(F("The treshold sensore value:"));
          BLESerial.println(sensorTreshold);
          prevMillis = 0;
          while (recieveBuff != 'p') {
            currentMillis = millis();
            if (currentMillis - prevMillis >= freqy ) {
              prevMillis = currentMillis;
              Serial.println(F("Interrupt occured."));
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

              switch (tresholdMode) {
                case '/':
                  if (tmpSensor < sensorTreshold && tresholdFlag == 0) {
                    Serial.println(F("SENSOR < TRESHOLD"));
                    tresholdFlag = 1;
                  } else if (tmpSensor >= sensorTreshold && tresholdFlag == 1) {
                    Serial.println(F("SENSOR > TRESHOLD"));
                    tresholdFlag = 0;
                  } else {
                    Serial.println(F("No changes..."));
                  }
                  break;
                case '+':
                  if (tmpSensor > sensorTreshold) {
                    BLESerial.print(F("[MAX]: The sensor value: "));
                    BLESerial.println(tmpSensor);
                  }
                  break;
                case '-':
                  if (tmpSensor < sensorTreshold) {
                    BLESerial.print(F("[MIN]: The sensor value: "));
                    BLESerial.println(tmpSensor);
                  }
                  break;
                default:
                  BLESerial.print(F("There is no such a treshold mode."));
                  Serial.println(F("Warning: Treshold mode not found!"));
                  break;
              }
            }

            if (BLESerial.available()) {
              recieveBuff = BLESerial.read();  // BLE recieve buffer
            }
          }
          status("ready");
        } else {
          BLESerial.print(F("Other modes not ready now..."));
        }
        break;
      default:
        Serial.print(F("DEFAULT RECIEVED BUFFER: "));
        Serial.println(recieveBuff);
        break;
    }
  }
  recieveBuff = "";
}
