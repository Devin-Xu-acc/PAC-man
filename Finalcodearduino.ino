#include <Servo.h>

// ---------- ULTRASONIC SENSORS ----------
#define TRIG_FRONT 2
#define ECHO_FRONT 3

#define TRIG_LEFT 4
#define ECHO_LEFT 5

#define TRIG_RIGHT 6
#define ECHO_RIGHT 7

// ---------- IR SENSORS ----------
#define IR_BOTTOM_LEFT A0
#define IR_BOTTOM_RIGHT A1
#define IR_TOP_LEFT A2
#define IR_TOP_RIGHT A3

// ---------- MOTOR DRIVER ----------
#define ENA A4
#define ENB A5

#define IN1 8
#define IN2 9
#define IN3 10
#define IN4 11

// ---------- SERVOS ----------
#define SERVO1_PIN 12
#define SERVO2_PIN 13

Servo servo1;
Servo servo2;

// ---------- SETTINGS ----------
int ultrasonicDistance = 20;

int motorSpeed = 140;
int creepSpeed = 100;

// If the 180 turn is too much or too little, adjust this
int turnAroundTime = 1400;

int reverseAwayTime = 900;
int faceObjectTurnTime = 220;

int reverseBackTime = 400;
int reverseIntoTrashTime = 900;

// ---------- IR ANALOG CALIBRATION ----------
int bottomLeftThreshold = 500;
int bottomRightThreshold = 500;
int topLeftThreshold = 500;
int topRightThreshold = 500;

// ---------- SERVO POSITIONS ----------
int servo1Open = 180;
int servo2Open = 0;

int servo1Closed = 0;
int servo2Closed = 180;

// ---------- MOTION STATE ----------
enum Motion { STOPPED, FWD, CREEP, REV, TURN_L, TURN_R };
Motion motion = STOPPED;

// ---------- ULTRASONIC FUNCTION ----------
long readDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);

  if (duration == 0) {
    return 999;
  }

  return duration * 0.034 / 2;
}

// ---------- IR FUNCTIONS ----------
int readIRAverage(int pin) {
  long total = 0;

  for (int i = 0; i < 5; i++) {
    total += analogRead(pin);
    delay(2);
  }

  return total / 5;
}

void readAllIR(bool &bottomLeft, bool &bottomRight, bool &topLeft, bool &topRight) {
  int blValue = readIRAverage(IR_BOTTOM_LEFT);
  int brValue = readIRAverage(IR_BOTTOM_RIGHT);
  int tlValue = readIRAverage(IR_TOP_LEFT);
  int trValue = readIRAverage(IR_TOP_RIGHT);

  bottomLeft = blValue < bottomLeftThreshold;
  bottomRight = brValue < bottomRightThreshold;
  topLeft = tlValue < topLeftThreshold;
  topRight = trValue < topRightThreshold;

  Serial.print("IR BL: ");
  Serial.print(blValue);
  Serial.print(" BR: ");
  Serial.print(brValue);
  Serial.print(" TL: ");
  Serial.print(tlValue);
  Serial.print(" TR: ");
  Serial.print(trValue);

  Serial.print(" | Detected BL:");
  Serial.print(bottomLeft);
  Serial.print(" BR:");
  Serial.print(bottomRight);
  Serial.print(" TL:");
  Serial.print(topLeft);
  Serial.print(" TR:");
  Serial.println(topRight);
}

// ---------- MOTOR SPEED ----------
void setSpeed(int speedValue) {
  analogWrite(ENA, speedValue);
  analogWrite(ENB, speedValue);
}

// ---------- MOTOR MOVEMENT ----------
void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  motion = STOPPED;
}

void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  if (motion != FWD) {
    analogWrite(ENA, 200);
    analogWrite(ENB, 200);
    delay(80);
    motion = FWD;
  }

  setSpeed(motorSpeed);
}

void creepForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  if (motion != CREEP) {
    analogWrite(ENA, 170);
    analogWrite(ENB, 170);
    delay(60);
    motion = CREEP;
  }

  setSpeed(creepSpeed);
}

void reverse() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  setSpeed(motorSpeed);
  motion = REV;
}

void left() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  setSpeed(motorSpeed);
  motion = TURN_L;
}

void right() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  setSpeed(motorSpeed);
  motion = TURN_R;
}

// ---------- SERVO FUNCTIONS ----------
void openServos() {
  servo1.write(servo1Open);
  servo2.write(servo2Open);
}

void closeServos() {
  servo1.write(servo1Closed);
  servo2.write(servo2Closed);
}

// ---------- TURN TO FACE OBJECT ----------
void faceObjectFromLeft() {
  left();
  delay(faceObjectTurnTime);

  stopMotors();
  delay(300);
}

