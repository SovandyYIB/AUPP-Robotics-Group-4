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

// PWM Channels
#define CH_M1 0
#define CH_M2 1
#define CH_M3 2
#define CH_M4 3

// Button pins
#define BUTTON_UP     16   // Increase forward speed
#define BUTTON_DOWN   15   // Decrease forward speed
#define BUTTON_RIGHT  4    // Increase rotation speed
#define BUTTON_LEFT   2    // Decrease rotation speed

// Joystick pins - Requirement
#define JOY_X 34
#define JOY_Y 35

// Joystick settings
#define JOY_CENTER 2048
#define DEADZONE 600

// Speed settings in percentage
int forwardSpeed = 50;     // 0% - 100%
int rotationSpeed = 50;    // 0% - 100%

#define SPEED_STEP 5
#define MIN_SPEED 0
#define MAX_SPEED 100

// Button previous states
bool lastUpBtn = HIGH;
bool lastDownBtn = HIGH;
bool lastLeftBtn = HIGH;
bool lastRightBtn = HIGH;

void setup() {
  Serial.begin(115200);

  // Motor direction pins
  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);

  pinMode(M2_IN1, OUTPUT);
  pinMode(M2_IN2, OUTPUT);

  pinMode(M3_IN1, OUTPUT);
  pinMode(M3_IN2, OUTPUT);

  pinMode(M4_IN1, OUTPUT);
  pinMode(M4_IN2, OUTPUT);

  // Joystick pins
  pinMode(JOY_X, INPUT);
  pinMode(JOY_Y, INPUT);

  // Button pins
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);

  // PWM setup
  ledcSetup(CH_M1, 20000, 8);
  ledcSetup(CH_M2, 20000, 8);
  ledcSetup(CH_M3, 20000, 8);
  ledcSetup(CH_M4, 20000, 8);

  ledcAttachPin(M1_PWM, CH_M1);
  ledcAttachPin(M2_PWM, CH_M2);
  ledcAttachPin(M3_PWM, CH_M3);
  ledcAttachPin(M4_PWM, CH_M4);

  stopAll();

  Serial.println("ESP32 Robot Control Ready");
  Serial.println("Forward Speed = 50%");
  Serial.println("Rotation Speed = 50%");
}

void loop() {
  // 1. Read and update button speed control
  handleSpeedButtons();

  // 2. Read joystick values
  int xValue = analogRead(JOY_X);
  int yValue = analogRead(JOY_Y);

  // 3. Check joystick direction using dead zone
  bool moveForward  = yValue < JOY_CENTER - DEADZONE;
  bool moveBackward = yValue > JOY_CENTER + DEADZONE;
  bool turnLeftDir  = xValue < JOY_CENTER - DEADZONE;
  bool turnRightDir = xValue > JOY_CENTER + DEADZONE;

  // 4. Decide robot movement
  if (moveForward) {
    // Serial.println("Moving Forward");
    forward(percentToPWM(forwardSpeed));
  }
  else if (moveBackward) {
    // Serial.println("Moving Backward");
    backward(percentToPWM(forwardSpeed));
  }
  else if (turnLeftDir) {
    // Serial.println("Turning Left");
    turnLeft(percentToPWM(rotationSpeed));
  }
  else if (turnRightDir) {
    // Serial.println("Turning Right");
    turnRight(percentToPWM(rotationSpeed));
  }
  else {
    // Serial.println("Stop");
    stopAll();
  }

  delay(50);
}

// Convert percentage speed 0-100 to PWM 0-255

int percentToPWM(int percent) {
  percent = constrain(percent, 0, 100);
  return map(percent, 0, 100, 0, 255);
}

// Button Speed Control

void handleSpeedButtons() {
  bool upBtn = digitalRead(BUTTON_UP);
  bool downBtn = digitalRead(BUTTON_DOWN);
  bool leftBtn = digitalRead(BUTTON_LEFT);
  bool rightBtn = digitalRead(BUTTON_RIGHT);

  // Button is pressed when LOW because INPUT_PULLUP is used

  if (lastUpBtn == HIGH && upBtn == LOW) {
    forwardSpeed += SPEED_STEP;
    forwardSpeed = constrain(forwardSpeed, MIN_SPEED, MAX_SPEED);

    Serial.print("Forward speed increased to: ");
    Serial.print(forwardSpeed);
    Serial.println("%");
  }

  if (lastDownBtn == HIGH && downBtn == LOW) {
    forwardSpeed -= SPEED_STEP;
    forwardSpeed = constrain(forwardSpeed, MIN_SPEED, MAX_SPEED);

    Serial.print("Forward speed decreased to: ");
    Serial.print(forwardSpeed);
    Serial.println("%");
  }

  if (lastRightBtn == HIGH && rightBtn == LOW) {
    rotationSpeed += SPEED_STEP;
    rotationSpeed = constrain(rotationSpeed, MIN_SPEED, MAX_SPEED);

    Serial.print("Rotation speed increased to: ");
    Serial.print(rotationSpeed);
    Serial.println("%");
  }

  if (lastLeftBtn == HIGH && leftBtn == LOW) {
    rotationSpeed -= SPEED_STEP;
    rotationSpeed = constrain(rotationSpeed, MIN_SPEED, MAX_SPEED);

    Serial.print("Rotation speed decreased to: ");
    Serial.print(rotationSpeed);
    Serial.println("%");
  }

  lastUpBtn = upBtn;
  lastDownBtn = downBtn;
  lastLeftBtn = leftBtn;
  lastRightBtn = rightBtn;
}

// Movement Functions

void forward(int speed) {
  // Left side reversed
  setMotor(CH_M1, M1_IN1, M1_IN2, speed, false);  // Front Left
  setMotor(CH_M2, M2_IN1, M2_IN2, speed, false);  // Rear Left

  // Right side normal
  setMotor(CH_M4, M4_IN1, M4_IN2, speed, true);   // Front Right
  setMotor(CH_M3, M3_IN1, M3_IN2, speed, true);   // Rear Right
}

void backward(int speed) {
  // Left side reversed
  setMotor(CH_M1, M1_IN1, M1_IN2, speed, true);   // Front Left
  setMotor(CH_M2, M2_IN1, M2_IN2, speed, true);   // Rear Left

  // Right side normal
  setMotor(CH_M4, M4_IN1, M4_IN2, speed, false);  // Front Right
  setMotor(CH_M3, M3_IN1, M3_IN2, speed, false);  // Rear Right
}

void turnLeft(int speed) {
  // Left side backward
  setMotor(CH_M1, M1_IN1, M1_IN2, speed, true);   // Front Left
  setMotor(CH_M2, M2_IN1, M2_IN2, speed, true);   // Rear Left

  // Right side forward
  setMotor(CH_M4, M4_IN1, M4_IN2, speed, true);   // Front Right
  setMotor(CH_M3, M3_IN1, M3_IN2, speed, true);   // Rear Right
}

void turnRight(int speed) {
  // Left side forward
  setMotor(CH_M1, M1_IN1, M1_IN2, speed, false);  // Front Left
  setMotor(CH_M2, M2_IN1, M2_IN2, speed, false);  // Rear Left

  // Right side backward
  setMotor(CH_M4, M4_IN1, M4_IN2, speed, false);  // Front Right
  setMotor(CH_M3, M3_IN1, M3_IN2, speed, false);  // Rear Right
}

// Motor Helper Functions

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
  speed = constrain(speed, 0, 255);

  if (isForward) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  }

  ledcWrite(channel, speed);
}