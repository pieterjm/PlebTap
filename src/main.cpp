#include <Arduino.h>
#include "TFT_eSPI.h"
#include <ESP32Servo.h>
#include <OneButton.h>


#define PIN_BUTTON_1                 0
#define PIN_BUTTON_2                 14

Servo servo1;
Servo servo2;
TFT_eSPI tft = TFT_eSPI();
OneButton button1(PIN_BUTTON_1, true);
OneButton button2(PIN_BUTTON_2, true);

#define SERVO_PIN 16

int servoAngle = 0;

void updateServo()
{
  if ( servoAngle < 0 ) {
    servoAngle = 0;
  }
  if ( servoAngle > 180 ) {
    servoAngle = 180;
  }
  
  tft.drawString(String(servoAngle) + "                ",10,10,2);
  servo1.write(servoAngle);
  servo2.write(servoAngle);
  delay(20);
}

void setup() {
  // Initialise serial port for debugging output
  Serial.begin(115200);

  // initialise TFT
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  servo1.attach(SERVO_PIN);
  servo2.attach(13);

  button1.attachClick([]() {
    servoAngle += 1;
    updateServo();
  });
  button1.attachDuringLongPress([]()  {
    servoAngle += 1;
    updateServo();
  });
  button2.attachClick([]() {
    servoAngle -= 1;
    updateServo();
  });
  button2.attachDuringLongPress([]()  {
    servoAngle -= 1;
    updateServo();
  });

  tft.drawString("Press a button",10,10,2);
}

void loop() {
  if ( Serial.available() != 0 ) {
    servoAngle = Serial.readStringUntil('\n').toInt();
    updateServo();
  }
  button1.tick();
  button2.tick();
}