#include "rest_client.h"


RestClient client = RestClient("sccug-330-05.lancs.ac.uk",5000);

const char* path = "/trigger";

String deviceID;

Thread* debugThread;

os_thread_return_t serialListener(){
  String buffer = "";
  String tempStr = "";
  String responseString = "";
  for(;;){
    if(Serial.available() > 0){
      Serial.flush();
      while (Serial.available() > 0) {
        if(Serial.peek() != 10){
          buffer = tempStr + buffer + (char)(Serial.read());
        } else {Serial.read();}
      }
      Serial.println("Incoming serial: " + buffer);
      String responseString;
      client.post(path, (const char*) buffer, &responseString);
      Serial.println(responseString);
      buffer = "";
      delay(100);
    }
  }
}


void setup()
{
    // opens serial over USB
    Serial.begin(9600);

    // enables interrupts
    interrupts();

    System.enableReset();

    deviceID = System.deviceID();


    debugThread = new Thread("debug", serialListener);

}


void loop(void)
{
      code();
      delay(5000);
}

void code(){
}
