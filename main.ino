#include <SoftwareSerial.h>
#include "src/Time.h"
#include <SPI.h>
#include "src/S25FLx.h"
#include "src/dht.h"

#define PHOTORESIS_PIN 1
#define DHT11_PIN 4
#define HALL_PIN 6
#define RX_PIN 8
#define TX_PIN 7
#define PAGE_SIZE 255

#define YEAR 18
#define MONTH 05
#define DAY 05

SoftwareSerial BLESerial(TX_PIN, RX_PIN); // TX, RX
dht DHT;
flash FLASH;  //starts flash class and initilzes SPI

/** CONFIG */
unsigned char type;
unsigned char sensorMode;
unsigned char tresholdMode;
uint8_t sensorTreshold;
unsigned long freqy;
byte tresholdFlag = 0;
uint8_t tmpSensor;

/** TIME */
unsigned long timeNow = 0;
unsigned long timeLast = 0;
uint8_t seconds = 0;
uint8_t minutes = 0;
uint8_t hours = 0;
uint8_t days = 0;
uint8_t months = 0;
boolean writerFlag = false;
boolean memoryIndexOverFlowFlag = false;
boolean tized;

/** FLASH MEMORY */
uint8_t writeBuffer[PAGE_SIZE + 1];
uint8_t readBuffer[PAGE_SIZE + 1];
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
  pinMode(CS, OUTPUT);  // CHIP SELECT PIN MOD

  Serial.println(F("MIC is ready to use"));
}

void status(char status[]) {
  if (status = "ready") {
    BLESerial.println(F("STATUS:READY"));
  }
}

void printConfig() {
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
  Serial.println(F("********************************"));
  status("ready");
}

