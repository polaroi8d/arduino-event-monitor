#include <EEPROM.h>
#include <SoftwareSerial.h>

#define THERMISTOR_PIN 0
#define PHOTORESIS_PIN 1
#define LED_PIN 13
#define RX_PIN 8
#define TX_PIN 7

SoftwareSerial BLESerial(TX_PIN,RX_PIN); // TX, RX

float Vin = 5.0;     // [V]        
float Rt = 10000;    // Resistor t [ohm]
float R0 = 10000;    // value of rct in T0 [ohm]
float T0 = 298.15;   // use T0 in Kelvin [K]
float Vout = 0.0;    // Vout in A0 
float Rout = 0.0;    // Rout in A0
int phoVal;
char recBuf;

// use the datasheet to get this data.
float T1=273.15;      // [K] in datasheet 0º C
float T2=373.15;      // [K] in datasheet 100° C
float RT1=35563;   // [ohms]  resistence in T1
float RT2=549;    // [ohms]   resistence in T2
float beta=0.0;    // initial parameters [K]
float Rinf=0.0;    // initial parameters [ohm]   
float TempC=0.0;   // variable output

void setup() {
  Serial.begin(9600);
  BLESerial.begin(9600);
  
  pinMode(THERMISTOR_PIN, INPUT);
  pinMode(PHOTORESIS_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Write a 0 to all 1024 bytes of the EEPROM
  // 1024 bytes in the ATmega328 EEPROM
  for (int i = 0; i < 1024; i++) {
    EEPROM.write(i, 0);
    digitalWrite(13, HIGH); // Turn led on until formating the memory
  }

  beta=(log(RT1/RT2))/((1/T1)-(1/T2));
  Rinf=R0*exp(-beta/T0);
}

void loop() {
  if (BLESerial.isListening()) {
    BLESerial.println("BLE is listening..");
  }
  if (BLESerial.available()) {
    recBuf = BLESerial.read();
    switch (recBuf) {
      case 'h': 
        Serial.println("Get the help message for usage.");
        BLESerial.println("*** Manual for commands ***");
        BLESerial.println("t - get the actual temperature");
        BLESerial.println("l - get the actual light value");
        break;
      case 't':
        Vout=Vin*((float)(analogRead(THERMISTOR_PIN))/1024.0); // Calc for NTC
        Rout=(Rt*Vout/(Vin-Vout));
        TempC=(beta/log(Rout/Rinf))-273.15;
        BLESerial.print("Temperature: ");
        BLESerial.print(TempC);
        BLESerial.println(" C");
        break;
      case 'l':
        phoVal=analogRead(PHOTORESIS_PIN);
        BLESerial.print("The light value: ");
        BLESerial.println(phoVal);
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

  //BLESerial.println("Connection established.");

  delay(1000); // delay 1sec 
}
