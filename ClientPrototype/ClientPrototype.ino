
typedef struct{
  int temperature;
  int humidity;
  int light;
  int sound;
  int motion;
} SENSOR_VALUE;

SENSOR_VALUE sensorValue;

uint8_t sensors = 0x00;

unsigned int localPort = 8888;

UDP Udp;

byte leaderAddress[4] = {192,  168, 0, 57};
IPAddress leaderIP = -1;

void setup()
{
    // opens serial over USB
    Serial.begin(9600);

    Udp.begin(localPort);

    leaderIP = IPAddress(leaderAddress[0], leaderAddress[1], leaderAddress[2], leaderAddress[3]);

    sensorValue.temperature = 10;
    sensorValue.humidity = 20;
    sensorValue.light = 30;
    sensorValue.sound = 40;
    sensorValue.motion = 50;
}


void loop(void)
{
  Serial.println(leaderIP);
  String sensorData = "temp:10;hum:20;light:30;sound:40.5;motion:61.76";
  Serial.print(Udp.sendPacket(sensorData, sizeof(sensorData), leaderIP, 8888));
  Serial.println("");
  delay(3000);
}
