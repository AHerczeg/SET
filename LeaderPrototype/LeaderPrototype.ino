
uint8_t sensors = 0x00;

typedef struct{
  int temperature;
  int humidity;
  int light;
  int sound;
  int motion;
} SENSOR_VALUE;

typedef union {
  int temperatureInputs;
  int humidityInputs;
  int lightInputs;
  int soundInputs;
  int motionInputs;
  SENSOR_VALUE sensorValue;
} SHARED_VALUES;

SENSOR_VALUE sensorValue;

SHARED_VALUES sharedValues;

uint8_t activatedSensors = 0x00;

unsigned int localPort = 8888;

UDP Udp;

bool isLeader = true;

void setup()
{
    // opens serial over USB
    Serial.begin(9600);
    RGB.control(false);
    Udp.begin(localPort);
}

void loop(void)
{

  Serial.println(WiFi.localIP());

  while(Udp.available()){
    Serial.println("UDP");
    char incomingMessage[100];
    int udpSize = Udp.parsePacket();
    Udp.read(incomingMessage, udpSize);
    Serial.print("Message: ");
    Serial.print(incomingMessage);
    SENSOR_VALUE sValue;
    memcpy(&sValue, incomingMessage, sizeof(sValue));
    Serial.print("\nShared temperature: ");
    Serial.print(sValue.temperature);
    Serial.print("\nShared humidity: ");
    Serial.print(sValue.humidity);
    Serial.print("\nShared light: ");
    Serial.print(sValue.light);
    Serial.print("\nShared sound: ");
    Serial.print(sValue.sound);
    Serial.print("\nShared motion: ");
    Serial.print(sValue.motion);
  }

  Serial.println("--------------------------------------");

  delay(1000);
}
