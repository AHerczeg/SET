#include "rest_client.h"


RestClient client = RestClient("sccug-330-05.lancs.ac.uk",5000);

const char* path = "/trigger";

String deviceID;

char rawRole[ROLE_SIZE];

String role;

bool emptyFlag = true;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} COLOUR;

COLOUR ledColour = {0, 0, 0};

uint8_t brightness = 0;

bool debugFlag = false;

uint8_t sensors = 0x00000000;

Thread* debugThread;
Thread* ledThread;
Thread* advertiseThread;

os_thread_return_t serialListener(){
  String buffer = "";
  String tempStr = "";
  String responseString = "";
  for(;;){
    if(Serial.available() > 0){
      Serial.flush();
      while (Serial.available() > 0) {
        if(Serial.peek() != 10){
          buffer = tempStr + buffer + (char)(Serial.read());
        } else {Serial.read();}
      }
      Serial.println("Incoming serial: " + buffer);
      client.post(path, (const char*) sensorString, &responseString);
      Serial.println(responseString);
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

os_thread_return_t advertise(){
  for(;;){
    Particle.publish("detectChan", deviceID, PRIVATE);
    delay(36000);
  }
}

//// ***************************************************************************



void setup()
{
    // opens serial over USB
    Serial.begin(9600);

    // enables interrupts
    interrupts();

    System.enableReset();

    deviceID = System.deviceID();

    EEPROM.get(0, rawRole);

    for(int i = 0; i < ROLE_SIZE; i++)
    {
      if(rawRole[i] != 0xFF){
        role = String(rawRole);
        emptyFlag = false;
      }
    }
    if(emptyFlag){
      role = "unassigned";
      role.toCharArray(rawRole, ROLE_SIZE);
      EEPROM.put(0, rawRole);
    }

    EEPROM.get(11, ledColour);

    RGB.control(true);

    RGB.color(ledColour.r, ledColour.g, ledColour.b);

    RGB.brightness(brightness);

    ledThread = new Thread("ledBlinking", ledBlinking);

    debugThread = new Thread("debug", serialListener);

    advertiseThread = new Thread("advertise", advertise);
}


void loop(void)
{
    while(!debugFlag){
      //code();
      Serial.println("Start");
      Si70xx si7020;
      Serial.println(mpu9150.begin(mpu9150._addr_motion));
      Serial.println(si7020.begin());
      Serial.println(Wire.begin());
      Serial.println("End");
      delay(5000);
    }
}

void code(){

  String tempStr = "";

  String sensorData = tempStr+"{temperature" + String(Si7020Temperature) + ", humidity:" + String(Si7020Humidity) + ",light:" + String(Si1132Visible) + "}";

  String sensorString = tempStr+"{\"sensorId\":\"" + deviceID + "\", \"sensorData\":\"" + sensorData + "\",\"timestamp\":\"" + timestamp + "\"}";

  String responseString = "";

  client.post(path, (const char*) sensorString, &responseString);

  delay(1000);
}


void blink(int level, int step, int speed){
  while(brightness < level){
    brightness += step;
    if(brightness > 255)
      brightness = 255;
    RGB.brightness(brightness);
    delay (speed);
  }
  delay(speed);
  while(brightness > 0){
    brightness -= step;
    if(brightness < 0)
      brightness = 0;
    RGB.brightness(brightness);
    delay (speed);
  }
  delay(speed);
}


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
