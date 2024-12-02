#include <SPI.h>
#include <SD.h>
#include <TMRpcm.h>
#include <Adafruit_NeoPixel.h>

//MOTOR VARIABLES
#define LED_COUNT 2 // Number of LEDs per strip
static const unsigned int MOTOR_COUNT = 9;
static const unsigned int MAX_POSITION = 4500; //how long to extend/retract motors for in milliseconds
static const unsigned int MIN_POSITION = 0;
bool runPatterns = false;

const char* concatenate(const char* text, uint8_t number) {
    static char buffer[16]; // Adjust size as needed
    char numChar = '0' + number; // Convert number to character ('0' + 2 = '2')
    snprintf(buffer, sizeof(buffer), "%s %c", text, numChar);
    return buffer;
}

Adafruit_NeoPixel led_0(LED_COUNT, 47, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_1(LED_COUNT, 37, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_2(LED_COUNT, 41, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_3(LED_COUNT, 49, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_4(LED_COUNT, 31, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_5(LED_COUNT, 35, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_6(LED_COUNT, 29, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_7(LED_COUNT, 39, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_8(LED_COUNT, 33, NEO_GRB + NEO_KHZ800);

struct Motor {
  const uint8_t pin1;
  const uint8_t pin2;
  const uint8_t motorNumber;
  const uint8_t boardNumber;
  const uint8_t id;
  Adafruit_NeoPixel* led;
  int position = MAX_POSITION;
  int targetPosition = MIN_POSITION;
  unsigned long lastStateChange = 0;
  uint8_t ledBrightness = 0;
  bool isInitialized = false;
  bool isStopped = false;
  bool isMoving = false;
  bool isWaiting = false;
  bool printedMoveLogForThisTarget = false;

  bool reachedTarget(){ return position == targetPosition; }
  void setInitialized(){
    isInitialized = true;
    this->setLEDBrightness(0);
    this->print("✅ Initialized");
  }
  void setTargetPosition(unsigned int target){
    if (target != targetPosition){
      lastStateChange = millis();
      targetPosition = constrain(target,MIN_POSITION,MAX_POSITION); 
      printedMoveLogForThisTarget = false;
      this->print("New Target");
    }
  }
  void wait(uint8_t id){
    if (!isWaiting){
      lastStateChange = millis();
      isWaiting = true;
      if (!isStopped) this->print( concatenate("Waiting ", id) );
    }
  }
  void noWait(uint8_t id){
    if (isWaiting){
      lastStateChange = millis();
      isWaiting = false;
      if (!isStopped) this->print( concatenate("Not Waiting ", id) );
    }
  }
  void stop() {
    isStopped = true;
    isMoving = false;
    isWaiting = false;
    lastStateChange = millis();

    this->print("Stopping");
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, LOW);
  }
  void move(){
    isMoving = true;
    isStopped = false;
    isWaiting = false;

    if (!printedMoveLogForThisTarget) {
      printedMoveLogForThisTarget = true;
      this->print("Moving");
    }
    int direction = targetPosition > position ? 1 : -1;
    unsigned long now = millis();
    int incrementalMotorPosition = now - lastStateChange;
    int nextPosition = position + direction*incrementalMotorPosition;
    position = constrain(nextPosition,MIN_POSITION,MAX_POSITION);
    
    lastStateChange = now;

    if (direction == 1){
      this->setLEDBrightness(255);
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, HIGH);
    } 
    else if (direction == -1){

      if (isInitialized) this->setLEDBrightness(0);
      digitalWrite(pin1, HIGH);
      digitalWrite(pin2, LOW);
    }
  }
  void setLEDBrightness(uint8_t newBrightness){
    if (ledBrightness != newBrightness){
      ledBrightness = newBrightness;
      led->setPixelColor(0, led->Color(newBrightness, newBrightness, newBrightness));
      led->setPixelColor(1, led->Color(newBrightness, newBrightness, newBrightness));
      led->show();
    }
  }
  void print(const char * eventName){
    Serial.print("[");
    Serial.print(id);
    Serial.print("] ");
    Serial.print(eventName);
    Serial.print("; ");
    Serial.print(position);
    Serial.print("->");
    Serial.print(targetPosition);
    Serial.print(" ");
    if (isStopped) Serial.print("🔴");
    if (isMoving) Serial.print("🟢");
    if (isWaiting) Serial.print("🟡");
    Serial.println();
  }

  Motor(uint8_t p1, uint8_t p2, uint8_t bNum, uint8_t mNum, uint8_t id, Adafruit_NeoPixel* ledObj) : pin1(p1), pin2(p2), boardNumber(bNum), motorNumber(mNum), id(id), led(ledObj) {}
};

Motor motor2(45, 43, 0, 0, 2, &led_2);
Motor motor3(48, 46, 0, 1, 3, &led_3);
Motor motor5(44, 42, 1, 0, 5, &led_5);
Motor motor8(40, 38, 1, 1, 8, &led_8);
Motor motor1(10, 4 , 2, 0, 1, &led_1);
Motor motor4(3 , 2 , 2, 1, 4, &led_4);
Motor motor6(7 , 6 , 3, 1, 6, &led_6);
Motor motor7(9 , 8 , 3, 0, 7, &led_7);
Motor motor0(36, 34, 4, 0, 0, &led_0);

Motor m[MOTOR_COUNT] = {motor0,motor1,motor2,motor3,motor4,motor5,motor6,motor7,motor8};

struct PatternStep {
  const uint8_t motor;
  const uint8_t target;
}

struct Pattern {
  const uint8_t[MOTOR_COUNT] steps;
}

static const uint8_t PATTERN_COUNT = 2;
uint8_t patternStep = 0;
uint8_t patternIndex = 0;

//board conflicts
//1 and 4, 2 and 3, 6 and 7, 5 and 8
//two motors moving at once
const uint8_t patterns[PATTERN_COUNT][MOTOR_COUNT] = {
  { 0, 5, 2, 8, 1, 7, 4, 6, 3},  //waves
  { 0, 1, 2, 4, 3, 5, 6, 8, 7}  //radials
};

//AUDIO VARIABLES

TMRpcm audio;
#define AUDIO_PIN_L 5
#define AUDIO_PIN_G 23
#define AUDIO_PIN_R 11

struct Audio {
  char const * fileName;
  unsigned long duration;
  unsigned long seekPosition;
  unsigned long playbackStartedTimestamp;

  void saveSeekPosition(){
    unsigned long now = millis();
    unsigned long incrementalPlayback = now - playbackStartedTimestamp;

    if (seekPosition + incrementalPlayback > duration) seekPosition = (seekPosition + incrementalPlayback)%duration;
    else seekPosition = seekPosition + incrementalPlayback;
  }

  Audio(char const * fn, unsigned long dur, unsigned long seek, unsigned long ts) : fileName(fn), duration(dur), seekPosition(seek), playbackStartedTimestamp(ts) {}
};

Audio audio1("file1.wav", 1000, 0, 0);
Audio audio2("file2.wav", 1000, 0, 0);

static const uint8_t AUDIO_COUNT = 2;
uint8_t audioIndex = 0;
Audio au[AUDIO_COUNT] = {audio1,audio2};

void setup() {
  Serial.begin(9600);

  SD.begin();
  audio.speakerPin = AUDIO_PIN_L;
  pinMode(AUDIO_PIN_R, OUTPUT);
  audio.loop(1);

  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    pinMode(m[i].pin1, OUTPUT);
    pinMode(m[i].pin2, OUTPUT);
    m[i].led->begin();
    m[i].led->show();
  }
}

bool allMotorsInitialized(){
  uint8_t countInitialized = 0;
  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    if (m[i].isInitialized) countInitialized = countInitialized+1;
  }
  return countInitialized == MOTOR_COUNT;
}

uint8_t getSameBoardMotorId(uint8_t i){
  uint8_t boardNumber = m[i].boardNumber;
  uint8_t motorNumber = m[i].motorNumber;
  uint8_t otherBoard = 0;
  uint8_t otherMotor = 0;

  uint8_t id = 255;

  for (uint8_t j = 0; j < MOTOR_COUNT; j++) {

    otherBoard = m[j].boardNumber;
    otherMotor = m[j].motorNumber;

    if (boardNumber == otherBoard && motorNumber != otherMotor) id = j;
  }

  return id;
}

bool isSameBoardMotorMoving(uint8_t i){
  uint8_t other = getSameBoardMotorId(i);
  if (other == 255) return false;
  else return !m[other].reachedTarget() && !m[other].isStopped && !m[other].isWaiting;
}

void playNextAudio(){
  au[audioIndex].saveSeekPosition();
  audioIndex = (audioIndex+1)%AUDIO_COUNT;
  audio.play("switch.wav",0,1);
  audio.play(au[audioIndex].fileName, au[audioIndex].seekPosition, 0);
  au[audioIndex].playbackStartedTimestamp = millis();
}

uint8_t nextMotor(){
  if (patternStep == MOTOR_COUNT-1) patternIndex = (patternIndex+1) % PATTERN_COUNT;
  patternStep = (patternStep + 1) % MOTOR_COUNT;
  uint8_t id = patterns[patternIndex][patternStep];
  m[id].print("Next Motor");
  playNextAudio();
  return id;
}

void loop() {

  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {

    if (!m[i].isInitialized) m[i].setLEDBrightness(255);
    if (isSameBoardMotorMoving(i) && !m[i].isWaiting) m[i].wait( getSameBoardMotorId(i) );
    if (!isSameBoardMotorMoving(i) && m[i].isWaiting) m[i].noWait( getSameBoardMotorId(i) );
    if (!m[i].reachedTarget() && !m[i].isWaiting) m[i].move();
    if (m[i].reachedTarget() && !m[i].isStopped) {
      
      m[i].stop();

      if (!runPatterns && !allMotorsInitialized()) m[i].setInitialized();
      if (!runPatterns && allMotorsInitialized()) {
        Serial.println("All initialized");
        runPatterns = true;
        m[ nextMotor() ].setTargetPosition(MAX_POSITION);
      }
      
      if (runPatterns) {
        m[i].setTargetPosition(0);
        if (m[i].position == MAX_POSITION) m[ nextMotor() ].setTargetPosition(MAX_POSITION);
      }
    }
  }
}