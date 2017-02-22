#include "rest_client.h"


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

TCPClient client;
byte server[] = {192, 168, 0, 103};

void setup()
{
    // opens serial over USB
    Serial.begin(9600);

    debugThread = new Thread("debug", serialListener);

    client.connect(server, 23);
}


void loop(void)
{
      code();
      delay(1000);
}

void code(){
  Serial.println("------------");
  if (client.connected())
  {
    Serial.println("sending");
    client.write(10);
  } else {
    client.connect(server, 23);
  }
  String tempStr = "";
  String buffer = "";
  while(client.available())
  {
    char receivedByte = client.read();
    if(receivedByte != 10)
      buffer = tempStr + buffer + receivedByte;
    else
      Serial.println(buffer);
  }
}
