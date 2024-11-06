//pin1, pin2, motor#, board#, stateCode, last state change timestamp, is allowed to move, completed initial retraction sequence
//statecodes: 2 = fully extended, 1 = extending, -1 = retracting, -2 = fully retracted
unsigned long motors[9][8] = {
  { 36, 34, 4, 0, 0, 0, 0, 0 },  //0 top

  { 5, 4, 2, 0, 0, 0, 0, 0 },    //1 top front
  { 52, 50, 0, 0, 0, 0, 0, 0 },  //2 top left
  { 48, 46, 0, 1, 0, 0, 0, 0 },  //3 top back
  { 3, 2, 2, 1, 0, 0, 0, 0 },    //4 top right

  { 44, 42, 1, 0, 0, 0, 0, 0 },  //5 bottom front right
  { 7, 6, 3, 1, 0, 0, 0, 0 },    //6 bottom front left
  { 9, 8, 3, 0, 0, 0, 0, 0 },    //7 bottom back left
  { 40, 38, 1, 1, 0, 0, 0, 0 }   //8 bottom back right
};

//board conflicts
//1 and 4, 2 and 3, 6 and 7, 5 and 8
//two motors moving at once
int patterns[2][9] = {
  { 0, 5, 2, 8, 1, 7, 4, 6, 3},  //waves
  { 0, 1, 2, 4, 3, 5, 6, 8, 7}  //radials
};

int patternStep = 0;
int patternNumber = 0;
int readyMotors = 0;
bool allMotorsReady = false;
bool movementStarted = false;

#define MOVEMENT_CYCLE_TIME 4000
#define AUDIO_PIN_L 22
#define AUDIO_PIN_G 24
#define AUDIO_PIN_R 26

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 9; i++) {
    pinMode(int(motors[i][0]), OUTPUT);
    pinMode(int(motors[i][1]), OUTPUT);
  }
}

int getBoardNumber(int i) {
  return motors[i][2];
}

int getMotorNumber(int i) {
  return motors[i][3];
}

int getState(int i) {
  return motors[i][4];
}

unsigned long getLastStateChangeTime(int i) {
  return motors[i][5];
}

void setState(int i, int state) {
  motors[i][4] = state;
}

void setIsFullyRetracted(int i) {
  setState(i, -2);
}

void setIsFullyExtended(int i) {
  setState(i, 2);
}

void setIsRetracting(int i) {
  setState(i, -1);
}

void setIsExtending(int i) {
  setState(i, 1);
}

void setCompletedInitialRetraction(int i){
  motors[i][7] = 1;
}

bool isRetracting(int i) {
  return getState(i) == -1;
}

bool isFullyRetracted(int i) {
  return getState(i) == -2;
}

bool isExtending(int i) {
  return getState(i) == 1;
}

bool isFullyExtended(int i) {
  return getState(i) == 2;
}

bool isAllowedToMove(int i) {
  return motors[i][6] == 1;
}

bool isMoving(int i) {
  return getLastStateChangeTime(i) + MOVEMENT_CYCLE_TIME > millis() && (isRetracting(i) || isExtending(i));
}

bool isMovementComplete(int i) {
  return getLastStateChangeTime(i) + MOVEMENT_CYCLE_TIME < millis();
}

bool completedInitialRetraction(int i){
  if (motors[i][7] == 0) return false;
  else return true;
}

//don't fry motor controllers
bool isSameBoardMotorMoving(int i) {

  const int board = getBoardNumber(i);
  const int motor = getMotorNumber(i);

  int otherBoard = 0;
  int otherMotor = 0;

  bool otherMotorMoving = false;

  for (int j = 0; j < 9; j++) {

    otherBoard = getBoardNumber(j);
    otherMotor = getMotorNumber(j);

    if (board == otherBoard && motor != otherMotor) otherMotorMoving = isMoving(j);
  }

  return otherMotorMoving;
}

void setLastStateChangeTime(int i, unsigned long time) {
  motors[i][5] = time;
}

