// UDP Port used for two way communication
unsigned int localPort = 8888;

// An UDP instance to let us send and receive packets over UDP
UDP Udp;

Thread* swarmThread;

// Thread for blinking the LED
os_thread_return_t swarm(){
  for(;;){
    String ipString = WiFi.localIP();
    Particle.publish("SwarmLeader", ipString);
    String serialString = "Sending IP address <" + ipString + ">";
    Serial.println(serialString);
    delay(5000);
  }
}


void setup() {
  // start the UDP
  Udp.begin(localPort);

  Serial.begin(9600);

  swarmThread = new Thread("swarm", swarm);

}

void loop() {
  // Check if data has been received

  if (Udp.parsePacket() > 0) {

    // Read first char of data received
    char c = Udp.read();

    Serial.println(c);

    // Ignore other chars
    while(Udp.available())
      Udp.read();

    // Store sender ip and port
    IPAddress ipAddress = Udp.remoteIP();
    int port = Udp.remotePort();

    Udp.beginPacket(ipAddress, port);
    Udp.write('B');
    Udp.endPacket();
  }
}
