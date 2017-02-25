// UDP Port used for two way communication
unsigned int localPort = 8888;

// An UDP instance to let us send and receive packets over UDP
UDP Udp;

int leaderAddress[4] = {-1, -1, -1, -1};

char c = 'A';

void swarmHandler(const char *event, const char *data)
{
  String buffer = data;
  ipSplit(data, 0);
}

void setup() {
  // start the UDP
  Udp.begin(localPort);

  Serial.begin(9600);

  Particle.subscribe("SwarmLeader", swarmHandler);
}

void loop() {
  if(c < 90)
    c++;
  else
    c = 'A';


  if(leaderAddress[0] > -1 && leaderAddress[1] > -1 && leaderAddress[2] > -1 && leaderAddress[3] > -1){
    IPAddress leaderIP(leaderAddress[0], leaderAddress[1], leaderAddress[2], leaderAddress[3]);
    Udp.beginPacket(leaderIP, 8888);
    Udp.write(c);
    Udp.endPacket();
    Serial.println("Packet out");
  }

  delay(500);


  //Udp.beginPacket(ipAddress, port);
  //Udp.write(c);
  //Udp.endPacket();
}

void ipSplit(String data, int i){
  int index = data.indexOf('.') + 1;
  if(index == 0){
    leaderAddress[i] = atoi(data);
  } else {
    leaderAddress[i] = atoi(data.substring(0, index));
    ipSplit(data.substring(index, data.length()), (i+1));
  }
}
