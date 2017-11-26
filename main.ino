#include <EEPROM.h>
#include <SoftwareSerial.h>
#include "main.h"
#include "dht.h"

#define PHOTORESIS_PIN 1
#define DHT11_PIN 4
#define LED_PIN 13
#define RX_PIN 8
#define TX_PIN 7

SoftwareSerial BLESerial(TX_PIN, RX_PIN); // TX, RX
dht DHT;

int phoVal;
int i_data = 0;
int tmp_address;
int read_tmp = 0;
int dht_sensor;

// BLE communication
char recieve_buff;

// EEPROM variables
byte high;
byte low;
byte readEEPROM;

void setup() {
  Serial.begin(9600);
  BLESerial.begin(9600);

  pinMode(PHOTORESIS_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Write a 0 to all 1024 bytes of the EEPROM
  // 1024 bytes in the ATmega328 EEPROM
  for (int z = 0; z < 1024; z++) {
    EEPROM.write(z, 0);
    digitalWrite(13, HIGH); // Turn led on until formating the memory
  }
  digitalWrite(13, LOW);
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
        BLESerial.println(EEPROMReadInt(0));
        BLESerial.print("OFF limit: ");
        BLESerial.println(EEPROMReadInt(2));
        break;
      case 'o': // o 111
        phoVal = analogRead(PHOTORESIS_PIN);
        EEPROMWriteInt(0, phoVal);
        BLESerial.print("Light ON config saved: ");
        BLESerial.println(phoVal);
        break;
      case 'q': // q 113
        phoVal = analogRead(PHOTORESIS_PIN);
        EEPROMWriteInt(2, phoVal);
        BLESerial.print("Light OFF config saved: ");
        BLESerial.println(phoVal);
        break;
      case 'r': // r 114
        Serial.println("Read the memory data:");
        while (read_tmp != 1023) {
          readEEPROM = EEPROM.read(read_tmp);
          Serial.print(read_tmp);
          Serial.print(". byte is ");
          Serial.println(readEEPROM);
          digitalWrite(13, HIGH); // Turn led on until formating the memory
          read_tmp++;
        }
        digitalWrite(13, LOW);
        break;
      case 's': // s 115
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
        break;
      case 'w':
        dht_sensor = DHT.read11(DHT11_PIN);
        BLESerial.print("Temperature: ");
        BLESerial.println(DHT.temperature);
        break;
      case 'e':
        dht_sensor = DHT.read11(DHT11_PIN);
        BLESerial.print("Humidity: ");
        BLESerial.println(DHT.humidity);
      default:
        BLESerial.println("Command not found. Check out the available commands with 'h'.");
        break;
    }
  }
  int chk = DHT.read11(DHT11_PIN);
  Serial.print("Temperature = ");
  Serial.println(DHT.temperature);
  Serial.print("Humidity = ");
  Serial.println(DHT.humidity);
  delay(2000);
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
