#include "MPU9150.h"
#include "Si1132.h"
#include "Si70xx.h"
#include "math.h"
// TODO
#include "rest_client.h"


//// ***************************************************************************
//// ***************************************************************************

//// Initialize application variables
#define RAD_TO_DEGREES 57.2957795131
#define DEG_TO_RADIANS 0.0174533
#define PI 3.1415926535
#define ACCEL_SCALE 2 // +/- 2g

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


// TODO
MPU9150 mpu9150;
bool ACCELOK = false;
int cx, cy, cz, ax, ay, az, gx, gy, gz;
double tm; //// Celsius
bool change = false;

Si1132 si1132 = Si1132();

int inputPin = D6; // PIR motion sensor. D6 goes HIGH when motion is detected and LOW when
                   // there's no motion.

int sensorState = LOW;        // Start by assuming no motion detected
int sensorValue = 0;
bool regularUpdate;
bool sensorAttached = false;

// TODO
ApplicationWatchdog watchDog(60000, System.reset);

// TODO
Timer sleepTimer(360000, startSleep);

// TODO
bool isSleeping = false;

// TODO
RestClient client = RestClient("sccug-330-05.lancs.ac.uk",5000);

// TODO
const char* path = "/Cup";

// TODO
String coreID;

int rank = 0;

Thread* rankThread;

//TODO
os_thread_return_t rankDetect(void* param){

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

    // diables interrupts
    interrupts();

    // initialises the IO pins
    setPinsMode();

    // initialises MPU9150 inertial measure unit
    initialiseMPU9150();

    // Initialize motion sensor input pin
    pinMode(inputPin, INPUT);

    // TODO
    regularUpdate = true;

    System.enableReset();

    coreID = getCoreID();

    // TODO
    sleepTimer.start();

    rankThread = new Thread("rank", rankDetect);
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

    RestClient client = RestClient("sccug-330-04.lancs.ac.uk",8000);

    const char* path = "/Logs";

    String tempStr = "";
    double sound = readSoundLevel();
    bool motionChange = false;
    time_t time = Time.minute();
    bool runUpdate =  ( (int)time == 0 || (int)time == 30);

    if (!runUpdate)
      regularUpdate = true;

    //regularUpdate = (runUpdate && regularUpdate);

    String sensorString = tempStr+"{\"CoreID\":\"" + coreID + "\"";
    double diff = oldTmp-Si7020Temperature;

    bool startUpdate = (runUpdate && regularUpdate);


    // THESE TWO MUST ALWAYS BE ON TOP! V


    sensorValue = digitalRead(inputPin);

    if(sensorValue == HIGH)
      sensorAttached = true;

    if(sensorAttached)
    {
      if (sensorValue == HIGH && sensorState == LOW)   // If the input pin is HIGH turn LED ON
      {
         sensorString =  sensorString + ", \"Motion\": 1";
         Particle.publish("Team3Motion", "1");
         sensorState = HIGH;                 // preserves current sensor state
         change = true;
      } else if (sensorValue == LOW && sensorState == HIGH) {
          sensorString = sensorString + ", \"Motion\": 0";
          Particle.publish("Team3Motion", "0");
          sensorState = LOW;                   // preserves current sensor state
          change = true;
      }

      if(startUpdate && !change)
      {
         if (sensorState == HIGH)
          sensorString = sensorString + ", \"Motion\": 1";
         else
          sensorString = sensorString + ", \"Motion\": 0";
      }
    }

    // THESE TWO MUST ALWAYS BE ON TOP! ^

    String check = getChange(Si7020Temperature, oldTmp, THRESHOLD, "Temp", startUpdate);
    if(check != "no"){
      sensorString = sensorString + check;
      change = true;
    }


    check = getChange(Si7020Humidity, oldHmd, THRESHOLD, "Humidity", startUpdate);
    if(check != "no"){
      sensorString = sensorString + check;
      change = true;
    }

    check = getChange(Si1132Visible, oldVisible, THRES_L, "Light", startUpdate);
    if(check != "no"){
      sensorString = sensorString + check;
      change = true;
    }

    check = getChange(sound, oldSound, THRES_S, "Sound", startUpdate);
    if(check != "no"){
      sensorString = sensorString + check;
      change = true;
    }

    sensorString = sensorString + "}";
    //WiFi RSSI guide: >50, it's in zone 1; between 50 and 45, in zone 2; <45, in zone 3

    Serial.println(sensorString);
    String responseString = "";

    // || time.minute() == 0 || time.minute() == 30

    //Particle.publish("regularUpdate", sensorString, PRIVATE);

    if(change)
    {
      sleepTimer.reset();
      if(isSleeping)
        endSleep();
      client.post(path, (const char*) sensorString, &responseString);
      change = false;

    }

    sensorString = sensorString+" "+ (sound);
    //Particle.publish("photonSensorData",sensorString, PRIVATE);

    String rTime = tempStr + "" + (int)time;

    if(startUpdate)
      regularUpdate = false;

    delay(500);

    watchDog.checkin();
}

String getChange(double &current, double &old, int thres, String name, bool start)
{
  int diff = 0;
  double d = 0;
  String sensorString = "no";

  if(name == "Sound")
    d = lround((current - old)*1000);
  else
    d = current - old;

  diff = abs(d);

  if(diff > thres || start)
  {
    old = current;
    change = true;
    sensorString =  ", \"" + name + "\":"+ current;
  }

  return sensorString;
}

String getCoreID(){
  String coreIdentifier = "";
  char id[12];
  memcpy(id, (char *)ID1, 12);
  char hex_digit;
  for (int i = 0; i < 12; ++i) {
    hex_digit = 48 + (id[i] >> 4);
    if (57 < hex_digit)
      hex_digit += 39;
    coreIdentifier = coreIdentifier + hex_digit;
    hex_digit = 48 + (id[i] & 0xf);
    if (57 < hex_digit)
      hex_digit += 39;
    coreIdentifier = coreIdentifier + hex_digit;
  }
  return coreIdentifier;
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

void startSleep(){
  sleepTimer.stop();
  String nameString = coreID + "Sleep start";
  Particle.publish("photonSensorData",nameString, PRIVATE);
  isSleeping = true;
  System.sleep(1);
}

void endSleep(){
  WiFi.on();
  WiFi.connect();
  Spark.connect();
  Particle.process();
  delay(500);
  String nameString = coreID + "Sleep end";
  Particle.publish("photonSensorData",nameString, PRIVATE);
  isSleeping = false;
  sleepTimer.reset();
}
