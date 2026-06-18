#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

const char* ssid = "Robotic WIFI";
const char* password = "rbtWIFI@2025";

WebServer server(80);
Servo scannerServo;

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

// Motor 3 = Front Right
#define M3_PWM 5
#define M3_IN1 21
#define M3_IN2 18

// Motor 4 = Rear Right
#define M4_PWM 19
#define M4_IN1 23
#define M4_IN2 22

#define CH_M1 8
#define CH_M2 9
#define CH_M3 10
#define CH_M4 11

#define DEFAULT_SPEED 50
#define OBSTACLE_DISTANCE_CM 20.0
#define AUTO_STOP_MS 180
#define TURN_RIGHT_90_MS 550
#define AUTO_LOOP_MS 70
#define RADAR_STEP_MS 80
#define RADAR_MIN_ANGLE 0
#define RADAR_MAX_ANGLE 180
#define RADAR_STEP_ANGLE 5

enum RobotMode {
  MODE_MANUAL,
  MODE_AUTOMATIC
};

enum AutoState {
  AUTO_FORWARD,
  AUTO_STOPPING,
  AUTO_TURNING
};

enum ManualMove {
  MOVE_STOPPED,
  MOVE_FORWARD,
  MOVE_BACKWARD,
  MOVE_LEFT,
  MOVE_RIGHT
};

RobotMode currentMode = MODE_MANUAL;
AutoState autoState = AUTO_FORWARD;
ManualMove currentManualMove = MOVE_STOPPED;

int speedValue = DEFAULT_SPEED;
int servoAngle = 90;
int radarAngle = 90;
int radarDirection = 1;
float lastDistanceCm = -1;

unsigned long autoStateStartedAt = 0;
unsigned long lastAutoUpdate = 0;
unsigned long lastRadarUpdate = 0;

void setupMotorPins();
void setupRoutes();
void handleRoot();
void handleMoveForward();
void handleMoveBackward();
void handleTurnLeft();
void handleTurnRight();
void handleStop();
void handleSpeed();
void handleServo();
void handleMode();
void handleManualMode();
void handleAutomaticMode();
void handleRadarData();
void setManualMode();
void setAutomaticMode();
void updateAutomaticMode();
void updateRadarScan();
void applyManualMovement();
void sendPlain(String message);
void setServoAngle(int angle);
float readDistanceCm();
void forward(int speed);
void backward(int speed);
void turnLeft(int speed);
void turnRight(int speed);
void stopAll();
void setMotor(int channel, int in1, int in2, int speed, bool isForward);

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  setupMotorPins();

  scannerServo.setPeriodHertz(50);
  scannerServo.attach(SERVO_PIN, 500, 2400);
  setServoAngle(servoAngle);

  stopAll();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  setupRoutes();
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
  updateRadarScan();

  if (currentMode == MODE_AUTOMATIC) {
    updateAutomaticMode();
  }
}

void setupMotorPins() {
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
}