void setIsAllowedToMove(int i, bool isAllowed) {
  if (isAllowed) motors[i][6] = 1;
  else motors[i][6] = 0;
}

void printState(int i) {
  Serial.print("Motor: ");
  Serial.print(i);
  Serial.print(" - Board #:");
  Serial.print(getBoardNumber(i));
  Serial.print(" - Motor #:");
  Serial.print(getMotorNumber(i));
  Serial.print(" - State: ");
  Serial.print(getState(i));
  Serial.print(" - Is allowed to move: ");
  Serial.print(isAllowedToMove(i));
  Serial.print(" - Last state change timestamp: ");
  Serial.print(getLastStateChangeTime(i));
  Serial.print(" - Completed initial retraction: ");
  Serial.print(completedInitialRetraction(i));

  Serial.println();
}

void extend(int i) {
  if (!isSameBoardMotorMoving(i)) {
    setIsExtending(i);
    setLastStateChangeTime(i, millis());

    int pin1 = motors[i][0];
    int pin2 = motors[i][1];

    digitalWrite(pin1, LOW);
    digitalWrite(pin2, HIGH);

    Serial.print("Extending motor ");
    Serial.print(i);
    Serial.println();
  } else {
    int boardNumber = getBoardNumber(i);
    Serial.print("ERROR: Tried to move both motors on board ");
    Serial.print(boardNumber);
    Serial.println();
  }
}

void retract(int i) {
  if (!isSameBoardMotorMoving(i)) {
    setIsRetracting(i);
    setLastStateChangeTime(i, millis());

    int pin1 = motors[i][0];
    int pin2 = motors[i][1];

    digitalWrite(pin1, HIGH);
    digitalWrite(pin2, LOW);

    Serial.print("Retracting motor ");
    Serial.print(i);
    Serial.println();
  } else {
    int boardNumber = getBoardNumber(i);
    Serial.print("ERROR: Tried to move both motors on board ");
    Serial.print(boardNumber);
    Serial.println();
  }
}

void stop(int i) {
  int pin1 = motors[i][0];
  int pin2 = motors[i][1];

  digitalWrite(pin1, LOW);
  digitalWrite(pin2, LOW);

  Serial.print("Stopping motor ");
  Serial.print(i);
  Serial.println();
}

void nextPattern(){
  patternNumber = (patternNumber+1)%2;
  Serial.print("Switching to pattern ");
  Serial.print(patternNumber);
  Serial.println();
}

void nextPatternStep(){
  patternStep = (patternStep + 1) % 9;
}

bool isPatternComplete(){
  return patternStep == 8;
}

void loop() {

  int now = millis();

  for (int i = 0; i < 9; i++) {

    if (!completedInitialRetraction(i)){
      if (!isRetracting(i) && !isFullyRetracted(i) && !isSameBoardMotorMoving(i)) retract(i);
      else if (isRetracting(i) && isMovementComplete(i)){
        setCompletedInitialRetraction(i);
        setIsFullyRetracted(i);
        stop(i);
        setIsAllowedToMove(i, false);
      }
    } else {
      readyMotors = readyMotors + 1;
      allMotorsReady = (readyMotors == 9);
    }

    if (allMotorsReady && !movementStarted){
      Serial.print("Starting movement sequence");
      Serial.println();
      setIsAllowedToMove(patterns[patternNumber][0], true);
      movementStarted = true;
    }

    if (isAllowedToMove(i)) {
      if (isFullyRetracted(i) && !isExtending(i)) extend(i);
      else if (isExtending(i) && isMovementComplete(i) && !isFullyExtended(i)) {
        setIsFullyExtended(i);
        stop(i);
      } else if (isFullyExtended(i) && !isRetracting(i)) {
        retract(i);
        if (isPatternComplete()) nextPattern();
        nextPatternStep();
        setIsAllowedToMove(patterns[patternNumber][patternStep], true);
      } else if (isRetracting(i) && isMovementComplete(i) && !isFullyRetracted(i)) {
        setIsFullyRetracted(i);
        stop(i);
        setIsAllowedToMove(i, false);
      }
    }
  }
}
