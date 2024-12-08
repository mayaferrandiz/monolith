#include <SPI.h>
#include <SD.h>
#include <TMRpcm.h>
#include <Adafruit_NeoPixel.h>
#define LED_COUNT 2 // Number of LEDs per strip
#define MOTOR_COUNT 9
#define MAX_MOTORS_MOVING 2
#define MAX_POSITION 4500
#define MIN_POSITION 0

TMRpcm audio;

struct Pattern;

template <typename T>
class EntityCycler {
  private:
      T* objects;
      size_t count;
      size_t index = 0;
  public:
      EntityCycler(T* objArray, size_t objCount) : objects(objArray), count(objCount) {}

    T& getCurrent() const { return objects[index]; }
    T& next() { index = (index + 1) % count; }
    bool reachedEnd() { return index == count-1; }
    size_t getIndex() { return index; }
};

struct Motor {
  const uint8_t pin1;
  const uint8_t pin2;
  const uint8_t sameBoardMotorID;
  const uint8_t ID;
  Adafruit_NeoPixel* led;
  int position = MAX_POSITION;
  int targetPosition = MIN_POSITION;
  unsigned long lastStateChange = 0;
  uint8_t ledBrightness = 0;
  bool isStopped = true;
  bool canMove = false;
  bool isMoving = false;
  bool completedPattern = false;
  bool printedMoveLogForThisTarget = false;

  void init(){
    this->print("Init");
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
    led->begin();
    led->show();
  }
  bool reachedTarget(){ return position == targetPosition; }
  void setTargetPosition(unsigned int target){
    if (target != targetPosition){
      unsigned long now = millis();
      lastStateChange = now;
      targetPosition = constrain(target,MIN_POSITION,MAX_POSITION); 
      printedMoveLogForThisTarget = false;
      this->print("New Target");
    }
  }
  void stop() {
    isStopped = true;
    canMove = false;
    isMoving = false;
    lastStateChange = millis();

    this->print("Stopping");
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, LOW);
  }
  void allowMove(){
    canMove = true;
    lastStateChange = millis();
  }
  void move(){
    isMoving = true;
    isStopped = false;

    if (!printedMoveLogForThisTarget) {
      printedMoveLogForThisTarget = true;
      this->print("Moving");
    }

    unsigned long now = millis();
    int direction = targetPosition > position ? 1 : -1;
    
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
      this->setLEDBrightness(0);
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
    Serial.print(ID);
    Serial.print("] ");
    if (isStopped) Serial.print("🔴 ");
    if (isMoving) Serial.print("🟢 ");
    Serial.print(eventName);
    Serial.print("; ");
    Serial.print(position);
    Serial.print("->");
    Serial.print(targetPosition);
    Serial.print(" ");
    Serial.println();
  }

  Motor(uint8_t p1, uint8_t p2, uint8_t ID, uint8_t sameBoardMotorID, Adafruit_NeoPixel* ledObj) : pin1(p1), pin2(p2), sameBoardMotorID(sameBoardMotorID), ID(ID), led(ledObj) {}
};

struct Pattern : public EntityCycler<uint8_t> {
    int destination;
    int origin; //will return here after reaching destination

    uint8_t getMotorID(){ return EntityCycler::getCurrent(); }

    Pattern(uint8_t* arr, int destination = MAX_POSITION, int origin = MIN_POSITION) : EntityCycler<uint8_t>(arr, MOTOR_COUNT), destination(destination), origin(origin) {}
};

struct AudioTrack {
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

  AudioTrack(char const * fn, unsigned long dur, unsigned long seek, unsigned long ts) : fileName(fn), duration(dur), seekPosition(seek), playbackStartedTimestamp(ts) {}
};

