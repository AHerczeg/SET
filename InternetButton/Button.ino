#include "InternetButton.h"
#include "rest_client.h"


InternetButton b = InternetButton();

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} COLOUR;

COLOUR ledColour = {0, 0, 0};

COLOUR green = {0, 1, 0};
COLOUR yellow = {1, 1, 0};
COLOUR red = {1, 0, 0};

int brightness = 0;

int securityLevel = 0;

Thread* buttonThread;
Thread* blinkingThread;

os_thread_return_t buttonListener(){
  for(;;){
    while(securityLevel <= 1){
      securityLevel++;
      delay(10000);
    }
    while(securityLevel >= 1){
      securityLevel--;
      delay(10000);
    }
  }
}

os_thread_return_t blinking(){
  for(;;){
    switch(securityLevel){
      case 0: ledColour = green;
              break;
      case 1: ledColour = yellow;
              break;
      case 2: ledColour = red;
              break;
    }
    while(brightness < 255){
      brightness += 5;
      if(brightness > 255)
        brightness = 255;
      b.allLedsOn(ledColour.r * brightness,ledColour.g * brightness,ledColour.b * brightness);
      delay ((3 - securityLevel) * 10);
    }
    while(brightness > 0){
      brightness -= 5;
      if(brightness < 0)
        brightness = 0;
      b.allLedsOn(ledColour.r * brightness,ledColour.g * brightness,ledColour.b * brightness);
      delay ((3 - securityLevel) * 10);
    }
    delay(500);
  }
}

void setup() {
    Serial.begin(9600);
    b.begin();
    ledColour = green;
    buttonThread = new Thread("buttonListener", buttonListener);
    blinkingThread = new Thread("blinking", blinking);
}


void loop() {
  code();
}

void code(){
  delay(500);
}
