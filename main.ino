#include<SPIFlash.h>

uint32_t strAddr;

SPIFlash flash;

void setup() {
  Serial.begin(115200);
  flash.begin();

  strAddr = 300;
  String inputString = "This is a test String";
  flash.writeStr(strAddr, inputString);
 
  Serial.print(F("Written string: "));
  Serial.println(inputString);
  Serial.print(F("To address: "));
  Serial.println(strAddr);
  String outputString = "";
  
  if (flash.readStr(strAddr, outputString)) {
    Serial.print(F("Read string: "));
    Serial.println(outputString);
    Serial.print(F("From address: "));
    Serial.println(strAddr);
  }

  Serial.println("***** F L A S H   I N F O *******");
  Serial.print("GET CAPACITY:");
  Serial.println(flash.getCapacity());
  Serial.print("GET MAX PAGE:");
  Serial.println(flash.getMaxPage());
  Serial.print("GET MAN IDE:");
  Serial.println(flash.getManID(), HEX);
  Serial.print("GET JEDEC IDE:");
  Serial.println(flash.getJEDECID(), HEX);
  Serial.print("GET UNIQUE ID:");
  Serial.println((long)flash.getUniqueID(), HEX);
}

void loop() {
}