void faceObjectFromRight() {
  right();
  delay(faceObjectTurnTime);

  stopMotors();
  delay(300);
}

// ---------- TURN AROUND ----------
void turnAroundOnly() {
  right();
  delay(turnAroundTime);

  stopMotors();
  delay(300);
}

void turnAroundAndSearch() {
  Serial.println("WALL / OBSTACLE DETECTED - TURNING AROUND");

  stopMotors();
  delay(300);

  reverse();
  delay(reverseAwayTime);

  stopMotors();
  delay(300);

  turnAroundOnly();
}

// ---------- TRASH PICKUP ----------
void reverseBackABit() {
  reverse();
  delay(reverseBackTime);

  stopMotors();
  delay(300);
}

void reverseIntoTrash() {
  // Servos should already be open before this starts
  openServos();
  delay(400);

  reverse();
  delay(reverseIntoTrashTime);

  stopMotors();
  delay(300);

  closeServos();
  delay(2000);
}

void handleTrash() {
  Serial.println("TRASH DETECTED");

  // Stop immediately so it does not push the trash forward
  stopMotors();
  delay(300);

  // NEW: reverse back a bit before opening/turning
  reverseBackABit();

  // Open the back after reversing away
  openServos();
  delay(500);

  // Turn 180 so the back faces the trash
  turnAroundOnly();

  // Reverse into the trash and close servos
  reverseIntoTrash();

  // Turn back to face forward again
  turnAroundOnly();

  stopMotors();
  delay(500);
}

// ---------- MOVE CLOSER + DECIDE ----------
void moveCloserThenCheckIR() {
  unsigned long startTime = millis();

  while (millis() - startTime < 2500) {
    bool bottomLeft, bottomRight, topLeft, topRight;
    readAllIR(bottomLeft, bottomRight, topLeft, topRight);

    bool bottomDetected = bottomLeft || bottomRight;
    bool topDetected = topLeft || topRight;

    // Top IR means wall/obstacle
    if (topDetected) {
      stopMotors();
      delay(300);

      turnAroundAndSearch();
      return;
    }

    // Bottom IR only means trash
    // Stop immediately and collect, no extra creeping forward
    if (bottomDetected && !topDetected) {
      stopMotors();
      delay(300);

      handleTrash();
      return;
    }

    creepForward();
    delay(50);
  }

  stopMotors();
  delay(300);

  turnAroundAndSearch();
}

// ---------- SETUP ----------
void setup() {
  pinMode(TRIG_FRONT, OUTPUT);
  pinMode(ECHO_FRONT, INPUT);

  pinMode(TRIG_LEFT, OUTPUT);
  pinMode(ECHO_LEFT, INPUT);

  pinMode(TRIG_RIGHT, OUTPUT);
  pinMode(ECHO_RIGHT, INPUT);

  pinMode(IR_BOTTOM_LEFT, INPUT);
  pinMode(IR_BOTTOM_RIGHT, INPUT);
  pinMode(IR_TOP_LEFT, INPUT);
  pinMode(IR_TOP_RIGHT, INPUT);

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);

  openServos();

  Serial.begin(9600);

  stopMotors();
  delay(500);
}

// ---------- LOOP ----------
void loop() {
  long frontDistance = readDistance(TRIG_FRONT, ECHO_FRONT);
  long leftDistance = readDistance(TRIG_LEFT, ECHO_LEFT);
  long rightDistance = readDistance(TRIG_RIGHT, ECHO_RIGHT);

  bool bottomLeft, bottomRight, topLeft, topRight;
  readAllIR(bottomLeft, bottomRight, topLeft, topRight);

  bool bottomDetected = bottomLeft || bottomRight;
  bool topDetected = topLeft || topRight;

  Serial.print("US Front: ");
  Serial.print(frontDistance);
  Serial.print(" | Left: ");
  Serial.print(leftDistance);
  Serial.print(" | Right: ");
  Serial.println(rightDistance);

  // Top IR sees anything = wall/obstacle
  if (topDetected) {
    turnAroundAndSearch();
  }

  // Bottom only = trash
  else if (bottomDetected) {
    handleTrash();
  }

  // Left ultrasonic sees something
  else if (leftDistance < ultrasonicDistance) {
    stopMotors();
    delay(300);

    faceObjectFromLeft();

    moveCloserThenCheckIR();
  }

  // Right ultrasonic sees something
  else if (rightDistance < ultrasonicDistance) {
    stopMotors();
    delay(300);

    faceObjectFromRight();

    moveCloserThenCheckIR();
  }

  // Front ultrasonic sees something
  else if (frontDistance < ultrasonicDistance) {
    stopMotors();
    delay(300);

    moveCloserThenCheckIR();
  }

  // Nothing detected, roam forward
  else {
    forward();
  }

  delay(50);
}