#include "InternetButton.h"
#include "rest_client.h"

#define GREEN {0, 255, 0}
#define YELLOW {255, 255, 0}
#define RED {255, 0, 0}

InternetButton b = InternetButton();

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} COLOUR;

COLOUR ledColour = {0, 0, 0};


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
      case 0: ledColour = GREEN;
              break;
      case 1: ledColour = YELLOW;
              break;
      case 2: ledColour = RED;
              break;
    }
    while(brightness < 255){
      brightness += 5;
      if(brightness > 255)
        brightness = 255;
      b.setBrightness(brightness);
      b.allLedsOn(ledColour.r, ledColour.g, ledColour.b);
      delay ((3 - securityLevel) * 10);
    }
    while(brightness > 0){
      brightness -= 5;
      if(brightness < 0)
        brightness = 0;
      b.setBrightness(brightness);
      b.allLedsOn(ledColour.r, ledColour.g, ledColour.b);
      delay ((3 - securityLevel) * 10);
    }
    delay(500);
  }
}

void setup() {
    Serial.begin(9600);
    b.begin();
    ledColour = GREEN;
    buttonThread = new Thread("buttonListener", buttonListener);
    blinkingThread = new Thread("blinking", blinking);
    b.allLedsOn(ledColour.r, ledColour.g, ledColour.b);
}


void loop() {
  code();
}

void code(){
  delay(500);
}
