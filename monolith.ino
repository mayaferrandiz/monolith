//pin1, pin2, motor#, board#, stateCode, last state change timestamp
//statecodes: 1.0 = extending, 0 = stopped, -1.0 = retracting
unsigned long motors[9][6] = {
  { 36, 34, 4, 0, 0, 0}, //top
  { 5, 4, 2, 0, 0, 0}, //top front
  { 52, 50, 0, 0, 0, 0}, //top left
  { 48, 46, 0, 1, 0, 0}, //top back
  { 3, 2, 2, 1, 0, 0}, //top right
  { 44, 42, 1, 0, 0, 0}, //bottom front right
  { 7, 6, 3, 1, 0, 0}, //bottom front left
  { 9, 8, 3, 0, 0, 0}, //bottom back left
  { 40, 38, 1, 1, 0, 0} //bottom back right
};

#define MOTOR_CYCLE_TIME 4000
#define AUDIO_PIN_L 22
#define AUDIO_PIN_G 24
#define AUDIO_PIN_R 26

void setup() {

  Serial.begin(9600);

  for (int i = 0; i < 9; i++){
    pinMode( int(motors[i][0]), OUTPUT);
    pinMode( int(motors[i][1]), OUTPUT);
    retractMotor(i);
  }
  delay(MOTOR_CYCLE_TIME);
  for (int i = 0; i < 9; i++){
    stopMotor(i);
  }
}

int getBoardNumber(int i){
  return motors[i][2];
}

int getMotorNumber(int i){
  return motors[i][3];
}

int getState(int i){
  return motors[i][4];
}

void setState(int i, int state){
  motors[i][4] = state;
}

unsigned long getLastStateChangeTime(int i){
  return motors[i][5];
}

void setLastStateChangeTime(int i, unsigned long time){
  motors[i][5] = time;
}

void extendMotor(int i) {
  setState(i, 1);
  setLastStateChangeTime(i, millis());

  int pin1 = motors[i][0];
  int pin2 = motors[i][1];

  digitalWrite(pin1, LOW);
  digitalWrite(pin2, HIGH);

  Serial.print("Extending motor ");
  Serial.print(i);
  Serial.println();
}

void retractMotor(int i) {
  setState(i, -1);
  setLastStateChangeTime(i, millis());

  int pin1 = motors[i][0];
  int pin2 = motors[i][1];

  digitalWrite(pin1, HIGH);
  digitalWrite(pin2, LOW);

  Serial.print("Retracting motor ");
  Serial.print(i);
  Serial.println();
}

void stopMotor(int i) {
  setState(i, 0);
  setLastStateChangeTime(i, millis());

  int pin1 = motors[i][0];
  int pin2 = motors[i][1];

  digitalWrite(pin1, LOW);
  digitalWrite(pin2, LOW);

  Serial.print("Stopping motor ");
  Serial.print(i);
  Serial.println();
}

bool cycleComplete(int i) {
  return getLastStateChangeTime(i) + MOTOR_CYCLE_TIME < millis();
}

int i = 0;

void loop() {

  int now = millis();

  if (getState(i) == 0) extendMotor(i);
  else if (getState(i) == 1 && cycleComplete(i)) retractMotor(i);
  else if (getState(i) == -1 && cycleComplete(i)) {
    stopMotor(i);
    i = (i + 1)%9;
    Serial.print("Switching to motor ");
    Serial.print(i);
    Serial.println();
  }
}
