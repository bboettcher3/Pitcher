#include <SD.h>
#include <ArduinoJson.h>
#include <math.h>

// Pin definitions
#define PIN_BUTTON_0 23
#define PIN_BUTTON_1 19
#define PIN_BUTTON_2 18
#define PIN_BUTTON_3 5
#define PIN_JOYSTICK_X 4
#define PIN_JOYSTICK_Y 14
#define PIN_JOYSTICK_SW 15
// Constants
#define MIN_NOTE_NUMBER 60 // C4
#define MAX_JOYSTICK_DIST 724.1 // if both axes are length 512
#define NOT_PLAYING -1
const int NOTE_TRIGGERS[7][4] = {
  {1, 0, 0, 0},
  {0, 1, 0, 0},
  {0, 0, 1, 0},
  {0, 0, 0, 1},
  {0, 0, 1, 1},
  {0, 1, 1, 1},
  { 1, 1, 1, 1}
};

// Bookkeeping
DynamicJsonDocument mScaleData(32065);
int mCurScaleIdx;
int mCurNotePlaying; // MIDI note # playing
float mJoystickX, mJoystickY;
bool mButton0Pressed, mButton1Pressed, mButton2Pressed, mButton3Pressed, mJoystickPressed;

void startNote(int noteNum) {
  // Send MIDI note on
  usbMIDI.sendNoteOn(noteNum, 127, 1);
  mCurNotePlaying = noteNum;
}

void stopNote() {
  // Send MIDI note off
  usbMIDI.sendNoteOff(mCurNotePlaying, 0, 1);
  mCurNotePlaying = NOT_PLAYING;
}

void parseJson(const char* filename, DynamicJsonDocument& json) {
  if (SD.exists(filename)) { // file exists, reading and loading
    File file = SD.open(filename, "r");   // Open file for reading
    DeserializationError error = deserializeJson(json, file);
    if (error) Serial.println("Failed to read json file");
    file.close();
  } else {
    Serial.println("Json file doesn't exist");
  }
}

void setup() {
  // Initialize serial port
  Serial.begin(9600);
  // Load SD library for json parsing
  while (!SD.begin()) {
    Serial.println("Failed to initialize SD library");
    delay(1000);
  }
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
  mCurScaleIdx = 0;
  mCurNotePlaying = NOT_PLAYING;
}

void loop() {
  // Update current scale from joystick values
  mJoystickX = analogRead(PIN_JOYSTICK_X) - 512; // balance around 0 (o.g ranged 0..1023)
  mJoystickY = analogRead(PIN_JOYSTICK_Y) - 512;
  // Calc distance from center and angle
  float joystickDist = sqrt(pow(mJoystickX, 2) + pow(mJoystickY, 2)) / MAX_JOYSTICK_DIST; // ranged 0..1
  float angle = atan2(mJoystickY, mJoystickX) + M_PI; // ranged 0..2*PI
  int numSlices = mScaleData["scales"][mCurScaleIdx]["adjacent_scales"].size();
  int selectedIdx = int((angle / 2*PI) * numSlices);
  JsonVariant curPlayingScale = mScaleData["scales"][mCurScaleIdx];
  // Change to corresponding adjacent scale if joystick pushed far enough
  if (joystickDist > 0.5) {
    String newScaleName = mScaleData["scales"][mCurScaleIdx]["adjacent_scales"][selectedIdx]["name"].as<String>();
    for (int i = 0; i < mScaleData["scales"].size(); ++i) {
      if (mScaleData["scales"][i]["name"].as<String>() == newScaleName) {
        // Scale found, replace current value
        curPlayingScale = mScaleData["scales"][i];
        break;
      }
    }
  }
  // Check for new scale latch from joystick press
  bool joystickPressed = digitalRead(PIN_JOYSTICK_SW);
  if (joystickPressed && !mJoystickPressed && joystickDist > 0.5) {
    // New scale latch triggered, find and update current scale
    String newScaleName = mScaleData["scales"][mCurScaleIdx]["adjacent_scales"][selectedIdx]["name"].as<String>();
    for (int i = 0; i < mScaleData["scales"].size(); ++i) {
      if (mScaleData["scales"][i]["name"].as<String>() == newScaleName) {
        // Scale found, replace current value
        mCurScaleIdx = i;
        break;
      }
    }
  }
  mJoystickPressed = joystickPressed;
  
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
  // Search for note trigger
  for (int offsetIdx = 0; offsetIdx < 7; ++offsetIdx) {
    bool matched = (mButton0Pressed == NOTE_TRIGGERS[offsetIdx][0]) &&
                   (mButton1Pressed == NOTE_TRIGGERS[offsetIdx][1]) &&
                   (mButton2Pressed == NOTE_TRIGGERS[offsetIdx][2]) &&
                   (mButton3Pressed == NOTE_TRIGGERS[offsetIdx][3]);
    if (matched) {
      if (mCurNotePlaying != NOT_PLAYING) {
        stopNote();
      }
      // Trigger MIDI note from current scale
      int root = curPlayingScale["root"].as<int>();
      // Find root index in pitch classes
      for (int pitchIdx = 0; pitchIdx < curPlayingScale["pitch_classes"].size(); ++pitchIdx) {
        if (curPlayingScale["pitch_classes"][pitchIdx].as<int>() == root) {
          // Calc offset from root pitch
          int notePitchIdx = (pitchIdx + offsetIdx) % curPlayingScale["pitch_classes"].size();
          int pitchOffset = (curPlayingScale["pitch_classes"][notePitchIdx].as<int>() - root + 12) % 12;
          int noteNum = MIN_NOTE_NUMBER + root + pitchOffset;
          startNote(noteNum);
          Serial.print("trig note: ");
          Serial.println(noteNum);
          break;
        }
      }
    } else {
      stopNote();
    }
  }
  
  Serial.print("Button states -> ");
  Serial.print(mButton0Pressed);
  Serial.print(", ");
  Serial.print(mButton1Pressed);
  Serial.print(", ");
  Serial.print(mButton2Pressed);
  Serial.print(", ");
  Serial.println(mButton3Pressed);
}
