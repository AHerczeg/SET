
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

  int udpSize = Udp.parsePacket();
  while(Udp.available()){
    Serial.println("UDP");
    char incomingMessage[30];
    Udp.read(incomingMessage, udpSize);
    Serial.print("Size of message: ");
    Serial.print(udpSize);
    Serial.print("  Message: ");
    int i = 0;
    Serial.print(String(incomingMessage));
    Serial.print("\n");
    dataParser(incomingMessage);
    /*
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
    */
    delay(1000);
  }

  Serial.println("--------------------------------------");

  delay(1000);
}

void dataParser(String data){
  int index = data.indexOf(';') + 1;
  if(index == 0){
    Serial.print("\ndata: ");
    Serial.print(data);
    Serial.print("\n");
    dataSwitch(data);
  } else {
    Serial.print("\ndata: ");
    Serial.print(data.substring(0, index));
    Serial.print("\n");
    dataSwitch(data.substring(0, index));
    dataParser(data.substring(index, data.length()));
  }
}


void dataSwitch(String data){
  int index = data.indexOf(':') + 1;
  switch(atoi(data.substring(0, index))){
    case 1:  Serial.println(data.substring(index, data.length()));
                  break;

    case 2: Serial.println(data.substring(index, data.length()));
                break;

    case 3: Serial.println(data.substring(index, data.length()));
                  break;

    case 4: Serial.println(data.substring(index, data.length()));
                  break;

    case 5:  Serial.println(data.substring(index, data.length()));
                    break;

    default: Serial.println("ERROR");
  }
}
