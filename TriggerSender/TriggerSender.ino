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
        addTrigger();
      } else if (buffer.compareTo("list") == 0){
        listTriggers();
      } else if (buffer.compareTo("reset") == 0){
        resetOwner();
      }
      buffer = "";
      delay(100);
    }
  }
}


void setup()
{
    // opens serial over USB
    Serial.begin(9600);

    debugThread = new Thread("debug", serialListener);

}


void loop(void)
{
      code();
      delay(5000);
}

void code(){
}

void addTrigger(){
  String buffer;
  String trigger = "";
  String tempStr = "";
  if(owner.length() < 1){
    Serial.println("Please specify username: ");
    while(Serial.available() <= 0)
      delay(100);
    if(Serial.available() > 0){
      Serial.flush();
      while (Serial.available() > 0) {
        if(Serial.peek() != 10){
          buffer = tempStr + buffer + (char)(Serial.read());
        } else {Serial.read();}
      }
      Serial.println("Incoming serial: " + buffer);
      owner = buffer;
      buffer = "";
      delay(100);
    }
  }
  Serial.println("Please add trigger: ");
  while(Serial.available() <= 0)
    delay(100);
  if(Serial.available() > 0){
    Serial.flush();
    while (Serial.available() > 0) {
      if(Serial.peek() != 10){
        buffer = tempStr + buffer + (char)(Serial.read());
      } else {Serial.read();}
    }
    Serial.println("Incoming serial: " + buffer);
    trigger = buffer;
    buffer = "";
    delay(100);
  }
  String sendingString = tempStr + "{\"trigger_string\": \"" + trigger + "\" , \"active\": true , \"owner\": \"" + owner +"\"}"; 
  String responseString;
  Serial.println(sendingString);
  client.post(path, (const char*) sendingString, &responseString);
  Serial.println(responseString);
}

void listTriggers(){
  Serial.println("<Trigger>, <Trigger>, <Trigger>");
}

void resetOwner(){
  owner = "";
}
