#include "rest_client.h"


RestClient client = RestClient("sccug-330-05.lancs.ac.uk",5000);

const char* path = "/trigger";

String owner = "";

Thread* debugThread;

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
      if(buffer.compareTo("add") == 0){
        Serial.println("---------");
      }
      buffer = "";
      delay(100);
    }
  }
}

TCPServer server = TCPServer(23);

TCPClient tcpClient;

void setup()
{
    // opens serial over USB
    Serial.begin(9600);

    debugThread = new Thread("debug", serialListener);

    server.begin();

    Serial.println(WiFi.localIP());
    Serial.println(WiFi.subnetMask());
    Serial.println(WiFi.gatewayIP());
    Serial.println(WiFi.SSID());

}


void loop(void)
{
      code();
      delay(500);
}

void code(){
  Serial.println("------------");
  Serial.println(WiFi.localIP());
  if (tcpClient.connected()) {
    // echo all available bytes back to the client
    while (tcpClient.available()) {
      Serial.println(tcpClient.read());
      server.println("Welcome to the swarm");
    }
  } else {
    // if no client is yet connected, check for a new connection
    tcpClient = server.available();
  }
}
