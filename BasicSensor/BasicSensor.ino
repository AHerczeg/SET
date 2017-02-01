#include "MPU9150.h"
#include "Si1132.h"
#include "Si70xx.h"
#include "rest_client.h"


//// ***************************************************************************
//// ***************************************************************************

//// Initialize application variables
#define RAD_TO_DEGREES 57.2957795131
#define DEG_TO_RADIANS 0.0174533
#define PI 3.1415926535
#define ACCEL_SCALE 2 // +/- 2g
#define ROLE_SIZE 11

int SENSORDELAY = 500;  //// 500; //3000; // milliseconds (runs x1)
int EVENTSDELAY = 1000; //// milliseconds (runs x10)
int OTAUPDDELAY = 7000; //// milliseconds (runs x1)
int SLEEP_DELAY = 0;    //// 40 seconds (runs x1) - should get about 24 hours on 2100mAH, 0 to disable and use RELAX_DELAY instead
String SLEEP_DELAY_MIN = "15"; // seconds - easier to store as string then convert to int
String SLEEP_DELAY_STATUS = "OK"; // always OK to start with
int RELAX_DELAY = 5; // seconds (runs x1) - no power impact, just idle/relaxing
double THRESHOLD = 1.0; //Threshold for temperature and humidity changes
int THRES_L = 5; // Threshold for light changes
int THRES_S = 23; // Threshold for sound changes

// Variables for the I2C scan
byte I2CERR, I2CADR;

//// ***************************************************************************
//// ***************************************************************************

int I2CEN = D2;
int ALGEN = D3;
int LED = D7;

int SOUND = A0;
double SOUNDV = 0; //// Volts Peak-to-Peak Level/Amplitude

int POWR1 = A1;
int POWR2 = A2;
int POWR3 = A3;
double POWR1V = 0; //Watts
double POWR2V = 0; //Watts
double POWR3V = 0; //Watts

int SOILT = A4;
double SOILTV = 0; //// Celsius: temperature (C) = Vout*41.67-40 :: Temperature (F) = Vout*75.006-40

int SOILH = A5;
double SOILHV = 0; //// Volumetric Water Content (VWC): http://www.vegetronix.com/TechInfo/How-To-Measure-VWC.phtml

bool BMP180OK = false;
double BMP180Pressure = 0;    //// hPa
double BMP180Temperature = 0; //// Celsius
double BMP180Altitude = 0;    //// Meters

double oldTmp = 0;
double oldHmd = 0;
double oldVisible = 0;
double oldSound = 0;

bool Si7020OK = false;
double Si7020Temperature = 0; //// Celsius
double Si7020Humidity = 0;    //// %Relative Humidity

bool Si1132OK = false;
double Si1132UVIndex = 0; //// UV Index scoring is as follows: 1-2  -> Low,
                          //// 3-5  -> Moderate, 6-7  -> High,
                          //// 8-10 -> Very High, 11+  -> Extreme
double Si1132Visible = 0; //// Lux
double Si1132InfraRed = 0; //// Lux


MPU9150 mpu9150;
bool ACCELOK = false;
int cx, cy, cz, ax, ay, az, gx, gy, gz;
double tm; //// Celsius
Si1132 si1132 = Si1132();

int inputPin = D6; // PIR motion sensor. D6 goes HIGH when motion is detected and LOW when
                   // there's no motion.

int sensorState = LOW;        // Start by assuming no motion detected
int sensorValue = 0;
bool sensorAttached = false;


RestClient client = RestClient("sccug-330-05.lancs.ac.uk",5000);

const char* path = "/sensor";

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

Thread* debugThread;
Thread* ledThread;

