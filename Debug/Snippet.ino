#include "MPU9150.h"
#include "Si1132.h"
#include "Si70xx.h"
#include "rest_client.h"

// ----------------------- //
#define TEMP_SENSOR 0x80
#define HUM_SENSOR 0x40
#define LIGHT_SENSOR 0x20
#define ACCEL_SENSOR 0x10
#define MOTION_SENSOR 0x08


typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} COLOUR;

COLOUR ledColour = {0, 0, 0};

uint8_t brightness = 0;

bool debugFlag = false;

uint8_t sensors = 0x00;

Thread* debugThread;
Thread* ledThread;
Thread* advertiseThread;

os_thread_return_t debugListener(){
  String buffer = "";
  String tempStr = "";
  for(;;){
    if(Serial.available() > 0){
      Serial.flush();
      while (Serial.available() > 0) {
        if(Serial.peek() != 10){
          buffer = tempStr + buffer + (char)(Serial.read());
        } else {Serial.read();}
      }
      Serial.println("Incoming serial: " + buffer);
      if(buffer.compareTo("debug") == 0){
        debug();
      }
      buffer = "";
      delay(100);
    }
  }
}

os_thread_return_t ledBlinking(){
  for(;;){
    if(!debugFlag)
      blink(100, 5, 100);
    delay(100);
  }
}

// --------- SETUP ---------------

void setup()
{
    // opens serial over USB
    Serial.begin(9600);

    ledThread = new Thread("ledBlinking", ledBlinking);

    debugThread = new Thread("debug", debugListener);

}

// -------------------------------
void debug(){

  debugFlag = true;

  delay(1000);

  noInterrupts();

  Serial.println("DEBUG_START");

  if(Serial.isConnected())
    Serial.println("  SERIAL: GO");
  else
    Serial.println("  SERIAL: NOGO");

  RGB.brightness(255);

  RGB.color(0, 0, 0);
  delay(500);
  RGB.color(255, 0, 0);
  delay(500);
  RGB.color(0, 255, 0);
  delay(500);
  RGB.color(0, 0, 255);
  delay(500);
  RGB.color(255, 255, 0);
  delay(500);
  RGB.color(255, 0, 255);
  delay(500);
  RGB.color(0, 255, 255);
  delay(500);
  RGB.color(255, 255, 255);
  delay(500);

  blink(255, 15, 50);
  blink(255, 15, 50);

  Serial.println("  RGB: GO");

  byte testByte = random(255);

  int testPos = random(2047);

  byte byteHolder = EEPROM.read(testPos);

  EEPROM.put(testPos, testByte);

  if(EEPROM.read(testPos) == testByte)
    Serial.println("  EEPROM: GO");
  else
    Serial.println("  EEPROM: NOGO");

  EEPROM.put(testPos, byteHolder);

  if(WiFi.connecting() || !(WiFi.ready()) )
    Serial.println("  WIFI: NOGO");
  else
    Serial.println("  WIFI: GO");

  if(Particle.connected())
    Serial.println("  CLOUD: GO");
  else
    Serial.println("  CLOUD: NOGO");

  const char* pathHolder = path;
  path = "/test";

  String testString = "test", responseString = "";
  client.post(path, (const char*) testString, &responseString);
  if(responseString.length() > 0)
    Serial.println("  SERVER: GO");
  else
    Serial.println("  SERVER: NOGO");

  Serial.println("  SENSORS")
  if((sensors & TEMP_SENSOR) > 0){
    Serial.println("    TEMPERATURE")
  }
  if((sensors & HUM_SENSOR) > 0){
    Serial.println("    HUMIDITIY")
  }
  if((sensors & LIGHT_SENSOR) > 0){
    Serial.println("    LIGHT")
  }
  if((sensors & ACCEL_SENSOR) > 0){
    Serial.println("    ACCELERATION")
  }
  if((sensors & MOTION_SENSOR) > 0){
    Serial.println("    MOTION")
  }

  Serial.println("---------- LOOP START ----------");
  unsigned long start = millis();
  code();
  unsigned long end = millis();
  Serial.println("---------- LOOP END ----------");
  int totalTime = end-start;
  Serial.println("  LOOP: GO");
  String tempStr = "        Runtime: ";
  tempStr += "" + String(totalTime) + "ms";
  Serial.println(tempStr);

  if(mpu9150.begin(mpu9150._addr_motion))
    Serial.println("  CLOUD: GO");
  else
    Serial.println("  CLOUD: NOGO");

  RGB.color(ledColour.r, ledColour.g, ledColour.b);

  debugFlag = false;

  Serial.println("DEBUG_END");

  interrupts();
}