Adafruit_NeoPixel led_0(LED_COUNT, 47, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_1(LED_COUNT, 41, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_2(LED_COUNT, 30, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_3(LED_COUNT, 49, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_4(LED_COUNT, 35, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_5(LED_COUNT, 39, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_6(LED_COUNT, 32, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_7(LED_COUNT, 33, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_8(LED_COUNT, 37, NEO_GRB + NEO_KHZ800);

Motor motor0(36, 34, 0, 255, &led_0);

Motor motor2(45, 43, 2, 3, &led_2);
Motor motor3(48, 46, 3, 2, &led_3);

Motor motor5(44, 42, 5, 8, &led_5);
Motor motor8(40, 38, 8, 5, &led_8);

Motor motor1(4 , 3 , 1, 4, &led_1);
Motor motor4(14, 15, 4, 1, &led_4);

Motor motor6(7 , 6 , 6, 7, &led_6);
Motor motor7(9 , 8 , 7, 6, &led_7);

Motor m[MOTOR_COUNT] = {motor0,motor1,motor2,motor3,motor4,motor5,motor6,motor7,motor8};

static const uint8_t PATTERN_COUNT = 3;
uint8_t radial[MOTOR_COUNT] = { 0, 1, 2, 4, 3, 5, 6, 8, 7};
uint8_t opposites[MOTOR_COUNT] = { 0, 5, 2, 8, 1, 7, 4, 6, 3};

Pattern radialRetractAll(radial, MIN_POSITION, MIN_POSITION);
Pattern extendAll(radial, MAX_POSITION, MAX_POSITION);
Pattern radialMax(radial);
Pattern radialShortRetract(radial, MAX_POSITION, 3*MAX_POSITION/4);
Pattern oppositesMax(opposites);
Pattern oppositesShortExtend(opposites, 3*MAX_POSITION/4, MIN_POSITION);
Pattern p[PATTERN_COUNT] = {radialRetractAll, oppositesMax, radialMax};//, radialShortRetract, oppositesMax, extendAll};
EntityCycler<Pattern> patterns(p, PATTERN_COUNT);

static const uint8_t AUDIO_COUNT = 2;
AudioTrack audio1("file1.wav", 1000, 0, 0);
AudioTrack audio2("file2.wav", 1000, 0, 0);
AudioTrack au[AUDIO_COUNT] = {audio1,audio2};
EntityCycler<AudioTrack> tracks(au, AUDIO_COUNT);

void setup() {
  Serial.begin(9600);

  SD.begin();
  audio.speakerPin = 5;
  pinMode(11, OUTPUT);
  audio.loop(1);

  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    m[i].init();
  }
}

bool isSameBoardMotorMoving(uint8_t i){
  uint8_t j = m[i].sameBoardMotorID;
  if (j==255) return false; //motor 0 is alone
  else return m[j].isMoving;
}

uint8_t countMotorsMoving(){
  uint8_t count = 0;
  for (uint8_t i = 0; i < MOTOR_COUNT; i++) if (m[i].isMoving) count = count+1;
  return count;
}

void nextTrack(){
  tracks.getCurrent().saveSeekPosition();
  tracks.next();
  AudioTrack track = tracks.getCurrent();
  audio.play("switch.wav",0,1);
  audio.play(track.fileName, track.seekPosition, 0);
  track.playbackStartedTimestamp = millis();
  Serial.print("Playing track ");
  Serial.println(track.fileName);
}

Pattern& getNextPattern(){
  if ( patterns.getCurrent().reachedEnd() ){ //reached last motor in pattern, advance to next pattern
    if (patterns.reachedEnd()) patterns.next(); //skip first pattern in series because it's the init pattern
    patterns.next();
  }
  patterns.getCurrent().next(); //next motor in array from pattern
  Serial.print("Pattern ");
  Serial.print(patterns.getIndex());
  Serial.print(" on step ");
  Serial.println(patterns.getCurrent().getIndex());

  return patterns.getCurrent();
}

void loop() {

  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {

    uint8_t countMoving = countMotorsMoving();
    bool sameBoardMotorMoving = isSameBoardMotorMoving(i);

    if ((sameBoardMotorMoving || countMoving > MAX_MOTORS_MOVING) && !m[i].isStopped) m[i].stop();
    else if (!sameBoardMotorMoving && countMoving < MAX_MOTORS_MOVING && !m[i].canMove) m[i].allowMove();

    if (!m[i].reachedTarget() && m[i].canMove) m[i].move();
    else if (m[i].reachedTarget() && !m[i].isStopped) {
      
      m[i].stop();

      if (!m[i].completedPattern){

        Pattern p = patterns.getCurrent();
        m[i].setTargetPosition(p.origin);
        m[i].completedPattern = true;

        p = getNextPattern();
        uint8_t j = p.getMotorID();
        m[j].setTargetPosition(p.destination);
        m[j].completedPattern = false;

        nextTrack();
      }
    }
  }
}