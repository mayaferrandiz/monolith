#include <SPI.h>
#include <SD.h>
#include <TMRpcm.h>
#include <Adafruit_NeoPixel.h>

//MOTOR VARIABLES

#define MOVEMENT_CYCLE_TIME 4500 //how long to extend/retract motors for

//pin1, pin2, motor#, board#, stateCode, last motor state change timestamp, is allowed to move, current LED brightness, last LED fade timestamp
//statecodes: 2 = fully extended, 1 = extending, -1 = retracting, -2 = fully retracted
unsigned long motors[9][10] = {
  { 36, 34, 4, 0, 0, 0, 0, 0, 0 },  //0 top

  { 10, 4, 2, 0, 0, 0, 0, 0, 0 },    //1 top front
  { 45, 43, 0, 0, 0, 0, 0, 0, 0 },  //2 top left
  { 48, 46, 0, 1, 0, 0, 0, 0, 0 },  //3 top back
  { 3, 2, 2, 1, 0, 0, 0, 0, 0 },    //4 top right

  { 44, 42, 1, 0, 0, 0, 0, 0, 0 },  //5 bottom front right
  { 7, 6, 3, 1, 0, 0, 0, 0, 0 },    //6 bottom front left
  { 9, 8, 3, 0, 0, 0, 0, 0, 0 },    //7 bottom back left
  { 40, 38, 1, 1, 0, 0, 0, 0, 0 }   //8 bottom back right
};

bool allMotorsExtended = false;
bool allMotorsRetracted = false;

//board conflicts
//1 and 4, 2 and 3, 6 and 7, 5 and 8
//two motors moving at once
int patterns[2][9] = {
  //{ 0, 5, 2, 8, 1, 7, 4, 6, 3},  //waves
  { 0, 1, 2, 4, 3, 5, 6, 8, 7},  //radials
  { 0, 1, 2, 4, 3, 5, 6, 8, 7}  //radials
};

int patternStep = 0;
int patternIndex = 0;
int completedPatternCount = -1;

//AUDIO VARIABLES

TMRpcm audio;
#define AUDIO_PIN_L 5
#define AUDIO_PIN_G 23
#define AUDIO_PIN_R 11

char const * audioFiles[3] = {
  "file1.wav",
  "file2.wav",
  "file3.wav"
};

//duration, amount played, playback started timestamp
unsigned long audioPlayback[3][3] = {
  {1000,0,0},
  {2000,0,0},
  {2000,0,0}
};

int audioIndex = 0;
char const * audioFileName = audioFiles[audioIndex];
unsigned long audioPosition = 0;

//LED VARIABLES

#define LED_COUNT 2 // Number of LEDs per strip

