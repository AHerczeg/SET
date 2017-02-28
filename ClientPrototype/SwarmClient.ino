// UDP Port used for two way communication
unsigned int localPort = 8888;

// An UDP instance to let us send and receive packets over UDP
UDP Udp;

bool isLeader = false;
int promoteStart = -1;

Timer timer(10000, promoteSelf, true);
Timer leaderTimer(5000, setLeader, true);

byte leaderAddress[4] = {-1, -1, -1, -1};
byte multicastAddress[4] = {-1, -1, -1, -1};
IPAddress leaderIP = -1;
IPAddress multicastIP = -1;
IPAddress ownIP = -1;

char c = 'A';

Thread* swarmThread;

Thread* competitionThread;

os_thread_return_t swarm(){
  for(;;){
    if(isLeader){
      String ipString = WiFi.localIP();
      ipString = "" + ipString + ",244.0.0.0";
      Particle.publish("SwarmLeader", ipString);
      delay(5000);
    }
  }
}

os_thread_return_t competition(){
  for(;;){
    if(promoteStart >= 0){
      char tempArray[100];
      itoa(promoteStart, tempArray, 10);
      String timeString = tempArray;
      Particle.publish("SwarmCompetition", timeString);
    }
    delay(300);
  }
}

void swarmHandler(const char *event, const char *data)
{
  timer.start();
  if(leaderTimer.isActive()){
    leaderTimer.reset();
    leaderTimer.stop();
    promoteStart = -1;
  }
  String buffer = data;
  Serial.println(buffer);
  int divider = buffer.indexOf(',');
  ipSplit(buffer.substring(0, divider), 0);
  multicastSplit(buffer.substring(divider + 1), 0);
  if(leaderAddress[0] > -1 && leaderAddress[1] > -1 && leaderAddress[2] > -1 && leaderAddress[3] > -1)
    leaderIP = IPAddress(leaderAddress[0], leaderAddress[1], leaderAddress[2], leaderAddress[3]);
  if(isLeader && leaderIP == ownIP){
    Serial.println("Other leader detected");
    RGB.color(0,0,0);
    isLeader = false;
    timer.reset();
    timer.stop();
    promoteSelf();
  }
}

void competitionHandler(const char *event, const char *data)
{
  String buffer = data;
  if(promoteStart >= 0 && atoi(data) < promoteStart && leaderTimer.isActive()){
    leaderTimer.reset();
    leaderTimer.stop();
    promoteStart = -1;
    timer.start();
    timer.reset();
  }
}

void promoteSelf(){
  promoteStart = Time.now();
  Serial.println("Promoting self");
  leaderTimer.start();
}

void setLeader(){
  isLeader = true;
  promoteStart = -1;
  RGB.color(255,128,0);
  Serial.println("Set as leader");
  swarmThread = new Thread("swarm", swarm);
}

void setup() {
  // start the UDP
  Udp.begin(localPort);

  Serial.begin(9600);

  Particle.subscribe("SwarmLeader", swarmHandler);

  Particle.subscribe("SwarmCompetition", competitionHandler);

  timer.start();

  RGB.control(true);

  competitionThread = new Thread("competition", competition);

  ownIP = WiFi.localIP();

  delay(1000);

  Serial.println("Setup Done");
}

void loop() {
  if(c < 90)
    c++;
  else
    c = 'A';

  Serial.println("------------------------------");

  /*
  while(Udp.available()){
    char incomingMessage[100];
    Udp.parsePacket();
    Udp.read(incomingMessage, 100);
    Serial.print("Message: ");
    Serial.print(incomingMessage);
  }


  if(leaderAddress[0] > -1 && leaderAddress[1] > -1 && leaderAddress[2] > -1 && leaderAddress[3] > -1 && !isLeader){
    IPAddress leaderIP(leaderAddress[0], leaderAddress[1], leaderAddress[2], leaderAddress[3]);
    Udp.beginPacket(leaderIP, 8888);
    Udp.write(c);
    Udp.endPacket();
    Serial.println("Packet out");
  }

  if(multicastAddress[0] > -1 && multicastAddress[1] > -1 && multicastAddress[2] > -1 && multicastAddress[3] > -1 && !isLeader){
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
  */
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
