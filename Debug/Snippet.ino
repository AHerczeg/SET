// *******************************************************
// * A basic template of the required code for debugging *
// *******************************************************


#include "MPU9150.h"
#include "Si1132.h"
#include "Si70xx.h"
#include "rest_client.h"

// ----------------------- //

// Define the variable bit flag
#define TEMP_SENSOR 0x80
#define HUM_SENSOR 0x40
#define LIGHT_SENSOR 0x20
#define ACCEL_SENSOR 0x10
#define MOTION_SENSOR 0x08

// The structure used for setting the LED light
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} COLOUR;

COLOUR ledColour = {0, 0, 0};

uint8_t brightness = 0;

bool debugFlag = false;

// Flags of all sensors, initially set to 00000000
uint8_t sensors = 0x00;

Thread* debugThread;
Thread* ledThread;
Thread* advertiseThread;

// Thread for listening to the serial
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

// Thread for blinking the LED
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

// Start debugging
void debug(){

  // Pause loop, wait for any running process to finish
  debugFlag = true;

  delay(1000);

  noInterrupts();

  Serial.println("DEBUG_START");

  // Test serial
  if(Serial.isConnected())
    Serial.println("  SERIAL: GO");
  else
    Serial.println("  SERIAL: NOGO");

  // Test LED
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

  // Test EEPROM
  byte testByte = random(255);
  int testPos = random(2047);
  byte byteHolder = EEPROM.read(testPos);
  EEPROM.put(testPos, testByte);

  if(EEPROM.read(testPos) == testByte)
    Serial.println("  EEPROM: GO");
  else
    Serial.println("  EEPROM: NOGO");

  EEPROM.put(testPos, byteHolder);

  // Test WiFi
  if(WiFi.connecting() || !(WiFi.ready()) )
    Serial.println("  WIFI: NOGO");
  else
    Serial.println("  WIFI: GO");

  // Test Particle cloud connection
  if(Particle.connected())
    Serial.println("  CLOUD: GO");
  else
    Serial.println("  CLOUD: NOGO");

  // Test server connection
  const char* pathHolder = path;
  path = "/test";

  String testString = "test", responseString = "";
  client.post(path, (const char*) testString, &responseString);
  if(responseString.length() > 0)
    Serial.println("  SERVER: GO");
  else
    Serial.println("  SERVER: NOGO");

  // List all recognised sensors
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

  // Run the main code once, measure runtime
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

  // End debugging
  RGB.color(ledColour.r, ledColour.g, ledColour.b);

  debugFlag = false;

  Serial.println("DEBUG_END");

  interrupts();
}