// Create Adafruit_NeoPixel objects for each LED strip
Adafruit_NeoPixel led_0(LED_COUNT, 47, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_1(LED_COUNT, 37, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_2(LED_COUNT, 41, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_3(LED_COUNT, 49, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_4(LED_COUNT, 31, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_5(LED_COUNT, 35, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_6(LED_COUNT, 29, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_7(LED_COUNT, 39, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_8(LED_COUNT, 33, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel* LEDs[9] = {
  &led_0,
  &led_1,
  &led_2,
  &led_3,
  &led_4,
  &led_5,
  &led_6,
  &led_7,
  &led_8
};

void setup() {
  Serial.begin(9600);

  SD.begin();
  audio.speakerPin = AUDIO_PIN_L;
  pinMode(AUDIO_PIN_R, OUTPUT);
  audio.loop(1);

  for (int motorIndex = 0; motorIndex < 9; motorIndex++) {
    pinMode(int(motors[motorIndex][0]), OUTPUT);
    pinMode(int(motors[motorIndex][1]), OUTPUT);
    LEDs[motorIndex]->begin();
    LEDs[motorIndex]->show(); // Turn off all LEDs initially
  }
}

//MOTORS

int getBoardNumber(int motorIndex) {
  return motors[motorIndex][2];
}

int getMotorNumber(int motorIndex) {
  return motors[motorIndex][3];
}

unsigned long getLastMotorMovementStateChangeTime(int motorIndex) {
  return motors[motorIndex][5];
}

void setIsFullyRetracted(int motorIndex) {
  motors[motorIndex][4] = -2;
}

void setIsFullyExtended(int motorIndex) {
  motors[motorIndex][4] = 2;
}

void setIsRetracting(int motorIndex) {
  motors[motorIndex][4] = -1;
}

void setIsExtending(int motorIndex) {
  motors[motorIndex][4] = 1;
}

void setLastMotorMovementStateChangeTime(int motorIndex, unsigned long time) {
  motors[motorIndex][5] = time;
}

void setMotorIsAllowedToMove(int motorIndex, bool isAllowed) {
  if (isAllowed) motors[motorIndex][6] = 1;
  else motors[motorIndex][6] = 0;
}

bool isRetracting(int motorIndex) {
  return motors[motorIndex][4] == -1;
}

bool isFullyRetracted(int motorIndex) {
  return motors[motorIndex][4] == -2;
}

bool isExtending(int motorIndex) {
  return motors[motorIndex][4] == 1;
}

bool isFullyExtended(int motorIndex) {
  return motors[motorIndex][4] == 2;
}

bool isAllowedToMove(int motorIndex) {
  return motors[motorIndex][6] == 1;
}

bool isMoving(int motorIndex) {
  return getLastMotorMovementStateChangeTime(motorIndex) + MOVEMENT_CYCLE_TIME > millis() && (isRetracting(motorIndex) || isExtending(motorIndex));
}

bool isMovementComplete(int motorIndex) {
  return getLastMotorMovementStateChangeTime(motorIndex) + MOVEMENT_CYCLE_TIME < millis();
}

bool isSameBoardMotorMoving(int motorIndex) {

  const int board = getBoardNumber(motorIndex);
  const int motor = getMotorNumber(motorIndex);

  int otherBoard = 0;
  int otherMotor = 0;

  bool otherMotorMoving = false;

  for (int otherMotorIndex = 0; otherMotorIndex < 9; otherMotorIndex++) {

    otherBoard = getBoardNumber(otherMotorIndex);
    otherMotor = getMotorNumber(otherMotorIndex);

    if (board == otherBoard && motor != otherMotor) otherMotorMoving = isMoving(otherMotorIndex);
  }

  return otherMotorMoving;
}

void printMotorState(int motorIndex) {
  Serial.print("Motor: ");
  Serial.print(motorIndex);
  Serial.print(" - Board #:");
  Serial.print(getBoardNumber(motorIndex));
  Serial.print(" - Motor #:");
  Serial.print(getMotorNumber(motorIndex));
  // Serial.print(" - State: ");
  // Serial.print(getMotorMovementState(motorIndex));
  Serial.print(" - Is allowed to move: ");
  Serial.print(isAllowedToMove(motorIndex));
  Serial.print(" - Last state change timestamp: ");
  Serial.print(getLastMotorMovementStateChangeTime(motorIndex));

  Serial.println();
}

void extend(int motorIndex) {
  if (!isSameBoardMotorMoving(motorIndex)) {
    setIsExtending(motorIndex);
    setLastMotorMovementStateChangeTime(motorIndex, millis());

    int pin1 = motors[motorIndex][0];
    int pin2 = motors[motorIndex][1];

    digitalWrite(pin1, LOW);
    digitalWrite(pin2, HIGH);

    Serial.print("Extending motor ");
    Serial.print(motorIndex);
    Serial.println();
  } else {
    int boardNumber = getBoardNumber(motorIndex);
    Serial.print("ERROR: Tried to move both motors on board ");
    Serial.print(boardNumber);
    Serial.println();
  }
}

void retract(int motorIndex) {
  if (!isSameBoardMotorMoving(motorIndex)) {
    setIsRetracting(motorIndex);
    setLastMotorMovementStateChangeTime(motorIndex, millis());

    int pin1 = motors[motorIndex][0];
    int pin2 = motors[motorIndex][1];

    digitalWrite(pin1, HIGH);
    digitalWrite(pin2, LOW);

    Serial.print("Retracting motor ");
    Serial.print(motorIndex);
    Serial.println();
  } else {
    int boardNumber = getBoardNumber(motorIndex);
    Serial.print("ERROR: Tried to move both motors on board ");
    Serial.print(boardNumber);
    Serial.println();
  }
}

void stop(int motorIndex) {
  int pin1 = motors[motorIndex][0];
  int pin2 = motors[motorIndex][1];

  digitalWrite(pin1, LOW);
  digitalWrite(pin2, LOW);

  Serial.print("Stopping motor ");
  Serial.print(motorIndex);
  Serial.println();
}

void extendAllMotors(){

  int stateVector = 0;

  for (int motorIndex = 0; motorIndex < 9; motorIndex++) {
    if (!isExtending(motorIndex) && !isFullyExtended(motorIndex) && !isSameBoardMotorMoving(motorIndex)) extend(motorIndex);
    else if (isExtending(motorIndex) && isMovementComplete(motorIndex)){
      setIsFullyExtended(motorIndex);
      stop(motorIndex);
      setMotorIsAllowedToMove(motorIndex, false);
    }
    stateVector = stateVector + motors[motorIndex][4];
  }

  if (stateVector == 18) allMotorsExtended = true;
}

void retractAllMotors(){

  int stateVector = 0;

  for (int motorIndex = 0; motorIndex < 9; motorIndex++) {
    if (!isRetracting(motorIndex) && !isFullyRetracted(motorIndex) && !isSameBoardMotorMoving(motorIndex)) retract(motorIndex);
    else if (isRetracting(motorIndex) && isMovementComplete(motorIndex)){
      setIsFullyRetracted(motorIndex);
      stop(motorIndex);
      setMotorIsAllowedToMove(motorIndex, false);
    }
    stateVector = stateVector + motors[motorIndex][4];
  }

  if (stateVector == -18) allMotorsExtended = true;
}

//LEDs

void setLEDBrightness(int motorIndex, uint8_t newBrightness){
  if (motors[motorIndex][7] != newBrightness){
    motors[motorIndex][7] = newBrightness;
    LEDs[motorIndex]->setPixelColor(0, LEDs[motorIndex]->Color(newBrightness, newBrightness, newBrightness));
    LEDs[motorIndex]->setPixelColor(1, LEDs[motorIndex]->Color(newBrightness, newBrightness, newBrightness));
    LEDs[motorIndex]->show();
  }
}

void turnOnLED(int motorIndex){
  setLEDBrightness(motorIndex, 255);
}

void turnOnAllLEDs(){
  for (int motorIndex = 0; motorIndex < 9; motorIndex++) {
    turnOnLED(motorIndex);
  }
}

void turnOffLED(int motorIndex){
  setLEDBrightness(motorIndex, 0);
}

void turnOffAllLEDs(){
  for (int motorIndex = 0; motorIndex < 9; motorIndex++) {
    turnOffLED(motorIndex);
  }
}

void fadeLED(int motorIndex, bool fadeIn) {
    unsigned long currentTime = millis();
    unsigned long lastFadeTime = motors[motorIndex][9];
    int currentBrightness = motors[motorIndex][8];

    // Calculate step delay for fading
    const int fadeSteps = 256; // 0 to 255 brightness levels
    int stepDelay = MOVEMENT_CYCLE_TIME / fadeSteps;

    if (currentTime - lastFadeTime >= stepDelay) {
        // Determine fade direction
        uint8_t newBrightness = fadeIn ? currentBrightness + 1 : currentBrightness - 1;
        newBrightness = constrain(newBrightness, 0, 255);

        // Update LEDs for the specific motor
        LEDs[motorIndex]->setPixelColor(0, LEDs[motorIndex]->Color(newBrightness, newBrightness, newBrightness));
        LEDs[motorIndex]->setPixelColor(1, LEDs[motorIndex]->Color(newBrightness, newBrightness, newBrightness));
        LEDs[motorIndex]->show();

        motors[motorIndex][8] = newBrightness;
        motors[motorIndex][9] = currentTime;  // Update last fade time
    }
}

//AUDIO

char const * getAudioFilename(int audioIndex){
  return audioFiles[audioIndex];
}

unsigned long getAudioTrackDuration(int audioIndex){
  return audioPlayback[audioIndex][0];
}

unsigned long getAudioPlaybackPosition(int audioIndex){
  return audioPlayback[audioIndex][1];
}

unsigned long getAudioPlaybackStartedTimestamp(int audioIndex){
  return audioPlayback[audioIndex][2];
}

void setAudioPlaybackStartedTimestamp(int audioIndex){
  audioPlayback[audioIndex][2] = millis();
}

void setAudioPlaybackPosition(int audioIndex){
  unsigned long now = millis();
  unsigned long lastPosition = getAudioPlaybackPosition(audioIndex);
  unsigned long playbackStartedTimestamp = getAudioPlaybackStartedTimestamp(audioIndex);
  unsigned long duration = getAudioTrackDuration(audioIndex);

  unsigned long incrementalPlayback = now - playbackStartedTimestamp;

  if (lastPosition + incrementalPlayback > duration) audioPlayback[audioIndex][1] = (lastPosition + incrementalPlayback)%duration;
  else audioPlayback[audioIndex][1] = lastPosition + incrementalPlayback;
}

void nextAudio(){
  setAudioPlaybackPosition(audioIndex);
  audioIndex = (audioIndex+1)%3;
  audioFileName = getAudioFilename(audioIndex);
  audioPosition = getAudioPlaybackPosition(audioIndex) / 1000;
  audio.play("switch.wav",0,1);
  audio.play(audioFileName,audioPosition,0);
  setAudioPlaybackStartedTimestamp(audioIndex);

  Serial.print("Playing audio file ");
  Serial.print(audioFileName);
  Serial.println();
}

//MOTOR MOVEMENT PATTERN

void nextPattern(){
  patternIndex = (patternIndex+1)%2;
  completedPatternCount = completedPatternCount+1;
  Serial.print("Switching to pattern ");
  Serial.print(patternIndex);
  Serial.println();
}

void nextPatternStep(){
  patternStep = (patternStep + 1) % 9;
}

bool isPatternComplete(){
  return patternStep == 8;
}

void loop() {

  if (completedPatternCount = -1){

    if (!allMotorsRetracted) {
      retractAllMotors();
      turnOnAllLEDs();
    }
    else {
      Serial.print("Starting movement sequence");
      Serial.println();
      turnOffAllLEDs();
      setMotorIsAllowedToMove(patterns[patternIndex][patternStep], true);
      completedPatternCount = 0;
    }

  } else {

    for (int motorIndex = 0; motorIndex < 9; motorIndex++) {
      if (isAllowedToMove(motorIndex)) {
        if (isFullyRetracted(motorIndex) && !isExtending(motorIndex)) {
          nextAudio();
          turnOnLED(motorIndex);
          extend(motorIndex);
        }
        else if (isExtending(motorIndex) && isMovementComplete(motorIndex) && !isFullyExtended(motorIndex)) {
          setIsFullyExtended(motorIndex);
          stop(motorIndex);
        } else if (isFullyExtended(motorIndex) && !isRetracting(motorIndex)) {
          retract(motorIndex);
          turnOffLED(motorIndex);
          if (isPatternComplete()) nextPattern();
          nextPatternStep();
          setMotorIsAllowedToMove(patterns[patternIndex][patternStep], true);
        } else if (isRetracting(motorIndex) && isMovementComplete(motorIndex) && !isFullyRetracted(motorIndex)) {
          setIsFullyRetracted(motorIndex);
          stop(motorIndex);
          setMotorIsAllowedToMove(motorIndex, false);
        }
      }
    }
  }
}