void printTime() {
    Serial.print("Time is: ");
    Serial.print(year()); Serial.print(F("/"));
    Serial.print(month()); Serial.print(F("/"));
    Serial.print(day()); Serial.print(F(" "));
    Serial.print(hour()); Serial.print(F(":"));
    Serial.print(minute()); Serial.print(F(":"));
    Serial.println(second());
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
      case '_':  // BLUETOOTH READY CONNECTION
        Serial.println(F("Bluetooth is connected, ready to communicate."));
        status("ready");
        break;
      case 'c':  // CONFIG PRINT
        printConfig();
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
        Serial.println(F("TRESHOLD MODE: MIN"));
        tresholdMode = 1;
        BLESerial.println(F("The treshold mode is MIN"));
        status("ready");
        break;
      case '+':
        Serial.println(F("TRESHOLD MODE: MAX"));
        tresholdMode = 2;
        BLESerial.println(F("The treshold mode is MAX"));
        status("ready");
        break;
      case '/':
        Serial.println(F("TRESHOLD MODE: LEVEL CHANGE TRIGGERED"));
        tresholdMode = 3;
        BLESerial.println(F("The treshold mode is level change triggered."));
        status("ready");
        break;
      case 'E':  // Format the Flash memory
        FLASH.erase_all();
        memoryIndex = 0;
        memoryPage = 0;
        pageReadCounter = 0;
        Serial.println(F("Ereased the flash memory."));
        BLESerial.println(F("Flash memory was ereased. Ready to write."));
        status("ready");
        break;
      case 'K':
        BLESerial.print("DATA:TRANSFER");
        if (memoryPage == 0 && memoryIndex == 0) {
          BLESerial.println("No data found!");
        }
        for(pageReadCounter=0; pageReadCounter<memoryPage; pageReadCounter++) {
          if (memoryPage == 0) {
            FLASH.read(0, writeBuffer, PAGE_SIZE);
          } else if (!memoryPage == 0) {
            FLASH.read(((pageReadCounter*PAGE_SIZE)+pageReadCounter), readBuffer, PAGE_SIZE);
          }
          FLASH.waitforit();

          Serial.print(F("pageCounter: ")); Serial.print(pageReadCounter);
          Serial.print(F(" | memoryPage: ")); Serial.print(memoryPage);
          Serial.print(" | start location:"); Serial.println((pageReadCounter*PAGE_SIZE)+pageReadCounter);
          for(j=0; j<PAGE_SIZE; j++) {
            Serial.print(F("Export the readBuffer[")); Serial.print(j); Serial.print(F("]: ")); Serial.println(readBuffer[j]);
            BLESerial.print("D:");
            BLESerial.println(readBuffer[j]);
            delay(50);
          }
        }

        if (memoryIndex != 0) {
          FLASH.read((pageReadCounter*PAGE_SIZE), readBuffer, memoryIndex);
          FLASH.waitforit();
          Serial.println(F("Reminder transfer:"));
          for(j=0; j<memoryIndex; j++) {
            Serial.print(F("Export the remaining readBuffer[")); Serial.print(j); Serial.print(F("]: ")); Serial.println(readBuffer[j]);
            BLESerial.print("D:");
            BLESerial.println(readBuffer[j]);
            delay(50);
          }
        }
        pageReadCounter = 0; // set defult paramaters
        Serial.println(F("Data transfer was finished."));
        BLESerial.print("DATA:END");
        status("ready");
        break;
      case 'C': // SAMPLING TIME PARSER
        tized = true;
        while(1){
          readBLE();
          if(recieveBuff == '/') { break; }
          if (tized) {
            hours = (recieveBuff - '0') * 10;  //convert char to digit
            tized = false;
          } else { hours += (recieveBuff - '0'); }
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
        setTime(hours, minutes, 0, DAY, MONTH, YEAR);
        BLESerial.println(F("Time is configured."));
        Serial.print(F("Time is configured:"));
        printTime();
        break;
      case 'T': // CLOCK CONFIG PARSER
        freqy = 0;
        recieveBuff = "";
        // recieve frequency -> magic parser
        while (recieveBuff != ':') {
          readBLE();
          if (recieveBuff == ':') { break; }

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
      case 'Q':
        Serial.println(F("GET THE CUSTOM TRESHOLD VALUE"));
        sensorTreshold = 0; // Delete the optional sensor value
        readBLE();
        while (recieveBuff != ':') {
          sensorTreshold = 10 * sensorTreshold + (recieveBuff - '0');
          readBLE();
        }
        Serial.print(F("Treshold configured to: "));
        Serial.println(sensorTreshold);
        status("ready");
        break;
      case 'q':
        Serial.println(F("TRESHOLD SAMPLING IS PROCESSED"));
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
            sensorTreshold = map(analogRead(PHOTORESIS_PIN), 0, 800, 0, 255);
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

        // The duplicated for the treshold placer in Client application
        BLESerial.print(F("SENSOR:TRESHOLD/"));
        BLESerial.println(sensorTreshold);
        status("ready");
        break;
      case 'r':
        memoryIndex = 0;  // Set defult paramaters
        memoryPage = 0;   // Set defult paramaters
        if (sensorMode == 1) { //SAMPLING MODE
          Serial.println(F("****** START SAMPLING MODE ******"));
          prevMillis = 0;
          while (recieveBuff != 'p') {
            currentMillis = millis();
            if (currentMillis - prevMillis >= freqy ) {
              prevMillis = currentMillis;
              Serial.println(F("Debug log: Interrupt occured."));
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
                  writeBuffer[memoryIndex] = map(analogRead(PHOTORESIS_PIN), 0, 800, 0, 255);
                  BLESerial.print(F("SAMPLING MODE Light: "));
                  BLESerial.println(writeBuffer[memoryIndex]);
                  break;
                case 3: // HUMIDITY SENSOR SETUP
                  DHT.read11(DHT11_PIN);
                  writeBuffer[memoryIndex] = DHT.humidity;
                  BLESerial.print(F("Humidity: "));
                  BLESerial.println(writeBuffer[memoryIndex]);
                  break;
              }
              if (memoryIndex == PAGE_SIZE-1) {
                if (memoryPage == 0) {
                  FLASH.write(0, writeBuffer, PAGE_SIZE);
                  FLASH.waitforit();
                } else if (!memoryPage == 0) {
                  FLASH.write(((memoryPage*PAGE_SIZE)+memoryPage), writeBuffer, PAGE_SIZE);
                  FLASH.waitforit();
                  Serial.print(F("starter location:" ));
                  Serial.println((memoryPage*PAGE_SIZE)+memoryPage);
                }
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
        } else if (sensorMode == 2) { // EVENT BASED MODE
          Serial.println(F("****** START EVENT BASE MODE ******"));
          prevMillis = 0;
          while (recieveBuff != 'p') {
            currentMillis = millis();
            if (currentMillis - prevMillis >= freqy ) {
              prevMillis = currentMillis;
              Serial.println(F("Debug log: Interrupt occured."));

              // get tmpSensor the right value depend on the picked sensor
              switch (type) {
                case 0: // WEATHER SENSOR SETUP
                  DHT.read11(DHT11_PIN);
                  tmpSensor = DHT.temperature;
                  break;
                case 1: // DISTANCE SENSOR SETUP
                  tmpSensor = digitalRead(HALL_PIN);
                  break;
                case 2:  // LIGHT SENSOR SETUP
                  tmpSensor = map(analogRead(PHOTORESIS_PIN), 0, 800, 0, 255);
                  break;
                case 3:  // HUMIDITY SENSOR SETUP
                  DHT.read11(DHT11_PIN);
                  tmpSensor = DHT.humidity;
                  break;
              }

              switch (tresholdMode) {
                case 3:  // EVENT BASED TRESHOLD MODE
                  if(!memoryIndexOverFlowFlag) {
                    if (tmpSensor < sensorTreshold && tresholdFlag == 0) {
                      Serial.println(F("Event detected: SENSOR < TRESHOLD"));
                      writeBuffer[memoryIndex] = tmpSensor;
                      writeBuffer[memoryIndex+1] = 0;
                      writeBuffer[memoryIndex+2] = (uint8_t) second();
                      writeBuffer[memoryIndex+3] = (uint8_t) minute();
                      writeBuffer[memoryIndex+4] = (uint8_t) hour();
                      writeBuffer[memoryIndex+5] = (uint8_t) day();
                      writeBuffer[memoryIndex+6] = (uint8_t) month();
                      tresholdFlag = 1;
                      writerFlag = true;
                    } else if (tmpSensor >= sensorTreshold && tresholdFlag == 1) {
                      Serial.println(F("Event detected: SENSOR > TRESHOLD"));
                      writeBuffer[memoryIndex] = tmpSensor;
                      writeBuffer[memoryIndex+1] = 1;
                      writeBuffer[memoryIndex+2] = (uint8_t) second();
                      writeBuffer[memoryIndex+3] = (uint8_t) minute();
                      writeBuffer[memoryIndex+4] = (uint8_t) hour();
                      writeBuffer[memoryIndex+5] = (uint8_t) day();
                      writeBuffer[memoryIndex+6] = (uint8_t) month();
                      tresholdFlag = 0;
                      writerFlag = true;
                    } else {
                      Serial.println(F("No changes..."));
                    }
                  }

                  if (writerFlag) {
                    Serial.print(F("Memory index: ")); Serial.print(memoryIndex);
                    Serial.print(F(" |  PAGE SIZE: ")); Serial.print(PAGE_SIZE);
                    Serial.print(F(" | Sensor treshold value: ")); Serial.println(sensorTreshold);
                    BLESerial.print("Sensor value: "); BLESerial.println(writeBuffer[memoryIndex]);
                    if ((memoryIndex-1) == PAGE_SIZE) { // WRITE DATA IF WRITER FLAG CHANGED
                      Serial.println(F("Memory index == PAGE_SIZE, so write it out to the memory."));
                      if (memoryPage == 0) {
                        FLASH.write(0, writeBuffer, PAGE_SIZE);
                        FLASH.waitforit();
                      } else {
                        FLASH.write(((memoryPage*PAGE_SIZE)+memoryPage), writeBuffer, PAGE_SIZE);
                        FLASH.waitforit();
                      }
                      Serial.println(F("1 page was written on the Flash memory."));
                      BLESerial.println(F("1 page was written on the Flash memory."));
                      memoryIndex = 0;
                      memoryPage++;
                    } else {
                      Serial.println(F("Memory index != PAGE_SIZE"));
                      memoryIndex += 8;
                      Serial.print(F("memory index:")); Serial.println(memoryIndex);
                    }
                    writerFlag = false;
                  }
                  
                  if((memoryIndex-1) == PAGE_SIZE) {
                    Serial.println("memoryIndexOverFlowFlag TRUE");
                    memoryIndexOverFlowFlag = true;
                    writerFlag = true;
                  } else {
                    memoryIndexOverFlowFlag = false;
                  }

                  break;
                case 2:
                  if (tmpSensor > sensorTreshold) {
                    BLESerial.print(F("[MAX]: The sensor value: "));
                    BLESerial.println(tmpSensor);
                  }
                  break;
                case 1:
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
