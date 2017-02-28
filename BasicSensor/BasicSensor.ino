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
#define TEMP_SENSOR 0x80
#define HUM_SENSOR 0x40
#define LIGHT_SENSOR 0x20
#define ACCEL_SENSOR 0x10
#define MOTION_SENSOR 0x08
#define SOUND_SENSOR 0x04
#define INTERNET_BUTTON 0x02

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


RestClient client = RestClient("sccug-330-05.lancs.ac.uk",5000);

const char* path = "/sensor_history";

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

uint8_t sensors = 0x00;

Thread* serialThread;
Thread* ledThread;

os_thread_return_t serialListener(){
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
      } else if (buffer.compareTo("scanning") == 0){
        Serial.println("response");
      }

      buffer = "";
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

    serialThread = new Thread("debug", serialListener);

    String tempStr = "";
    String sensorString = tempStr+"{\"id\":\"" + deviceID + "\", \"role\":\"" + role + "\",\"owner\":\"" + "Adam" + "\",\"status\":\"" + "On" + "\"}";
    client.post(path, (const char*) sensorString);

    readWeatherSi7020();
    readSi1132Sensor();
}

void initialiseMPU9150()
{
  ACCELOK = mpu9150.begin(mpu9150._addr_motion); // Initialize MPU9150

  if (ACCELOK)
  {
      sensors = sensors | ACCEL_SENSOR;
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


// The code is outside of the loop so we can control when it's being ran
void code(){

  //// ***********************************************************************

  //// powers up sensors
  digitalWrite(I2CEN, HIGH);
  digitalWrite(ALGEN, HIGH);

  //// allows sensors time to warm up
  delay(SENSORDELAY);

  //// ***********************************************************************

  readWeatherSi7020();
  readSi1132Sensor();
  double sound = readSoundLevel();

  Time.setFormat(TIME_FORMAT_ISO8601_FULL);

  String tempStr = "";
  String timestamp = timestampFormat();

  String sensorData = tempStr+"{\\\"temperature\\\": " + String(Si7020Temperature) + ", \\\"humidity\\\": " + String(Si7020Humidity) + ", \\\"light\\\": " + String(Si1132Visible) + "}";

  String sensorString = tempStr+"{\"sensorId\":\"" + deviceID + "\", \"sensorData\":\"" + sensorData + "\",\"timestamp\":\"" + timestamp + "\"}";

  String responseString = "";

  client.post(path, (const char*) sensorString, &responseString);

  Serial.println("---------");
  Serial.println(sensorString);
  Serial.println(responseString);

  delay(1000);
}

// Read sound measurements
void readMPU9150()
{
    //// reads the MPU9150 sensor values. Values are read in order of temperature,
    //// compass, Gyro, Accelerometer

    tm = ( (double) mpu9150.readSensor(mpu9150._addr_motion, MPU9150_TEMP_OUT_L, MPU9150_TEMP_OUT_H) + 12412.0 ) / 340.0;
    cx = mpu9150.readSensor(mpu9150._addr_motion, MPU9150_CMPS_XOUT_L, MPU9150_CMPS_XOUT_H);  //Compass_X
    cy = mpu9150.readSensor(mpu9150._addr_motion, MPU9150_CMPS_YOUT_L, MPU9150_CMPS_YOUT_H);  //Compass_Y
    cz = mpu9150.readSensor(mpu9150._addr_motion, MPU9150_CMPS_ZOUT_L, MPU9150_CMPS_ZOUT_H);  //Compass_Z
    ax = mpu9150.readSensor(mpu9150._addr_motion, MPU9150_ACCEL_XOUT_L, MPU9150_ACCEL_XOUT_H);
    ay = mpu9150.readSensor(mpu9150._addr_motion, MPU9150_ACCEL_YOUT_L, MPU9150_ACCEL_YOUT_H);
    az = mpu9150.readSensor(mpu9150._addr_motion, MPU9150_ACCEL_ZOUT_L, MPU9150_ACCEL_ZOUT_H);
    gx = mpu9150.readSensor(mpu9150._addr_motion, MPU9150_GYRO_XOUT_L, MPU9150_GYRO_XOUT_H);
    gy = mpu9150.readSensor(mpu9150._addr_motion, MPU9150_GYRO_YOUT_L, MPU9150_GYRO_YOUT_H);
    gz = mpu9150.readSensor(mpu9150._addr_motion, MPU9150_GYRO_ZOUT_L, MPU9150_GYRO_ZOUT_H);


}

// Reas temperature and humidity measurements
int readWeatherSi7020()
{
    Si70xx si7020;
    Si7020OK = si7020.begin(); //// initialises Si7020

    if (Si7020OK)
    {
        sensors = sensors | TEMP_SENSOR;
        sensors = sensors | HUM_SENSOR;
        Si7020Temperature = si7020.readTemperature();
        Si7020Humidity = si7020.readHumidity();
    }

    return Si7020OK ? 2 : 0;
}



///reads UV, visible and InfraRed light level
void readSi1132Sensor()
{
    if((sensors & TEMP_SENSOR) > 0 && (sensors & HUM_SENSOR) > 0)
      sensors = sensors | LIGHT_SENSOR;
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

    if( SOUNDV > 0.02)
      sensors = sensors | SOUND_SENSOR;
    //return 1;
    return SOUNDV;
}

// Slowly blinks the LED
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

// Converts time into specific format
String timestampFormat(){
  String tempStr = "";
  String timestamp = tempStr + String(Time.year()) + "-";
  if(Time.month() < 10)
    timestamp += tempStr + "0" + String(Time.month()) + "-";
  else
    timestamp += tempStr + String(Time.month()) + "-";
  if(Time.day() < 10)
    timestamp += tempStr + "0" + String(Time.day()) + "T";
  else
    timestamp += tempStr + String(Time.day()) + "T";
  if(Time.hour() < 10)
    timestamp += tempStr + "0" + String(Time.hour()) + ":";
  else
    timestamp += tempStr + String(Time.hour()) + ":";
  if(Time.minute() < 10)
    timestamp += tempStr + "0" + String(Time.minute()) + ":";
  else
    timestamp += tempStr + String(Time.minute()) + ":";
  if(Time.second() < 10)
    timestamp += tempStr + "0" + String(Time.second());
  else
    timestamp += tempStr + String(Time.second());

  return timestamp;
}

void sendDataToServer(){

}

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
  delay(1000);
  RGB.color(255, 0, 0);
  delay(1000);
  RGB.color(0, 255, 0);
  delay(1000);
  RGB.color(0, 0, 255);
  delay(1000);
  RGB.color(255, 255, 0);
  delay(1000);
  RGB.color(255, 0, 255);
  delay(1000);
  RGB.color(0, 255, 255);
  delay(1000);
  RGB.color(255, 255, 255);
  delay(1000);

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

  path = pathHolder;

  // List all recognised sensors
  Serial.println("  SENSORS");
  if((sensors & TEMP_SENSOR) > 0){
    Serial.println("    TEMPERATURE");
  }
  if((sensors & HUM_SENSOR) > 0){
    Serial.println("    HUMIDITIY");
  }
  if((sensors & LIGHT_SENSOR) > 0){
    Serial.println("    LIGHT");
  }
  if((sensors & ACCEL_SENSOR) > 0){
    Serial.println("    ACCELERATION");
  }
  if((sensors & MOTION_SENSOR) > 0){
    Serial.println("    MOTION");
  }
  if((sensors & SOUND_SENSOR) > 0){
    Serial.println("    SOUND");
  }
  if((sensors & INTERNET_BUTTON) > 0){
    Serial.println("    INTERNET BUTTON");
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
