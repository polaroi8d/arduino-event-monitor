#include <SoftwareSerial.h>
#include <SPIFlash.h>
#include <SPI.h>
#include "main.h"
#include "dht.h"

#define PHOTORESIS_PIN 1
#define DHT11_PIN 4
#define RX_PIN 8
#define TX_PIN 7

SoftwareSerial BLESerial(TX_PIN, RX_PIN); // TX, RX
SPIFlash flash;
dht DHT;

int phoVal;
int i_data = 0;
int tmp_address;
int read_tmp = 0;
int dht_sensor;
unsigned long past_time;

// BLE communication
char recieve_buff;

// EEPROM
uint32_t strAddr = 0;
char output = "";

void setup() {
  Serial.begin(9600);
  BLESerial.begin(9600);
  flash.begin();

  pinMode(PHOTORESIS_PIN, OUTPUT);
}

void loop() {
  if (BLESerial.available()) {
    recieve_buff = BLESerial.read();
    Serial.print(recieve_buff);
    switch (recieve_buff) {
      case 'h': // h 104
        Serial.println("Get the help message for usage.");
        BLESerial.println("*** Manual for commands ***");
        BLESerial.println("w - get the actual temperature value");
        BLESerial.println("e - get the actual humidity value");
        BLESerial.println("l - get the actual light value");
        BLESerial.println("c - list of the config values");
        BLESerial.println("o - set [ON] light limit");
        BLESerial.println("q - set [OFF] light limit");
        BLESerial.println("r - read the values from memory");
        BLESerial.println("s - start sampling the data (photoresistor mode)");

        break;
      case 'l': // l 108
        phoVal = analogRead(PHOTORESIS_PIN);
        BLESerial.print("Read the light value: ");
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
      /*case 's': // s 115
        BLESerial.print("Start the sampling of the photoresistor values");
        BLESerial.print("Use @ to stop it");
        tmp_address = 4;
        byte STATUS;
        while (BLESerial.read() != '@') {
          delay(1500);
          phoVal = analogRead(PHOTORESIS_PIN);
         if (phoVal > (EEPROMReadInt(0) + 100)) {
            if (STATUS == 1) {
              BLESerial.println("The lamp is turn off.");
            }
            Serial.println("Lightning off!");
            EEPROM.write(tmp_address++, STATUS);
            STATUS = 0;
          } else if (phoVal < (EEPROMReadInt(2) - 100)) {
            if (STATUS == 0) {
              BLESerial.println("The lamp is turn on.");
            }
            Serial.println("Lightning on!");
            EEPROM.write(tmp_address++, STATUS);
            STATUS = 1;
          } else {
            Serial.println(phoVal);
          }
        }
        BLESerial.println("Stoped the sampling.");
        break;*/
      case 'w':
        dht_sensor = DHT.read11(DHT11_PIN);
        BLESerial.print("Temperature: ");
        BLESerial.println(DHT.temperature);
        break;
      case 'e':
        dht_sensor = DHT.read11(DHT11_PIN);
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
          dht_sensor = DHT.read11(DHT11_PIN);
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
        break;
    }
  }
}
