// UDP Port used for two way communication
unsigned int localPort = 8888;

// An UDP instance to let us send and receive packets over UDP
UDP Udp;

bool isLeader = false;
int promoteStart = -1;

Timer timer(10000, promoteSelf);
Timer leaderTimer(10000, setLeader);

int leaderAddress[4] = {-1, -1, -1, -1};
int multicastAddress[4] = {-1, -1, -1, -1};

char c = 'A';

Thread* swarmThread;

// Thread for blinking the LED
os_thread_return_t swarm(){
  for(;;){
    String ipString = WiFi.localIP();
    ipString = "" + ipString + ",244.0.0.0";
    Particle.publish("SwarmLeader", ipString);
    String serialString = "Sending IP address <" + ipString + ">";
    //Serial.println(serialString);
    delay(5000);
  }
}

void swarmHandler(const char *event, const char *data)
{
  String buffer = data;
  int divider = buffer.indexOf(',');
  ipSplit(buffer.substring(0, divider), 0);
  multicastSplit(buffer.substring(divider + 1), 0);
  timer.reset();
}

void competitionHandler(const char *event, const char *data)
{
  String buffer = data;
  if(promoteStart >= 0 && atoi(data) < promoteStart && leaderTimer.isActive())
    leaderTimer.stop();
}

void promoteSelf(){
  promoteStart = Time.now();
  Particle.publish("SwarmCompetition", promoteStart);
  leaderTimer.start();
  leaderTimer.reset();
}

void setLeader(){
  isLeader = true;
  swarmThread = new Thread("swarm", swarm);
}

void setup() {
  // start the UDP
  Udp.begin(localPort);

  Serial.begin(9600);

  Particle.subscribe("SwarmLeader", swarmHandler);

  Particle.subscribe("SwarmCompetition", competitionHandler);

  timer.start();
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

  if(multicastAddress[0] > -1 && multicastAddress[1] > -1 && multicastAddress[2] > -1 && multicastAddress[3] > -1){
    IPAddress multicastIP(multicastAddress[0], multicastAddress[1], multicastAddress[2], multicastAddress[3]);
    Udp.joinMulticast(multicastIP);
    while(Udp.available()){
      char incomingMessage[100];
      Udp.parsePacket();
      Udp.read(incomingMessage, 100);
      Serial.print("Message: ");
      Serial.print(incomingMessage);
    }
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

void multicastSplit(String data, int i){
  int index = data.indexOf('.') + 1;
  if(index == 0){
    multicastAddress[i] = atoi(data);
  } else {
    multicastAddress[i] = atoi(data.substring(0, index));
    ipSplit(data.substring(index, data.length()), (i+1));
  }
}
