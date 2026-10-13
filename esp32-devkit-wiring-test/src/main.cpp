// ESP32 DevKit wiring test for the waste sorter bin.
//   Servo MG996R signal -> GPIO 13 (powered from the 5V rail)
//   IR sensor OUT       -> GPIO 27 (VCC 3V3)
//   ESP32-CAM link      -> GPIO 16 (RX, from CAM IO14), GPIO 17 (TX, to CAM IO15)
// Sends 'C' to the camera every 3 s; the camera answers PLASTIC, BIO, EMPTY or UNKNOWN.
// Serial monitor 115200. Type: s = servo sweep, l = left, c = center, r = right.
#include <Arduino.h>

#define SERVO_PIN   13
#define IR_PIN      27
#define LINK_RX_PIN 16
#define LINK_TX_PIN 17
#define LINK_BAUD   115200

#define SERVO_CH    0
#define SERVO_HZ    50
#define SERVO_BITS  16
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2400

static int servoAngle = 90;
static int irChanges = 0;
static bool irDetected = false;
static unsigned long lastReply = 0;
static String lastReplyText;

static void servoWrite(int angle) {
  angle = constrain(angle, 0, 180);
  servoAngle = angle;
  uint32_t us = map(angle, 0, 180, SERVO_MIN_US, SERVO_MAX_US);
  uint32_t duty = (uint32_t)((uint64_t)us * ((1 << SERVO_BITS) - 1) / (1000000 / SERVO_HZ));
  ledcWrite(SERVO_CH, duty);
  Serial.printf("Servo -> %d deg\n", angle);
}

static void servoSweep() {
  Serial.println("Servo sweep: center, left, center, right, center");
  const int steps[] = {90, 0, 90, 180, 90};
  for (int a : steps) {
    servoWrite(a);
    delay(800);
  }
}

static void readLink() {
  static String line;
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n' || c == '\r') {
      line.trim();
      if (line.length()) {
        lastReply = millis();
        lastReplyText = line;
        Serial.printf("Camera answered: %s\n", line.c_str());
      }
      line = "";
    } else if (line.length() < 64) {
      line += c;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("=== Waste sorter DevKit wiring test ===");
  Serial.println("Servo GPIO13 | IR GPIO27 | CAM link RX16/TX17");

  pinMode(IR_PIN, INPUT_PULLUP);
  irDetected = digitalRead(IR_PIN) == LOW;

  ledcSetup(SERVO_CH, SERVO_HZ, SERVO_BITS);
  ledcAttachPin(SERVO_PIN, SERVO_CH);

  Serial2.begin(LINK_BAUD, SERIAL_8N1, LINK_RX_PIN, LINK_TX_PIN);

  servoSweep();
  Serial.println("Wave your hand in front of the IR sensor - the servo moves when it detects something.");
}

void loop() {
  // Serial commands
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 's') servoSweep();
    else if (c == 'l') servoWrite(0);
    else if (c == 'c') servoWrite(90);
    else if (c == 'r') servoWrite(180);
  }

  // IR sensor (most modules pull OUT low when an object is close)
  bool now = digitalRead(IR_PIN) == LOW;
  if (now != irDetected) {
    delay(20);  // debounce
    now = digitalRead(IR_PIN) == LOW;
    if (now != irDetected) {
      irDetected = now;
      irChanges++;
      Serial.printf("IR: %s\n", irDetected ? "OBJECT DETECTED" : "clear");
      if (irDetected) {
        servoWrite(0);
        delay(600);
        servoWrite(90);
      }
    }
  }

  // Camera link: ask for a classification every 3 s
  static unsigned long lastAsk = 0;
  if (millis() - lastAsk > 3000) {
    lastAsk = millis();
    Serial2.print('C');
  }
  readLink();

  // Summary every 5 s
  static unsigned long lastSummary = 0;
  if (millis() - lastSummary > 5000) {
    lastSummary = millis();
    bool linkOk = lastReply && millis() - lastReply < 8000;
    Serial.printf("[STATUS] Servo: %d deg | IR: %s (%d changes seen) | Camera link: %s%s\n",
                  servoAngle, irDetected ? "object" : "clear", irChanges,
                  linkOk ? "OK, last answer " : "NO REPLY", linkOk ? lastReplyText.c_str() : "");
  }
  delay(5);
}
