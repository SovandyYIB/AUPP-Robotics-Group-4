#include <IRremote.hpp>

// IR Receiver Pin
#define IR_RECEIVER_PIN 36

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

// PWM Channel
#define CH_M1 0
#define CH_M2 1
#define CH_M3 2
#define CH_M4 3


// Number buttons
#define IR_1 0xBA45FF00
#define IR_2 0xB946FF00
#define IR_3 0x79275389
#define IR_4 0xBB44FF00
#define IR_5 0xBF40FF00
#define IR_6 0xBC43FF00
#define IR_7 0xF807FF00
#define IR_8 0xEA15FF00
#define IR_9 0xF609FF00
#define IR_0 0xE619FF00
#define IR_STAR 0xE916FF00
#define IR_HASH 0xF20DFF00

// Direction buttons
#define IR_UP    0xE718FF00
#define IR_LEFT  0xF708FF00
#define IR_DOWN  0xAD52FF00
#define IR_RIGHT 0xA55AFF00

// OK button
#define IR_OK 0xE31CFF00

int speedValue = 50; 
String speedInput = "";

int currentCommand = 0;

void setup() {
  Serial.begin(115200);

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

  stopAll();

  IrReceiver.begin(IR_RECEIVER_PIN, ENABLE_LED_FEEDBACK);

  Serial.println("=================================");
  Serial.println("ESP32 IR Remote Robot Control Lab 2");
  Serial.println("Default speed = 50");
  Serial.println("Speed range = 0 to 100");
  Serial.println("UP = Forward");
  Serial.println("DOWN = Backward");
  Serial.println("LEFT = Turn Left");
  Serial.println("RIGHT = Turn Right");
  Serial.println("OK = Stop");
  Serial.println("* = Decrease speed by 5");
  Serial.println("# = Increase speed by 5");
  Serial.println("1-9 = Enter speed digits");
  Serial.println("0 = Confirm numeric speed");
  Serial.println("=================================");
}

void loop() {
  if (IrReceiver.decode()) {
    uint32_t code = IrReceiver.decodedIRData.decodedRawData;

    // Ignore repeat or invalid codes
    if (code != 0xFFFFFFFF && code != 0x00000000) {
      handleIRCode(code);
    }

    IrReceiver.resume();
  }

  runCurrentCommand();

  delay(20);
}


void handleIRCode(uint32_t code) {
  Serial.print("Received IR Code: 0x");
  Serial.println(code, HEX);

  int digit = getDigitFromCode(code);

  if (digit >= 1 && digit <= 9) {
    speedInput += String(digit);

    Serial.print("Speed input: ");
    Serial.println(speedInput);

    return;
  }

  if (code == IR_0) {
    if (speedInput.length() > 0) {
      speedValue = speedInput.toInt();

      speedValue = constrain(speedValue, 0, 100);

      Serial.print("Confirmed speed: ");
      Serial.println(speedValue);

      speedInput = "";
    } else {
      Serial.println("No speed input to confirm.");
    }

    return;
  }

  if (code == IR_STAR) {
    speedValue -= 5;
    speedValue = constrain(speedValue, 0, 100);

    Serial.print("Speed decreased to: ");
    Serial.println(speedValue);

    return;
  }

  if (code == IR_HASH) {
    speedValue += 5;
    speedValue = constrain(speedValue, 0, 100);

    Serial.print("Speed increased to: ");
    Serial.println(speedValue);

    return;
  }

  if (code == IR_OK) {
    currentCommand = 0;
    stopAll();

    Serial.println("Robot stopped");

    return;
  }

  if (code == IR_UP) {
    currentCommand = 1;
    Serial.println("Command: Forward");
    return;
  }

  if (code == IR_DOWN) {
    currentCommand = 2;
    Serial.println("Command: Backward");
    return;
  }

  if (code == IR_LEFT) {
    currentCommand = 3;
    Serial.println("Command: Turn Left");
    return;
  }

  if (code == IR_RIGHT) {
    currentCommand = 4;
    Serial.println("Command: Turn Right");
    return;
  }

  Serial.println("Unknown button. Use this code to update your IR mapping.");
}


int getDigitFromCode(uint32_t code) {
  if (code == IR_1) return 1;
  if (code == IR_2) return 2;
  if (code == IR_3) return 3;
  if (code == IR_4) return 4;
  if (code == IR_5) return 5;
  if (code == IR_6) return 6;
  if (code == IR_7) return 7;
  if (code == IR_8) return 8;
  if (code == IR_9) return 9;

  return -1;
}


void runCurrentCommand() {
  if (currentCommand == 1) {
    forward(speedValue);
  } 
  else if (currentCommand == 2) {
    backward(speedValue);
  } 
  else if (currentCommand == 3) {
    turnLeft(speedValue);
  } 
  else if (currentCommand == 4) {
    turnRight(speedValue);
  } 
  else {
    stopAll();
  }
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

  // Convert lab speed range 0-100 to PWM range 0-255
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