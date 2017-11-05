#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <Wire.h> // I2C communication
#include "RTCLib.h"
#include "main.h"

#define THERMISTOR_PIN 0
#define PHOTORESIS_PIN 1
#define LED_PIN 13
#define RX_PIN 8
#define TX_PIN 7

SoftwareSerial BLESerial(TX_PIN, RX_PIN); // TX, RX
RTC_DS1307 RTC;

float Vin = 5.0;     // [V]
float Rt = 10000;    // Resistor t [ohm]
float R0 = 10000;    // value of rct in T0 [ohm]
float T0 = 298.15;   // use T0 in Kelvin [K]
float Vout = 0.0;    // Vout in A0
float Rout = 0.0;    // Rout in A0
byte phoVal;
int photo_tlimit;

// BLE communication
char recBuf;

// EEPROM variables
int address_e = 0;
byte value_e;
int bytemax_e = 1024;

// use the datasheet to get this data.
float T1 = 273.15;    // [K] in datasheet 0º C
float T2 = 373.15;    // [K] in datasheet 100° C
float RT1 = 35563; // [ohms]  resistence in T1
float RT2 = 549;  // [ohms]   resistence in T2
float beta = 0.0;  // initial parameters [K]
float Rinf = 0.0;  // initial parameters [ohm]
float TempC = 0.0; // variable output

void setup() {
  Serial.begin(9600);
  BLESerial.begin(9600);
  Wire.begin();
  RTC.begin();

  pinMode(THERMISTOR_PIN, INPUT);
  pinMode(PHOTORESIS_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Write a 0 to all 1024 bytes of the EEPROM
  // 1024 bytes in the ATmega328 EEPROM
  for (int i = 0; i < 1024; i++) {
    EEPROM.write(i, 0);
    digitalWrite(13, HIGH); // Turn led on until formating the memory
  }

  // Check to see if the RTC is keeping time. If it is, load the time from your computer.
  if (!RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // This will reflect the time that your sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  beta = (log(RT1 / RT2)) / ((1 / T1) - (1 / T2));
  Rinf = R0 * exp(-beta / T0);
}

void loop() {
 // EEPROM.write(address_e, nowTime)
  
  String dataString = "";

  // Extra Security for BLE Serial
  // if (BLESerial.isListening()) {
  //  BLESerial.println("BLE is listening..");
  // }

  if (BLESerial.available()) {
    recBuf = BLESerial.read();
    switch (recBuf) {
      case 'h':
        Serial.println("Get the help message for usage.");
        BLESerial.println("*** Manual for commands ***");
        BLESerial.println("t - get the actual temperature");
        BLESerial.println("ll - get the actual light value");
        BLESerial.println("-lc - list of the config values");
        BLESerial.println("-lo - set [ON] light limit");
        BLESerial.println("-lq - set [OFF] light limit");
        BLESerial.println("-lt - threshold limit");
        BLESerial.println("-i - time parser");
        BLESerial.println("-p - time reader");
        break;
      case 't':
        BLESerial.print("Temperature: ");
        BLESerial.print(temperatureSensor());
        BLESerial.println(" C");
        break;
      case 'l':
        phoVal = analogRead(PHOTORESIS_PIN);
        if (EEPROM.read(0) == 0 && EEPROM.read(2) == 0) {
          BLESerial.println("< Configuration not found, for the light sensor! >");
        }
        if (BLESerial.available()) {
          recBuf = BLESerial.read();
          switch (recBuf) {
            case 'c':
              BLESerial.println("Light module configuration");
              BLESerial.print("ON limit: ");
              BLESerial.println(EEPROM.read(0));
              BLESerial.print("OFF limit: ");
              BLESerial.println(EEPROM.read(2));
              break;
            case 'o':
              EEPROM.write(0, phoVal);
              Serial.print("Read the 0 value on flash: ");
              Serial.println(EEPROM.read(0));
              BLESerial.print("Light ON config saved: ");
              BLESerial.println(phoVal);
              break;
            case 'q':
              EEPROM.write(2, phoVal);
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
            default:
              BLESerial.println("Command not found. Use 'h' for command list");
          }
        }
        break;
      case 'w':
        BLESerial.println("Write the photoresistor value.");
        EEPROM.write(address_e, analogRead(PHOTORESIS_PIN));
        address_e++;
        break;
      case 'r':
        BLESerial.println("Read the photoresistor value.");
        value_e = EEPROM.read(address_e - 1);
        Serial.println(value_e);
        BLESerial.println(value_e);
        break;
      case 'i':
        Serial.println("TIME PARSER(0)");
        TimeParser(address_e);
        address_e += 4;
        break;
      case 'p':
        Serial.println("TIME READ(0)");
        address_e -= 4;
        TimeReader(address_e);
        break;
      case 'b':
        memoryReader();
        break;
      default:
        BLESerial.println("Command not found. Check out the available commands with 'h'.");
        break;
    }
  }

  if (phoVal > 300) { // Switch on the Led on pin 13 if there is dark
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  temperatureLogger(address_e);
  delay(1000); // delay 1sec
  
}

float temperatureSensor() {
  Vout = Vin * ((float)(analogRead(THERMISTOR_PIN)) / 1024.0); // Calc for NTC
  Rout = (Rt * Vout / (Vin - Vout));
  TempC = (beta / log(Rout / Rinf)) - 273.15;

  return TempC;
}

void temperatureLogger(int address_e) {
  EEPROM.write(address_e++, temperatureSensor());
}

void memoryReader() {
  int address = 0;
  while(EEPROM.read(address) != 0) {
    Serial.print("MEMORY READ: ");
    int tmp = EEPROM.read(address);
    Serial.println(tmp);
  }
}

void TimeParser(int address) {
  int time_address = address; 
  time_t timeStruct;
  // Initialize the current time
  DateTime now = RTC.now();
  
  int year_t = now.year();  // 2 byte 
  timeStruct.month = now.month();
  timeStruct.day = now.day();
  timeStruct.hour = now.hour();
  timeStruct.minute = now.minute();
  byte sec_t = now.second();
  
  //Put all the time and date strings into one String
  
  Serial.println("Time parsing: ");
  //Serial.println(dataString);

  EEPROM.write(time_address++, timeStruct.month); 
  EEPROM.write(time_address++, timeStruct.day);
  EEPROM.write(time_address++, timeStruct.hour);
  EEPROM.write(time_address++, timeStruct.minute);
 }

void TimeReader(int address) {
  int time_address = address;
  int tmp_mon = EEPROM.read(time_address++);
  int tmp_day = EEPROM.read(time_address++);
  int tmp_hour = EEPROM.read(time_address++);
  int tmp_min = EEPROM.read(time_address++);
  BLESerial.print("Month: ");
  BLESerial.println(tmp_mon);
  BLESerial.print("Day: ");
  BLESerial.println(tmp_day);
  BLESerial.print("Hour: ");
  BLESerial.println(tmp_hour);
  BLESerial.print("Minute: ");
  BLESerial.println(tmp_min);
 }
