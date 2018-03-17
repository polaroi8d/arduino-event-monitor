#include <SoftwareSerial.h>
#include <SPIFlash.h>
#include <SPI.h>
#include "Log.h"
#include <stdint.h>
#include "main.h"
#include "dht.h"

#define PHOTORESIS_PIN 1
#define DHT11_PIN 4
#define HALL_PIN 6
#define RX_PIN 8
#define TX_PIN 7

SoftwareSerial BLESerial(TX_PIN, RX_PIN); // TX, RX
SPIFlash flash;
dht DHT;

/** CONFIG VARIABLES */
int type;
int mode;
unsigned long freqy;

/** SENSORS VARIABLES */
int phoVal;
int hallVal;
int dhtVal;

int tmp_address;
int read_tmp = 0;

unsigned long past_time;

// BLE Serial 
char recieveBuff;

// EEPROM
uint16_t strAddr = 0;
char output = "";

void setup() {
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Serial.begin(9600);
  BLESerial.begin(9600);
  flash.begin();

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
        mode = 0;
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
      case 'r':
        Serial.println("****** S T A R T   P R O G R A M ******");
        if (mode = '1') { //SAMPLING MODE
          while (recieveBuff != 'p') {
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
                BLESerial.print("Hall sensor:");
                BLESerial.println(hallVal);
                break;
              case 2:  // LIGHT SENSOR SETUP
                phoVal = analogRead(PHOTORESIS_PIN);
                BLESerial.print("Light:");
                BLESerial.println(phoVal);
                break;
            }
            if (BLESerial.available()) {
              recieveBuff = BLESerial.read();  // BLE recieve buffer
            }
            delay(freqy);  // SETUP SAMPLING TIME
          }
          status("ready");
        } else if (mode = '2') {
          // TODO: Implement the event based logger
        } else {
          BLESerial.print("Other modes not ready now...");
        }
        break;
    default:
        Log.notice(" DEFAULT RECIEVED BUFFER: %c"CR, recieveBuff);
        break;
    }
  }

  /*switch (recieveBuff) {
    case 'h': // h 104
      Serial.println("Get the help message for usage.");
      BLESerial.println("******************************************");
      BLESerial.println("w - get the actual temperature value");
      BLESerial.println("e - get the actual humidity value");
      BLESerial.println("a - get actual hall sensor");
      BLESerial.println("l - get the actual light value");
      BLESerial.println("c - list of the config values");
      BLESerial.println("o - set [ON] light limit");
      BLESerial.println("q - set [OFF] light limit");
      BLESerial.println("r - read the values from memory");
      BLESerial.println("s - start sampling the data (photoresistor mode)");
      BLESerial.println("******************************************");
      break;
    case 'a':
      hallVal = digitalRead(HALL_PIN);
      BLESerial.print("HALL:");
      BLESerial.println(hallVal);
      break;
    case 'l': // l 108
      phoVal = analogRead(PHOTORESIS_PIN);
      BLESerial.print("LIGHT:");
      BLESerial.println(phoVal);
      break;
    case 'c': // c 99
      BLESerial.println("Light module configuration");
      BLESerial.print("ON limit: ");
      //BLESerial.println(EEPROMReadInt(0));
      BLESerial.print("OFF limit: ");
      //BLESerial.println(EEPROMReadInt(2));
      break;
    case 'o': // o 111
      phoVal = analogRead(PHOTORESIS_PIN);
      //EEPROMWriteInt(0, phoVal);
      BLESerial.print("Light ON config saved: ");
      BLESerial.println(phoVal);
      break;
    case 'q': // q 113
      phoVal = analogRead(PHOTORESIS_PIN);
      //EEPROMWriteInt(2, phoVal);
      BLESerial.print("Light OFF config saved: ");
      BLESerial.println(phoVal);
      break;
    case 'r': // r 114
      Serial.println("Read the memory data:");
      while (read_tmp != 1023) {
        //readEEPROM = EEPROM.read(read_tmp);
        Serial.print(read_tmp);
        Serial.print(". byte is ");
        //Serial.println(readEEPROM);
        digitalWrite(13, HIGH); // Turn led on until formating the memory
        read_tmp++;
      }
      digitalWrite(13, LOW);
      break;
    case 'w':
      dhtVal = DHT.read11(DHT11_PIN);
      BLESerial.print("Temperature: ");
      BLESerial.println(DHT.temperature);
      break;
    case 'e':
      dhtVal = DHT.read11(DHT11_PIN);
      BLESerial.print("Humidity: ");
      BLESerial.println(DHT.humidity);
      break;
    case 'k':
      Serial.println("Start sampling temperature & humidity.");
      Serial.println("Stop the sampling with the /s/ command");
      char sampling_stopper;
      while ( sampling_stopper != 's'){
        if (BLESerial.available()) {
          sampling_stopper = BLESerial.read();
        }
        dhtVal = DHT.read11(DHT11_PIN);
        BLESerial.print("Temperature: ");
        BLESerial.println(DHT.temperature);
        BLESerial.print("Humidity: ");
        BLESerial.println(DHT.humidity);
        past_time = millis(); // get the elapsed time from the uno start
        flash.writeULong(strAddr, past_time);
        strAddr += 4;
        flash.writeShort(strAddr, DHT.humidity);
        strAddr += 2;
        flash.writeShort(strAddr, DHT.temperature);
        strAddr += 2;
        delay(2000); // DHT11 sensor specific
      }
      if (flash.readAnything(0, output)) {
        Serial.print(F("Read string: "));
        Serial.println(output);
        Serial.print(F("From address: "));
        Serial.println(strAddr);
      }
      break;
    default:
      BLESerial.println("Command not found. Check out the available commands with 'h'.");
      BLESerial.println("RECIEVE BUFFER:");
      BLESerial.print(recieveBuff);
      break;
    }*/
  recieveBuff = "";
}
