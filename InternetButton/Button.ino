#include "InternetButton.h"
#include "rest_client.h"

#define GREEN {0, 255, 0}
#define YELLOW {255, 255, 0}
#define RED {255, 0, 0}

InternetButton b = InternetButton();

RestClient client = RestClient("sccug-330-05.lancs.ac.uk",5000);

const char* path = "/button";

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} COLOUR;

COLOUR ledColour = {0, 0, 0};

int lightMode = 1;

int brightness = 0;

int lightSpeed = 0;

Thread* buttonThread;
Thread* blinkingThread;
Thread* accelThread;

os_thread_return_t buttonListener(){
  for(;;){
    bool b1 = false;
    bool b2 = false;
    bool b3 = false;
    bool b4 = false;
    while(!b.allButtonsOff()){
      if(b.buttonOn(1))
        b1 = true;
      if(b.buttonOn(2))
        b2 = true;
      if(b.buttonOn(3))
        b3 = true;
      if(b.buttonOn(4))
        b4 = true;
      delay(100);
    }
    delay(100);
  }
}

os_thread_return_t light(){
  for(;;){
    switch(lightMode){
      case 0: blinking();
              break;
      case 1: circle();
              break;
      default: delay(100);
    }


  }
}

void setup() {
    Serial.begin(9600);
    b.begin();
    ledColour = GREEN;
    buttonThread = new Thread("buttonListener", buttonListener);
    blinkingThread = new Thread("light", light);
    b.allLedsOn(ledColour.r, ledColour.g, ledColour.b);
}


void loop() {
  code();
}

void code(){
  delay(2000);
}

void buttonPressed(int button){
  if(button >= 0 && button <= 4){

  }
}

void blinking(){
  while(brightness < 255){
    brightness += 5;
    if(brightness > 255)
      brightness = 255;
    b.setBrightness(brightness);
    b.allLedsOn(ledColour.r, ledColour.g, ledColour.b);
    delay (10 + lightSpeed * 10);
  }
  while(brightness > 0){
    brightness -= 5;
    if(brightness < 0)
      brightness = 0;
    b.setBrightness(brightness);
    b.allLedsOn(ledColour.r, ledColour.g, ledColour.b);
    delay (10 + lightSpeed * 10);
  }
  delay(500);
}

void circle(){
  int i;
  for(i = 1; i < 12; i++){
    b.allLedsOff();
    b.ledOn(i, ledColour.r * 0.1, ledColour.g * 0.1, ledColour.b * 0.1);
    b.ledOn(((i+1)/12 + (i+1)%12), ledColour.r * 0.2, ledColour.g * 0.2, ledColour.b * 0.2);
    b.ledOn(((i+2)/12 + (i+2)%12), ledColour.r * 0.3, ledColour.g * 0.3, ledColour.b * 0.3);
    b.ledOn(((i+3)/12 + (i+3)%12), ledColour.r * 0.4, ledColour.g * 0.4, ledColour.b * 0.4);
    b.ledOn(((i+4)/12 + (i+4)%12), ledColour.r * 0.5, ledColour.g * 0.5, ledColour.b * 0.5);
    b.ledOn(((i+5)/12 + (i+5)%12), ledColour.r * 0.6, ledColour.g * 0.6, ledColour.b * 0.6);
    b.ledOn(((i+6)/12 + (i+6)%12), ledColour.r * 0.7, ledColour.g * 0.7, ledColour.b * 0.7);
    b.ledOn(((i+7)/12 + (i+7)%12), ledColour.r * 0.8, ledColour.g * 0.8, ledColour.b * 0.8);
    b.ledOn(((i+8)/12 + (i+8)%12), ledColour.r * 0.9, ledColour.g * 0.9, ledColour.b * 0.9);
    b.ledOn(((i+9)/12 + (i+9)%12), ledColour.r, ledColour.g, ledColour.b);
    delay(100 + lightSpeed * 10);
  }
}
