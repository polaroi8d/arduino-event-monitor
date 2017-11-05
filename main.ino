#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <Wire.h> // I2C communication
#include "RTCLib.h"
#include "main.h"

#define PHOTORESIS_PIN 1
#define LED_PIN 13
#define RX_PIN 8
#define TX_PIN 7

SoftwareSerial BLESerial(TX_PIN, RX_PIN); // TX, RX

int phoVal;
int phoValWord;
int photo_tlimit;
byte high;
byte low;

// BLE communication
char recBuf;

// EEPROM variables
int address_e = 0;
byte value_e;
int bytemax_e = 1024;

void setup() {
  Serial.begin(9600);
  BLESerial.begin(9600);
  Wire.begin();

  pinMode(PHOTORESIS_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Write a 0 to all 1024 bytes of the EEPROM
  // 1024 bytes in the ATmega328 EEPROM
  for (int i = 0; i < 1024; i++) {
    EEPROM.write(i, 0);
    digitalWrite(13, HIGH); // Turn led on until formating the memory
  }
  digitalWrite(13, LOW);
}

void loop() {
  if (BLESerial.available()) {
    recBuf = BLESerial.read();
    switch (recBuf) {
      case 'h':
        Serial.println("Get the help message for usage.");
        BLESerial.println("*** Manual for commands ***");
        BLESerial.println("ll - get the actual light value");
        BLESerial.println("-lc - list of the config values");
        BLESerial.println("-lo - set [ON] light limit");
        BLESerial.println("-lq - set [OFF] light limit");
        BLESerial.println("-lt - threshold limit");
        break;
      case 'l':
        phoVal = analogRead(PHOTORESIS_PIN);
        if (EEPROM.read(0) == 0 && EEPROM.read(2) == 0) {
          BLESerial.println("Configuration not found!");
        }
        if (BLESerial.available()) {
          recBuf = BLESerial.read();
          switch (recBuf) {
            case 'c':
              BLESerial.println("Light module configuration");
              BLESerial.print("ON limit: ");
              BLESerial.println(EEPROMReadInt(0));
              BLESerial.print("OFF limit: ");
              BLESerial.println(EEPROMReadInt(2));
              break;
            case 'o':
              EEPROMWriteInt(0, phoVal);
              BLESerial.print("Light ON config saved: ");
              BLESerial.println(phoVal);
              break;
            case 'q':
              EEPROMWriteInt(2, phoVal);
              BLESerial.print("Light OFF config saved: ");
              BLESerial.println(phoVal);
              break;
            case 'l':
              BLESerial.print("Read the light value: ");
              BLESerial.println(phoVal);
              break;
            case 't':
              photo_tlimit = BLESerial.read();
              BLESerial.print("Photoresistor threshold limit: ");
              BLESerial.println(photo_tlimit);
            case 'r':
              byte tmp_data;
              while (tmp_data != 0) {
                int i = 0;
                tmp_data = EEPROM.read(i);
                Serial.println(tmp_data);
                digitalWrite(13, HIGH); // Turn led on until formating the memory
                i++;
              } 
              digitalWrite(13, LOW);
              break;
            case 's':
              BLESerial.print("Start the sampling of the photoresistor values");
              BLESerial.print("Use @ to stop it");
              byte STATUS;
              while(BLESerial.read() != '@') {
                int tmp_address = 4;
                delay(1500);
                phoVal = analogRead(PHOTORESIS_PIN);
                int tmp_min_pho = EEPROMReadInt(0);
                int tmp_max_pho = EEPROMReadInt(2);
                if (phoVal > (tmp_min_pho+100)) {
                  if (STATUS == 1){
                    BLESerial.println("A lampa allapota lekapcsolt lett.");
                  }
                  Serial.println("Lightning off!");
                  EEPROM.write(tmp_address++, 'L');
                  STATUS = 0;
                } else if (phoVal < (tmp_max_pho-100)) {
                  if (STATUS == 0){
                    BLESerial.println("A lampa allapota felkapcsolt lett.");
                  }
                  Serial.println("Lightning on!");
                  EEPROM.write(tmp_address++, 'F');
                  STATUS = 1;
                } else {
                  Serial.println(phoVal);
                }
              }
              break;
            default:
              BLESerial.println("Command not found. Use 'h' for command list");
          }
        }
        break;
      default:
        BLESerial.println("Command not found. Check out the available commands with 'h'.");
        break;
    }
  }
  delay(1000); // delay 1sec
}


void EEPROMWriteInt(int address, int value)
{
  byte two = (value & 0xFF);
  byte one = ((value >> 8) & 0xFF);
  
  EEPROM.update(address, two);
  EEPROM.update(address + 1, one);
}
 
int EEPROMReadInt(int address)
{
  long two = EEPROM.read(address);
  long one = EEPROM.read(address + 1);
 
  return ((two << 0) & 0xFFFFFF) + ((one << 8) & 0xFFFFFFFF);
}