os_thread_return_t debugListener(){
  String buffer = "";
  String tempStr = "";
  for(;;){
    if(Serial.available() > 0){
      while (Serial.available() > 0) {
        char incomingByte = (char)(Serial.read());
        if(incomingByte != 10){
          buffer = tempStr + buffer + incomingByte;
        }
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

//// ***************************************************************************


float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setPinsMode()
{
    pinMode(I2CEN, OUTPUT);
    pinMode(ALGEN, OUTPUT);
    pinMode(LED, OUTPUT);

    pinMode(SOUND, INPUT);

    pinMode(POWR1, INPUT);
    pinMode(POWR2, INPUT);
    pinMode(POWR3, INPUT);

    pinMode(SOILT, INPUT);
    pinMode(SOILH, INPUT);
}

void setup()
{
    // opens serial over USB
    Serial.begin(9600);

    // Set I2C speed
    // 400Khz seems to work best with the Photon with the packaged I2C sensors
    Wire.setSpeed(CLOCK_SPEED_400KHZ);

    Wire.begin();  // Start up I2C, required for LSM303 communication

    // enables interrupts
    interrupts();

    // initialises the IO pins
    setPinsMode();

    // initialises MPU9150 inertial measure unit
    initialiseMPU9150();

    // Initialize motion sensor input pin
    pinMode(inputPin, INPUT);

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

    debugThread = new Thread("debug", debugListener);

    //if(sensorValue == HIGH)
      //sensorAttached = true;
}

void initialiseMPU9150()
{
  ACCELOK = mpu9150.begin(mpu9150._addr_motion); // Initialize MPU9150

  if (ACCELOK)
  {
      // Clear the 'sleep' bit to start the sensor.
      mpu9150.writeSensor(mpu9150._addr_motion, MPU9150_PWR_MGMT_1, 0);

      /// Set up compass
      mpu9150.writeSensor(mpu9150._addr_compass, 0x0A, 0x00); //PowerDownMode
      mpu9150.writeSensor(mpu9150._addr_compass, 0x0A, 0x0F); //SelfTest
      mpu9150.writeSensor(mpu9150._addr_compass, 0x0A, 0x00); //PowerDownMode

      mpu9150.writeSensor(mpu9150._addr_motion, 0x24, 0x40); //Wait for Data at Slave0
      mpu9150.writeSensor(mpu9150._addr_motion, 0x25, 0x8C); //Set i2c address at slave0 at 0x0C
      mpu9150.writeSensor(mpu9150._addr_motion, 0x26, 0x02); //Set where reading at slave 0 starts
      mpu9150.writeSensor(mpu9150._addr_motion, 0x27, 0x88); //set offset at start reading and enable
      mpu9150.writeSensor(mpu9150._addr_motion, 0x28, 0x0C); //set i2c address at slv1 at 0x0C
      mpu9150.writeSensor(mpu9150._addr_motion, 0x29, 0x0A); //Set where reading at slave 1 starts
      mpu9150.writeSensor(mpu9150._addr_motion, 0x2A, 0x81); //Enable at set length to 1
      mpu9150.writeSensor(mpu9150._addr_motion, 0x64, 0x01); //overvride register
      mpu9150.writeSensor(mpu9150._addr_motion, 0x67, 0x03); //set delay rate
      mpu9150.writeSensor(mpu9150._addr_motion, 0x01, 0x80);

      mpu9150.writeSensor(mpu9150._addr_motion, 0x34, 0x04); //set i2c slv4 delay
      mpu9150.writeSensor(mpu9150._addr_motion, 0x64, 0x00); //override register
      mpu9150.writeSensor(mpu9150._addr_motion, 0x6A, 0x00); //clear usr setting
      mpu9150.writeSensor(mpu9150._addr_motion, 0x64, 0x01); //override register
      mpu9150.writeSensor(mpu9150._addr_motion, 0x6A, 0x20); //enable master i2c mode
      mpu9150.writeSensor(mpu9150._addr_motion, 0x34, 0x13); //disable slv4
    }
    else
    {
      Serial.println("Unable to start MPU5150");
    }

}

void loop(void)
{
    while(!debugFlag){
      code();
    }
}

int readWeatherSi7020()
{
    Si70xx si7020;
    Si7020OK = si7020.begin(); //// initialises Si7020

    if (Si7020OK)
    {
        Si7020Temperature = si7020.readTemperature();
        Si7020Humidity = si7020.readHumidity();
    }

    return Si7020OK ? 2 : 0;
}



///reads UV, visible and InfraRed light level
void readSi1132Sensor()
{
    si1132.begin(); //// initialises Si1132
    Si1132UVIndex = si1132.readUV() *0.01;
    Si1132Visible = si1132.readVisible();
    Si1132InfraRed = si1132.readIR();
}

//returns sound level measurement in as voltage values (0 to 3.3v)
float readSoundLevel()
{
    unsigned int sampleWindow = 50; // Sample window width in milliseconds (50 milliseconds = 20Hz)
    unsigned long endWindow = millis() + sampleWindow;  // End of sample window

    unsigned int signalSample = 0;
    unsigned int signalMin = 4095; // Minimum is the lowest signal below which we assume silence
    unsigned int signalMax = 0; // Maximum signal starts out the same as the Minimum signal

    // collect data for milliseconds equal to sampleWindow
    while (millis() < endWindow)
    {
        signalSample = analogRead(SOUND);
        if (signalSample > signalMax)
        {
            signalMax = signalSample;  // save just the max levels
        }
        else if (signalSample < signalMin)
        {
            signalMin = signalSample;  // save just the min levels
        }
    }

    //SOUNDV = signalMax - signalMin;  // max - min = peak-peak amplitude
    SOUNDV = mapFloat((signalMax - signalMin), 0, 4095, 0, 3.3);

    //return 1;
    return SOUNDV;
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

void code(){
  //// prints device version and address

  //Serial.print("Device version: "); Serial.println(System.version());
  //Serial.print("Device ID: "); Serial.println(System.deviceID());
  //Serial.print("WiFi IP: "); Serial.println(WiFi.localIP());

  //// ***********************************************************************

  //// powers up sensors
  digitalWrite(I2CEN, HIGH);
  digitalWrite(ALGEN, HIGH);

  //// allows sensors time to warm up
  delay(SENSORDELAY);

  //// ***********************************************************************

  readWeatherSi7020();
  readSi1132Sensor();

  String tempStr = "";
  double sound = readSoundLevel();

  String sensorString = tempStr+"{\"id\":\"" + deviceID + "\", \"role\":\"" + role + "\",\"owner\":\"" + "Adam" + "\",\"status\":\"" + "On" + "\"}";

  sensorValue = digitalRead(inputPin);

  if(sensorValue == HIGH)
    sensorAttached = true;

  String responseString = "";

  //client.post(path, (const char*) sensorString, &responseString);

  Serial.println("---------");
  //Serial.println(sensorString);
  //Serial.println(responseString);

  delay(500);
}

void debug(){

  debugFlag = true;

  delay(500);

  noInterrupts();

  Serial.println("DEBUG_START");

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

  byte testByte = rand() % 255 + 1;

  int testPos = rand() % 2047 + 1;

  byte byteHolder = EEPROM.read(testPos);

  EEPROM.put(testPos, testByte);

  if(EEPROM.read(testPos) == testByte)
    Serial.println("  EEPROM: GO");
  else
    Serial.println("  EEPROM: NOGO");

  EEPROM.put(testPos, byteHolder);

  code();
  Serial.println("  LOOP: GO");

  //  TODO
  //  Server check
  //  Sensors check
  //  Wifi check
  //  Serial check
  //  Loop check;

  RGB.color(ledColour.r, ledColour.g, ledColour.b);

  debugFlag = false;

  Serial.println("DEBUG_END");

  interrupts();
}
