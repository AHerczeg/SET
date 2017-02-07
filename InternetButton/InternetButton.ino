#include "InternetButton.h"

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

void setup() {
    Serial.begin(9600);
    b.begin();
    ledColour = green;
}


void loop() {
  switch(securityLevel){
    case 0: ledColour = green;
            break;
    case 1: ledColour = yellow;
            break;
    case 2: ledColour = red;
            break;
  }
  while(brightness < 255){
    brightness += 10;
    if(brightness > 255)
      brightness = 255;
    b.allLedsOn(ledColour.r * brightness,ledColour.g * brightness,ledColour.b * brightness);
    delay ((3 - securityLevel) * 300);
  }
  while(brightness > 0){
    brightness -= 10;
    if(brightness < 0)
      brightness = 0;
    b.allLedsOn(ledColour.r * brightness,ledColour.g * brightness,ledColour.b * brightness);
    delay ((3 - securityLevel) * 300);
  }
  delay(1000);
}
