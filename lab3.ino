#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE

#include <DabbleESP32.h>
#include <ESP32Servo.h>

#define TRIG_PIN 13
#define ECHO_PIN 39

#define SERVO_PIN 17


// Motor 1 = Front Left
#define M1_PWM 33
#define M1_IN1 26
#define M1_IN2 25

// Motor 2 = Rear Left
#define M2_PWM 14
#define M2_IN1 32
#define M2_IN2 27

// Motor 3 = Rear Right
#define M3_PWM 5
#define M3_IN1 21
#define M3_IN2 18

// Motor 4 = Front Right
#define M4_PWM 19
#define M4_IN1 23
#define M4_IN2 22

#define CH_M1 8
#define CH_M2 9
#define CH_M3 10
#define CH_M4 11

#define DEFAULT_SPEED 20
#define OBSTACLE_DISTANCE_CM 25.0
#define STOP_BEFORE_TURN_MS 120
#define TURN_90_MS 550
#define SERVO_MIN_ANGLE 30
#define SERVO_MAX_ANGLE 140
#define SERVO_STEP_ANGLE 10

enum RobotMode {
  MODE_IDLE,
  MODE_MANUAL,
  MODE_AUTOMATIC
};

RobotMode currentMode = MODE_IDLE;

Servo myServo;

int speedValue = DEFAULT_SPEED;
int servoAngle = 90;

bool lastSelectState = false;
bool lastStartState = false;
bool lastCrossState = false;
bool lastTriangleState = false;
bool lastSquareState = false;
bool lastCircleState = false;

bool autoTurning = false;
unsigned long turnStartTime = 0;

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);

  pinMode(M2_IN1, OUTPUT);
  pinMode(M2_IN2, OUTPUT);

  pinMode(M3_IN1, OUTPUT);
  pinMode(M3_IN2, OUTPUT);

  pinMode(M4_IN1, OUTPUT);
  pinMode(M4_IN2, OUTPUT);

  ledcSetup(CH_M1, 20000, 8);
  ledcSetup(CH_M2, 20000, 8);
  ledcSetup(CH_M3, 20000, 8);
  ledcSetup(CH_M4, 20000, 8);

  ledcAttachPin(M1_PWM, CH_M1);
  ledcAttachPin(M2_PWM, CH_M2);
  ledcAttachPin(M3_PWM, CH_M3);
  ledcAttachPin(M4_PWM, CH_M4);

  myServo.attach(SERVO_PIN);
  setServoAngle(servoAngle);

  stopAll();

  Dabble.begin("Lab3_Robot");

  Serial.println("Lab 3 Robot Ready");
  Serial.println("SELECT = Manual Mode");
  Serial.println("START = Automatic Mode");
}

void loop() {
  Dabble.processInput();

  handleModeSelection();

  if (currentMode == MODE_MANUAL) {
    handleManualMode();
  } else if (currentMode == MODE_AUTOMATIC) {
    handleAutomaticMode();
  } else {
    stopAll();
  }

  delay(20);
}


void handleModeSelection() {
  bool selectPressed = GamePad.isSelectPressed();
  bool startPressed = GamePad.isStartPressed();

  if (selectPressed && !lastSelectState) {
    currentMode = MODE_MANUAL;
    autoTurning = false;
    stopAll();
    Serial.println("Mode: Manual");
  }

  if (startPressed && !lastStartState) {
    currentMode = MODE_AUTOMATIC;
    autoTurning = false;
    stopAll();
    Serial.println("Mode: Automatic");
  }

  lastSelectState = selectPressed;
  lastStartState = startPressed;
}


void handleManualMode() {
  handleServoButtons();

  if (GamePad.isUpPressed()) {
    forward(speedValue);
  } else if (GamePad.isDownPressed()) {
    backward(speedValue);
  } else if (GamePad.isLeftPressed()) {
    turnLeft(speedValue);
  } else if (GamePad.isRightPressed()) {
    turnRight(speedValue);
  } else {
    stopAll();
  }
}

