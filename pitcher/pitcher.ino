#include "SPIFFS.h"
#include <ArduinoJson.h>

// Pin definitions
#define PIN_BUTTON_0 23
#define PIN_BUTTON_1 19
#define PIN_BUTTON_2 18
#define PIN_BUTTON_3 5
#define PIN_JOYSTICK_X 4
#define PIN_JOYSTICK_Y 14
#define PIN_JOYSTICK_SW 15

// Bookkeeping
DynamicJsonDocument mScaleData(32065);
float mJoystickX, mJoystickY;
bool mButton0Pressed, mButton1Pressed, mButton2Pressed, mButton3Pressed, mJoystickPressed;

void setup() {
  // Initialize serial port
  Serial.begin(9600);
  // Mount filesystem
  mountFS();
  // Parse scale data json
  parseJson("/ScalesData.json", mScaleData);
  // Setup button pins
  pinMode(PIN_BUTTON_0, INPUT_PULLUP);
  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);
  pinMode(PIN_BUTTON_3, INPUT_PULLUP);
  mButton0Pressed = false;
  mButton1Pressed = false;
  mButton2Pressed = false;
  mButton3Pressed = false;
  // Setup joystick pins
  pinMode(PIN_JOYSTICK_X, INPUT_PULLUP);
  pinMode(PIN_JOYSTICK_Y, INPUT_PULLUP);
  pinMode(PIN_JOYSTICK_SW, INPUT_PULLUP);
  mJoystickX = 0;
  mJoystickY = 0;
  mJoystickPressed = false;
}

void loop() {
  // Update current scale from joystick values
  mJoystickX = analogRead(PIN_JOYSTICK_X);
  mJoystickX = analogRead(PIN_JOYSTICK_Y);
  mJoystickPressed = digitalRead(PIN_JOYSTICK_SW);
  Serial.print("Joystick values-> x: ");
  Serial.print(mJoystickX);
  Serial.print(", y: ");
  Serial.print(mJoystickY);
  Serial.print(", pressed: ");
  Serial.println(mJoystickPressed);
  // Check button states, output MIDI when pressed from respective scale
  mButton0Pressed = digitalRead(PIN_BUTTON_0);
  mButton1Pressed = digitalRead(PIN_BUTTON_1);
  mButton2Pressed = digitalRead(PIN_BUTTON_2);
  mButton3Pressed = digitalRead(PIN_BUTTON_3);
  Serial.print("Button states -> ");
  Serial.print(mButton0Pressed);
  Serial.print(", ");
  Serial.print(mButton1Pressed);
  Serial.print(", ");
  Serial.print(mButton2Pressed);
  Serial.print(", ");
  Serial.println(mButton3Pressed);
}
