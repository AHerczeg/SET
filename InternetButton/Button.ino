#include "InternetButton.h"
#include "rest_client.h"

#define GREEN {0, 255, 0}
#define YELLOW {255, 255, 0}
#define RED {255, 0, 0}
#define BLUE {0, 0, 255}
#define WHITE {255, 255, 255}
#define LIGHTS_ON "/lights/on"
#define LIGHTS_OFF "/lights/off"
#define KETTLE_ON "/kettle/on"
#define KETTLE_OFF "/kettle/off"

InternetButton b = InternetButton();

RestClient client = RestClient("sccug-330-05.lancs.ac.uk",5000);

const char* path = "/lights/on";

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} COLOUR;

COLOUR ledColour = {0, 0, 0};

int lightMode;
int brightness;
int lightSpeed;

// -- EXAMPLE code for demo --
COLOUR allColours[] = {RED, GREEN, BLUE, YELLOW, WHITE};
int currentColour = 0;
bool kettleMode = false;
bool bulbOn = true;
// ---------------------------

Thread* buttonThread;
Thread* blinkingThread;
Thread* serialThread;

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
      if(buffer.compareTo("kettle") == 0){
        kettleMode = true;
        ledColour = BLUE;
        b.allLedsOn(ledColour.r, ledColour.g, ledColour.b);
      } else if (buffer.compareTo("light") == 0){
        kettleMode = false;
        ledColour = WHITE;
        b.allLedsOn(ledColour.r, ledColour.g, ledColour.b);
      }

      buffer = "";
    }
  }
}

os_thread_return_t buttonListener(){
  for(;;){
    int pressed = buttonPressed();
    if(pressed > 0){
      // -- EXAMPLE code for demo --
      if(kettleMode){

        switch(pressed){
          case 15:  path = KETTLE_ON;
                    client.get(path);
                    colourFade(255, 0, 0);
                    Serial.println("Switching kettle on");
                    break;
        }
      } else {
        switch(pressed){
          case 1: if(!lightMode){
                    currentColour -= 1;
                    if(currentColour < 0)
                      currentColour = 4;
                    ledColour = allColours[currentColour];
                  } else {
                    brightness -= 25;
                    if(brightness < 0)
                      brightness = 0;
                    b.setBrightness(brightness);
                  }
                  break;
          case 2: if(lightMode == 0)
                    lightMode = 1;
                  else
                    lightMode = 0;
                  break;
          case 4: if(!lightMode){
                    currentColour += 1;
                    if(currentColour > 4)
                      currentColour = 0;
                    ledColour = allColours[currentColour];
                  } else {
                    brightness += 25;
                    if(brightness > 255)
                      brightness = 255;
                    b.setBrightness(brightness);
                  }
                  break;
          case 8: if(lightMode == 0)
                    lightMode = 1;
                  else
                    lightMode = 0;
                  break;
          case 15:  if(bulbOn){
                      path = LIGHTS_OFF;
                      bulbOn = false;
                      client.get(path);
                      Serial.println("Switching lights off");
                    } else {
                      path = LIGHTS_ON;
                      bulbOn = true;
                      client.get(path);
                      Serial.println("Switching lights on");
                    }
                    break;
        }

      }
      // ---------------------------
    }
    delay(100);
  }
}

os_thread_return_t light(){
  for(;;){
    switch(lightMode){
      case 0: blinking(lightMode);
              break;
      case 1: circle(lightMode);
              break;
      default:  b.allLedsOn(ledColour.r, ledColour.g, ledColour.b);
                delay(100);
    }
  }
}

void setup() {
    Serial.begin(9600);
    b.begin();

    lightMode = -1;
    brightness = 0;
    lightSpeed = 5;
    ledColour = WHITE;

    buttonThread = new Thread("buttonListener", buttonListener);
    blinkingThread = new Thread("light", light);
    serialThread = new Thread("serial", serialListener);
    b.allLedsOn(ledColour.r, ledColour.g, ledColour.b);
}


void loop() {
  code();
}

void code(){
  delay(5000);
}

int buttonPressed(){
  uint8_t button = 0x00;
  while(!b.allButtonsOff()){
    if(b.buttonOn(1))
      button = button | 0x01;
    if(b.buttonOn(2))
      button = button | 0x02;
    if(b.buttonOn(3))
      button = button | 0x04;
    if(b.buttonOn(4))
      button = button | 0x08;
    delay(100);
  }
  return (int) button;
}

void blinking(int myMode){
  while(brightness < 255 && lightMode == myMode){
    brightness += 5;
    if(brightness > 255)
      brightness = 255;
    b.setBrightness(brightness);
    b.allLedsOn(ledColour.r, ledColour.g, ledColour.b);
    delay (10 + lightSpeed * 10);
  }
  delay(500 + lightSpeed * 10);
  while(brightness > 0 && lightMode == myMode){
    brightness -= 5;
    if(brightness < 0)
      brightness = 0;
    b.setBrightness(brightness);
    b.allLedsOn(ledColour.r, ledColour.g, ledColour.b);
    delay (10 + lightSpeed * 10);
  }
  delay(500 + lightSpeed * 10);
}

void circle(int myMode){
  int i;
  for(i = 1; i < 12 && lightMode == myMode; i++){
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
    delay(100 + lightSpeed * 5);
  }
}

void setColour(int red, int green, int blue){
  if (red <= 255 && red >= 0 && green <= 255 && green >= 0 && blue <= 255 && blue >= 0)
    ledColour = {red, green, blue};
}

void setMode(int mode){
  if(mode >= 0)
    lightMode = mode;
}

void setSpeed(int speed){
  if(speed >= 0)
    lightSpeed = speed;
}

void setBrightness(int newBrightness){
  if(brightness >= 0 && brightness <= 255)
    brightness = newBrightness;
}

void colourFade(int red, int green, int blue){
  if (red <= 255 && red >= 0 && green <= 255 && green >= 0 && blue <= 255 && blue >= 0)
  {
    while(ledColour.r != red || ledColour.g != green || ledColour.b != blue){
      if(ledColour.r < red){
        if(ledColour.r <= 250)
          ledColour.r += 5;
        else
          ledColour.r = 255;
      } else if(ledColour.r > red){
        if(ledColour.r >= 5)
          ledColour.r -= 5;
        else
          ledColour.r = 0;
      }
      if(ledColour.g < green){
        if(ledColour.g <= 250)
          ledColour.g += 5;
        else
          ledColour.g = 255;
      } else if(ledColour.g > green){
        if(ledColour.g >= 5)
          ledColour.g -= 5;
        else
          ledColour.g = 0;
      }
      if(ledColour.b < blue){
        if(ledColour.b <= 250)
          ledColour.b += 5;
        else
          ledColour.b = 255;
      } else if(ledColour.b > blue){
        if(ledColour.b >= 5)
          ledColour.b -= 5;
        else
          ledColour.b = 0;
      }
      delay(300);
    }
  }
}