void handleServoButtons() {
  bool squarePressed = GamePad.isSquarePressed();
  bool circlePressed = GamePad.isCirclePressed();
  bool crossPressed = GamePad.isCrossPressed();
  bool trianglePressed = GamePad.isTrianglePressed();

  if (squarePressed && !lastSquareState) {
    setServoAngle(servoAngle + SERVO_STEP_ANGLE);
  }

  if (circlePressed && !lastCircleState) {
    setServoAngle(servoAngle - SERVO_STEP_ANGLE);
  }

  if (crossPressed && !lastCrossState) {
    setServoAngle(SERVO_MIN_ANGLE);
  }

  if (trianglePressed && !lastTriangleState) {
    setServoAngle(SERVO_MAX_ANGLE);
  }

  lastSquareState = squarePressed;
  lastCircleState = circlePressed;
  lastCrossState = crossPressed;
  lastTriangleState = trianglePressed;
}

void setServoAngle(int angle) {
  servoAngle = constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
  myServo.write(servoAngle);

  Serial.print("Servo angle: ");
  Serial.println(servoAngle);
}


void handleAutomaticMode() {
  if (autoTurning) {
    turnLeft(speedValue);

    if (millis() - turnStartTime >= TURN_90_MS) {
      autoTurning = false;
      stopAll();
    }

    return;
  }

  float distance = readDistanceCm();

  if (distance < 0) {
    Serial.println("No echo");
    forward(speedValue);
    return;
  }

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance < OBSTACLE_DISTANCE_CM) {
    autoTurning = true;
    stopAll();
    delay(STOP_BEFORE_TURN_MS);
    turnStartTime = millis();
    turnLeft(speedValue);
    Serial.println("Obstacle detected: rotating left");
  } else {
    forward(speedValue);
  }
}

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return -1;
  }

  return (duration * 0.0343) / 2.0;
}


void forward(int speed) {
  // Left side reversed
  setMotor(CH_M1, M1_IN1, M1_IN2, speed, false);
  setMotor(CH_M2, M2_IN1, M2_IN2, speed, false);

  // Right side normal
  setMotor(CH_M4, M4_IN1, M4_IN2, speed, true);
  setMotor(CH_M3, M3_IN1, M3_IN2, speed, true);
}

void backward(int speed) {
  // Left side reversed
  setMotor(CH_M1, M1_IN1, M1_IN2, speed, true);
  setMotor(CH_M2, M2_IN1, M2_IN2, speed, true);

  // Right side normal
  setMotor(CH_M4, M4_IN1, M4_IN2, speed, false);
  setMotor(CH_M3, M3_IN1, M3_IN2, speed, false);
}

void turnLeft(int speed) {
  // Left side backward
  setMotor(CH_M1, M1_IN1, M1_IN2, speed, true);
  setMotor(CH_M2, M2_IN1, M2_IN2, speed, true);

  // Right side forward
  setMotor(CH_M4, M4_IN1, M4_IN2, speed, true);
  setMotor(CH_M3, M3_IN1, M3_IN2, speed, true);
}

void turnRight(int speed) {
  // Left side forward
  setMotor(CH_M1, M1_IN1, M1_IN2, speed, false);
  setMotor(CH_M2, M2_IN1, M2_IN2, speed, false);

  // Right side backward
  setMotor(CH_M4, M4_IN1, M4_IN2, speed, false);
  setMotor(CH_M3, M3_IN1, M3_IN2, speed, false);
}


void stopAll() {
  ledcWrite(CH_M1, 0);
  ledcWrite(CH_M2, 0);
  ledcWrite(CH_M3, 0);
  ledcWrite(CH_M4, 0);

  digitalWrite(M1_IN1, LOW);
  digitalWrite(M1_IN2, LOW);

  digitalWrite(M2_IN1, LOW);
  digitalWrite(M2_IN2, LOW);

  digitalWrite(M3_IN1, LOW);
  digitalWrite(M3_IN2, LOW);

  digitalWrite(M4_IN1, LOW);
  digitalWrite(M4_IN2, LOW);
}

void setMotor(int channel, int in1, int in2, int speed, bool isForward) {
  speed = constrain(speed, 0, 100);

  int pwmValue = map(speed, 0, 100, 0, 255);

  if (speed == 0) {
    ledcWrite(channel, 0);
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    return;
  }

  if (isForward) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  }

  ledcWrite(channel, pwmValue);
}
