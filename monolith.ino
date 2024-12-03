#include <SPI.h>
#include <SD.h>
#include <TMRpcm.h>
#include <Adafruit_NeoPixel.h>
#define LED_COUNT 2 // Number of LEDs per strip
#define AUDIO_PIN_L 5
#define AUDIO_PIN_G 23
#define AUDIO_PIN_R 11

static const unsigned int MOTOR_COUNT = 9;
static const unsigned int MAX_POSITION = 4500; //how long to extend/retract motors for in milliseconds
static const unsigned int MIN_POSITION = 0;
TMRpcm audio;

const char* concatenate(const char* text, uint8_t number) {
    static char buffer[16]; // Adjust size as needed
    char numChar = '0' + number; // Convert number to character ('0' + 2 = '2')
    snprintf(buffer, sizeof(buffer), "%s %c", text, numChar);
    return buffer;
}

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
  const uint8_t motorNumber;
  const uint8_t boardNumber;
  const uint8_t id;
  Adafruit_NeoPixel* led;
  int position = MAX_POSITION;
  int targetPosition = MIN_POSITION;
  unsigned long lastStateChange = 0;
  uint8_t ledBrightness = 0;
  bool isStopped = false;
  bool isMoving = false;
  bool isWaiting = false;
  bool movedToPatternDestination = true;
  bool printedMoveLogForThisTarget = false;

  bool reachedTarget(){ return position == targetPosition; }
  void setTargetPosition(unsigned int target, bool destination){
    movedToPatternDestination = destination;
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
      if (!isStopped) this->print( concatenate("Waiting", id) );
    }
  }
  void noWait(uint8_t id){
    if (isWaiting){
      lastStateChange = millis();
      isWaiting = false;
      if (!isStopped) this->print( concatenate("Not Waiting", id) );
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
    Serial.print(id);
    Serial.print("] ");
    if (isStopped) Serial.print("🔴 ");
    if (isMoving) Serial.print("🟢 ");
    if (isWaiting) Serial.print("🟡 ");
    Serial.print(eventName);
    Serial.print("; ");
    Serial.print(position);
    Serial.print("->");
    Serial.print(targetPosition);
    Serial.print(" ");
    Serial.println();
  }

  Motor(uint8_t p1, uint8_t p2, uint8_t bNum, uint8_t mNum, uint8_t id, Adafruit_NeoPixel* ledObj) : pin1(p1), pin2(p2), boardNumber(bNum), motorNumber(mNum), id(id), led(ledObj) {}
};

struct Pattern : public EntityCycler<uint8_t> {
    int destination;
    int origin; //will return here after reaching destination

    Pattern(uint8_t* arr, int destination = MAX_POSITION, int origin = MIN_POSITION) : EntityCycler<uint8_t>(arr, MOTOR_COUNT), destination(destination), origin(origin) {}
};

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

Adafruit_NeoPixel led_0(LED_COUNT, 47, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_1(LED_COUNT, 37, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_2(LED_COUNT, 41, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_3(LED_COUNT, 49, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_4(LED_COUNT, 31, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_5(LED_COUNT, 35, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_6(LED_COUNT, 29, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_7(LED_COUNT, 39, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_8(LED_COUNT, 33, NEO_GRB + NEO_KHZ800);

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

static const uint8_t PATTERN_COUNT = 5;
uint8_t radial[MOTOR_COUNT] = { 0, 1, 2, 4, 3, 5, 6, 8, 7};
uint8_t opposites[MOTOR_COUNT] = { 0, 5, 2, 8, 1, 7, 4, 6, 3};
Pattern retractAll(radial, MIN_POSITION, MIN_POSITION);
Pattern extendAll(radial, MAX_POSITION, MAX_POSITION);
Pattern radialMax(radial);
Pattern radialShortRetract(radial, MAX_POSITION, 3*MAX_POSITION/4);
Pattern oppositesMax(opposites);
Pattern oppositesShortExtend(opposites, 3*MAX_POSITION/4, MIN_POSITION);
Pattern p[PATTERN_COUNT] = {retractAll, radialMax, radialShortRetract, oppositesMax, extendAll};
EntityCycler<Pattern> patterns(p, PATTERN_COUNT);

static const uint8_t AUDIO_COUNT = 2;
Audio audio1("file1.wav", 1000, 0, 0);
Audio audio2("file2.wav", 1000, 0, 0);
Audio au[AUDIO_COUNT] = {audio1,audio2};
EntityCycler<Audio> tracks(au, AUDIO_COUNT);

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

void nextTrack(){
  tracks.getCurrent().saveSeekPosition();
  Audio track = tracks.next();
  audio.play("switch.wav",0,1);
  audio.play(track.fileName, track.seekPosition, 0);
  track.playbackStartedTimestamp = millis();
}

void nextPattern(){
  if ( patterns.getCurrent().reachedEnd() ){ //reached last entry in pattern
    if (patterns.reachedEnd()) patterns.next(); //advance twice to skip init pattern
    patterns.next();
  }
  patterns.getCurrent().next(); //next entry in array from pattern
  Serial.print("Pattern ");
  Serial.print(patterns.getIndex());
  Serial.print(" on step ");
  Serial.println(patterns.getCurrent().getIndex());
}

void loop() {

  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    if (isSameBoardMotorMoving(i) && !m[i].isWaiting) m[i].wait( getSameBoardMotorId(i) );
    if (!isSameBoardMotorMoving(i) && m[i].isWaiting) m[i].noWait( getSameBoardMotorId(i) );
    if (!m[i].reachedTarget() && !m[i].isWaiting) m[i].move();
    if (m[i].reachedTarget() && !m[i].isStopped) {
      
      m[i].stop();

      if (m[i].movedToPatternDestination){
        m[i].setTargetPosition( patterns.getCurrent().origin, false);
        nextPattern();
        m[ patterns.getCurrent().getCurrent() ].setTargetPosition( patterns.getCurrent().destination, true );
        nextTrack();
      }
    }
  }
}