void setupRoutes() {
  server.on("/", handleRoot);
  server.on("/radar", handleRoot);
  server.on("/data", handleRadarData);

  server.on("/forward", handleMoveForward);
  server.on("/backward", handleMoveBackward);
  server.on("/back", handleMoveBackward);
  server.on("/left", handleTurnLeft);
  server.on("/right", handleTurnRight);
  server.on("/stop", handleStop);

  server.on("/speed", handleSpeed);
  server.on("/servo", handleServo);
  server.on("/mode", handleMode);
  server.on("/manual", handleManualMode);
  server.on("/automatic", handleAutomaticMode);
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Robot Radar</title>
  <style>
    body { margin: 0; font-family: Arial, sans-serif; background: #101820; color: #f6f7f9; }
    main { max-width: 820px; margin: 0 auto; padding: 18px; }
    h1 { font-size: 24px; margin: 0 0 14px; }
    .panel { background: #182533; border: 1px solid #2e4155; border-radius: 8px; padding: 14px; margin-bottom: 14px; }
    .row { display: flex; flex-wrap: wrap; gap: 8px; align-items: center; margin: 8px 0; }
    button { min-width: 92px; min-height: 42px; border: 0; border-radius: 6px; background: #2f80ed; color: white; font-weight: 700; }
    button.stop { background: #d64545; }
    button.mode { background: #22a06b; }
    label { min-width: 96px; }
    input[type=range] { flex: 1; min-width: 180px; }
    canvas { width: 100%; max-width: 760px; height: auto; background: #071018; border-radius: 8px; display: block; }
    .value { min-width: 72px; text-align: right; color: #a8d8ff; }
  </style>
</head>
<body>
<main>
  <h1>ESP32 Robot Control and Radar</h1>
  <section class="panel">
    <div class="row">
      <button class="mode" onclick="send('/manual')">Manual</button>
      <button class="mode" onclick="send('/automatic')">Automatic</button>
      <button class="stop" onclick="send('/stop')">Stop</button>
    </div>
    <div class="row">
      <button onclick="send('/forward')">Forward</button>
      <button onclick="send('/backward')">Backward</button>
      <button onclick="send('/left')">Left</button>
      <button onclick="send('/right')">Right</button>
    </div>
    <div class="row">
      <label>Speed</label>
      <input id="speed" type="range" min="0" max="100" value="50" oninput="setSpeed(this.value)">
      <span id="speedText" class="value">50</span>
    </div>
    <div class="row">
      <label>Servo</label>
      <input id="servo" type="range" min="0" max="180" value="90" oninput="setServo(this.value)">
      <span id="servoText" class="value">90</span>
    </div>
  </section>
  <section class="panel">
    <canvas id="radar" width="760" height="410"></canvas>
    <div class="row">
      <span id="status">Waiting for data...</span>
    </div>
  </section>
</main>
<script>
const canvas = document.getElementById('radar');
const ctx = canvas.getContext('2d');
const statusEl = document.getElementById('status');
let points = [];

function send(path) {
  fetch(path).catch(() => {});
}

function setSpeed(value) {
  document.getElementById('speedText').textContent = value;
  send('/speed?value=' + value);
}

function setServo(value) {
  document.getElementById('servoText').textContent = value;
  send('/servo?value=' + value);
}

function drawRadar(angle, distance, mode, speed, servo) {
  const w = canvas.width;
  const h = canvas.height;
  const cx = w / 2;
  const cy = h - 25;
  const radius = 350;

  ctx.clearRect(0, 0, w, h);
  ctx.strokeStyle = '#245c49';
  ctx.lineWidth = 1;

  for (let r = 70; r <= radius; r += 70) {
    ctx.beginPath();
    ctx.arc(cx, cy, r, Math.PI, 2 * Math.PI);
    ctx.stroke();
  }

  for (let a = 0; a <= 180; a += 30) {
    const rad = (180 - a) * Math.PI / 180;
    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.lineTo(cx + Math.cos(rad) * radius, cy - Math.sin(rad) * radius);
    ctx.stroke();
  }

  points = points.filter(p => Date.now() - p.t < 2500);
  if (distance > 0 && distance < 120) {
    points.push({ angle, distance, t: Date.now() });
  }

  for (const p of points) {
    const rad = (180 - p.angle) * Math.PI / 180;
    const rr = Math.min(p.distance, 120) / 120 * radius;
    ctx.fillStyle = '#ff5a5f';
    ctx.beginPath();
    ctx.arc(cx + Math.cos(rad) * rr, cy - Math.sin(rad) * rr, 5, 0, 2 * Math.PI);
    ctx.fill();
  }

  const sweepRad = (180 - angle) * Math.PI / 180;
  ctx.strokeStyle = '#29e68b';
  ctx.lineWidth = 3;
  ctx.beginPath();
  ctx.moveTo(cx, cy);
  ctx.lineTo(cx + Math.cos(sweepRad) * radius, cy - Math.sin(sweepRad) * radius);
  ctx.stroke();

  statusEl.textContent = 'Mode: ' + mode + ' | Speed: ' + speed + ' | Servo: ' + servo +
                         ' deg | Radar: ' + angle + ' deg | Distance: ' +
                         (distance < 0 ? 'No echo' : distance.toFixed(1) + ' cm');
}

async function update() {
  try {
    const response = await fetch('/data');
    const data = await response.json();
    drawRadar(data.angle, data.distance, data.mode, data.speed, data.servo);
  } catch (e) {
    statusEl.textContent = 'Radar data unavailable';
  }
}

setInterval(update, 180);
update();
</script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleMoveForward() {
  if (currentMode != MODE_MANUAL) {
    sendPlain("Ignored: automatic mode is active");
    return;
  }

  currentManualMove = MOVE_FORWARD;
  forward(speedValue);
  sendPlain("forward");
}

void handleMoveBackward() {
  if (currentMode != MODE_MANUAL) {
    sendPlain("Ignored: automatic mode is active");
    return;
  }

  currentManualMove = MOVE_BACKWARD;
  backward(speedValue);
  sendPlain("backward");
}

void handleTurnLeft() {
  if (currentMode != MODE_MANUAL) {
    sendPlain("Ignored: automatic mode is active");
    return;
  }

  currentManualMove = MOVE_LEFT;
  turnLeft(speedValue);
  sendPlain("left");
}

void handleTurnRight() {
  if (currentMode != MODE_MANUAL) {
    sendPlain("Ignored: automatic mode is active");
    return;
  }

  currentManualMove = MOVE_RIGHT;
  turnRight(speedValue);
  sendPlain("right");
}

void handleStop() {
  currentMode = MODE_MANUAL;
  autoState = AUTO_FORWARD;
  currentManualMove = MOVE_STOPPED;
  stopAll();
  sendPlain("stop");
}

void handleSpeed() {
  if (server.hasArg("value")) {
    speedValue = constrain(server.arg("value").toInt(), 0, 100);
  }

  if (currentMode == MODE_MANUAL) {
    applyManualMovement();
  }

  sendPlain(String("speed=") + speedValue);
}

void handleServo() {
  if (server.hasArg("value")) {
    setServoAngle(server.arg("value").toInt());
  }

  sendPlain(String("servo=") + servoAngle);
}

void handleMode() {
  String value = server.arg("value");
  value.toLowerCase();

  if (value == "manual") {
    setManualMode();
  } else if (value == "automatic" || value == "auto") {
    setAutomaticMode();
  }

  sendPlain(currentMode == MODE_MANUAL ? "manual" : "automatic");
}

void handleManualMode() {
  setManualMode();
  sendPlain("manual");
}

void handleAutomaticMode() {
  setAutomaticMode();
  sendPlain("automatic");
}

void handleRadarData() {
  String json = "{";
  json += "\"mode\":\"";
  json += (currentMode == MODE_MANUAL) ? "manual" : "automatic";
  json += "\",\"speed\":";
  json += speedValue;
  json += ",\"servo\":";
  json += servoAngle;
  json += ",\"angle\":";
  json += radarAngle;
  json += ",\"distance\":";
  json += String(lastDistanceCm, 1);
  json += "}";

  server.send(200, "application/json", json);
}

void setManualMode() {
  currentMode = MODE_MANUAL;
  autoState = AUTO_FORWARD;
  currentManualMove = MOVE_STOPPED;
  stopAll();
  setServoAngle(servoAngle);
  Serial.println("Mode: Manual");
}

void setAutomaticMode() {
  currentMode = MODE_AUTOMATIC;
  autoState = AUTO_FORWARD;
  currentManualMove = MOVE_STOPPED;
  autoStateStartedAt = millis();
  lastAutoUpdate = 0;
  Serial.println("Mode: Automatic");
}

void updateAutomaticMode() {
  unsigned long now = millis();

  if (now - lastAutoUpdate < AUTO_LOOP_MS) {
    return;
  }

  lastAutoUpdate = now;

  if (autoState == AUTO_FORWARD) {
    if (lastDistanceCm > 0 && lastDistanceCm < OBSTACLE_DISTANCE_CM) {
      stopAll();
      autoState = AUTO_STOPPING;
      autoStateStartedAt = now;
      Serial.println("Obstacle detected: stopping");
    } else {
      forward(speedValue);
    }
    return;
  }

  if (autoState == AUTO_STOPPING) {
    stopAll();

    if (now - autoStateStartedAt >= AUTO_STOP_MS) {
      turnRight(speedValue);
      autoState = AUTO_TURNING;
      autoStateStartedAt = now;
      Serial.println("Turning right");
    }
    return;
  }

  if (autoState == AUTO_TURNING) {
    turnRight(speedValue);

    if (now - autoStateStartedAt >= TURN_RIGHT_90_MS) {
      autoState = AUTO_FORWARD;
      autoStateStartedAt = now;
      Serial.println("Continuing forward");
    }
  }
}

void updateRadarScan() {
  unsigned long now = millis();

  if (now - lastRadarUpdate < RADAR_STEP_MS) {
    return;
  }

  lastRadarUpdate = now;

  if (currentMode == MODE_AUTOMATIC) {
    radarAngle += radarDirection * RADAR_STEP_ANGLE;

    if (radarAngle >= RADAR_MAX_ANGLE) {
      radarAngle = RADAR_MAX_ANGLE;
      radarDirection = -1;
    } else if (radarAngle <= RADAR_MIN_ANGLE) {
      radarAngle = RADAR_MIN_ANGLE;
      radarDirection = 1;
    }

    scannerServo.write(radarAngle);
  } else {
    radarAngle = servoAngle;
  }

  lastDistanceCm = readDistanceCm();
}

void applyManualMovement() {
  if (currentManualMove == MOVE_FORWARD) {
    forward(speedValue);
  } else if (currentManualMove == MOVE_BACKWARD) {
    backward(speedValue);
  } else if (currentManualMove == MOVE_LEFT) {
    turnLeft(speedValue);
  } else if (currentManualMove == MOVE_RIGHT) {
    turnRight(speedValue);
  } else {
    stopAll();
  }
}

void sendPlain(String message) {
  server.send(200, "text/plain", message);
}

void setServoAngle(int angle) {
  servoAngle = constrain(angle, 0, 180);

  if (currentMode == MODE_MANUAL) {
    radarAngle = servoAngle;
    scannerServo.write(servoAngle);
  }

  Serial.print("Servo angle: ");
  Serial.println(servoAngle);
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
  setMotor(CH_M1, M1_IN1, M1_IN2, speed, true);
  setMotor(CH_M2, M2_IN1, M2_IN2, speed, false);
  setMotor(CH_M3, M3_IN1, M3_IN2, speed, true);
  setMotor(CH_M4, M4_IN1, M4_IN2, speed, true);
}

void backward(int speed) {
  setMotor(CH_M1, M1_IN1, M1_IN2, speed, false);
  setMotor(CH_M2, M2_IN1, M2_IN2, speed, true);
  setMotor(CH_M3, M3_IN1, M3_IN2, speed, false);
  setMotor(CH_M4, M4_IN1, M4_IN2, speed, false);
}

void turnLeft(int speed) {
  setMotor(CH_M1, M1_IN1, M1_IN2, speed, false);
  setMotor(CH_M2, M2_IN1, M2_IN2, speed, true);
  setMotor(CH_M3, M3_IN1, M3_IN2, speed, true);
  setMotor(CH_M4, M4_IN1, M4_IN2, speed, true);
}

void turnRight(int speed) {
  setMotor(CH_M1, M1_IN1, M1_IN2, speed, true);
  setMotor(CH_M2, M2_IN1, M2_IN2, speed, false);
  setMotor(CH_M3, M3_IN1, M3_IN2, speed, false);
  setMotor(CH_M4, M4_IN1, M4_IN2, speed, false);
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
