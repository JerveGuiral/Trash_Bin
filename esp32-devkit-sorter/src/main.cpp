// =====================================================================
// SMART AUTOMATED WASTE SEPARATOR - ESP32 DevKit (main controller)
//
// IR sensor sees trash on the lid -> asks the ESP32-CAM 'C' -> camera answers
// PLASTIC / BIO / EMPTY / UNKNOWN -> servo tilts the lid to the right side.
// Also serves the CABWMS web dashboard at http://192.168.137.60/ (WiFi in secrets.h).
//
// Wiring:
//   IR sensor OUT  -> GPIO 27   (IR VCC -> 3V3, GND -> GND)
//   Servo orange   -> GPIO 13   (red -> 5V rail, brown -> GND rail)
//   ESP32-CAM IO14 -> GPIO 16   (RX2)
//   ESP32-CAM IO15 -> GPIO 17   (TX2)
//   All GNDs connected together.
//
// Serial monitor 115200 for demo/testing:
//   p = tilt to plastic, b = tilt to bio, c = ask camera now
// =====================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <stdarg.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "secrets.h"
#include "dashboard.h"

// ----- Pins -----
#define IR_PIN      27
#define SERVO_PIN   13
#define CAM_RX      16
#define CAM_TX      17

// ----- Settings to calibrate -----
const int CENTER        = 90;    // lid flat
const int PLASTIC_ANGLE = 45;    // tilt toward Plastic side
const int BIO_ANGLE     = 135;   // tilt toward Biodegradable side
const int TILT_MS       = 1500;  // how long the lid stays tilted so trash slides off
const bool RELEASE_SERVO_WHEN_IDLE = true;  // stop holding the servo when idle (saves a lot of power)
const int STEP_MS       = 12;    // ms per degree: slow moves draw much less current than full-speed jumps
const int ITEM_FILL_PERCENT = 10;  // estimated fill added per sorted item (no level sensor fitted)
const int FULL_THRESHOLD    = 85;  // % that counts as full (matches the dashboard)

// ----- Network (Windows mobile hotspot "lalalala") -----
const IPAddress LOCAL_IP(192, 168, 137, 60), GATEWAY(192, 168, 137, 1), SUBNET(255, 255, 255, 0);
WebServer server(80);

// ----- Servo (LEDC PWM, no library needed) -----
#define SERVO_CH     0
#define SERVO_HZ     50
#define SERVO_BITS   16
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2400

// ----- Live state shown on the dashboard -----
enum SorterState { ST_IDLE, ST_DETECTED, ST_CLASSIFYING, ST_TILTING };
static const char *STATE_NAMES[] = {"idle", "detected", "classifying", "tilting"};
static SorterState state = ST_IDLE;
static String lastItem;               // PLASTIC / BIO / EMPTY / UNKNOWN / TIMEOUT
static uint32_t lastItemAt = 0;       // seconds since boot
static uint32_t lastAiMs = 0;
static int countPlastic = 0, countBio = 0;
static int plasticSinceEmpty = 0, bioSinceEmpty = 0;
static const char *cameraStatus = "waiting";  // waiting / online / no reply
static char pendingAction = 0;        // set by the dashboard, run from loop()

struct Event { uint32_t t; const char *type; char msg[72]; };
#define MAX_EVENTS 20
static Event events[MAX_EVENTS];
static int eventCount = 0, eventHead = 0;

static void logEvent(const char *type, const char *fmt, ...) {
  Event &e = events[eventHead];
  e.t = millis() / 1000;
  e.type = type;
  va_list args;
  va_start(args, fmt);
  vsnprintf(e.msg, sizeof(e.msg), fmt, args);
  va_end(args);
  eventHead = (eventHead + 1) % MAX_EVENTS;
  if (eventCount < MAX_EVENTS) eventCount++;
  Serial.printf("[%s] %s\n", type, e.msg);
}

static int fillPercent(int items) { return min(100, items * ITEM_FILL_PERCENT); }

// Keep the dashboard responsive during long waits (camera ~10 s, servo moves)
static void pump() { server.handleClient(); }
static void waitMs(uint32_t ms) {
  uint32_t start = millis();
  while (millis() - start < ms) { pump(); delay(2); }
}

static void servoWrite(int angle) {
  angle = constrain(angle, 0, 180);
  uint32_t us = map(angle, 0, 180, SERVO_MIN_US, SERVO_MAX_US);
  ledcWrite(SERVO_CH, (uint32_t)((uint64_t)us * ((1 << SERVO_BITS) - 1) / (1000000 / SERVO_HZ)));
}

static void servoRelease() {
  ledcWrite(SERVO_CH, 0);  // no pulses: servo stops drawing holding current
}

// Move one degree at a time so the servo never draws a big current spike
static int servoAngle = CENTER;
static void servoMoveSlow(int target) {
  int step = target > servoAngle ? 1 : -1;
  while (servoAngle != target) {
    servoAngle += step;
    servoWrite(servoAngle);
    waitMs(STEP_MS);
  }
  servoWrite(target);
}

static void tilt(int angle, const char *side) {
  Serial.printf(">>> Tilting to %s (%d deg)\n", side, angle);
  SorterState before = state;
  state = ST_TILTING;
  servoWrite(servoAngle);  // re-engage at the last known position (no jump)
  waitMs(100);
  servoMoveSlow(angle);
  waitMs(TILT_MS);
  servoMoveSlow(CENTER);
  waitMs(500);
  if (RELEASE_SERVO_WHEN_IDLE) servoRelease();
  state = before == ST_TILTING ? ST_IDLE : before;
}

// Ask the ESP32-CAM what the trash is
static String askCamera() {
  while (Serial2.available()) Serial2.read();  // clear old data
  Serial2.print('C');
  Serial.println("Asking camera (takes about 10 seconds)...");
  unsigned long start = millis();
  String r;
  while (millis() - start < 15000) {  // float32 model takes ~10 s per photo
    while (Serial2.available()) {
      char c = Serial2.read();
      if (c == '\n') {
        r.trim();
        if (r == "PLASTIC" || r == "BIO" || r == "EMPTY" || r == "UNKNOWN") return r;
        r = "";
      } else if (c != '\r' && r.length() < 32) {
        r += c;
      }
    }
    pump();
    delay(2);
  }
  return "TIMEOUT";
}

static void checkFull(const char *name, int before, int after) {
  if (before < FULL_THRESHOLD && after >= FULL_THRESHOLD)
    logEvent("alert", "BIN-CBD-001 %s compartment %d%% full - schedule a pickup", name, after);
}

static void sortItem() {
  state = ST_CLASSIFYING;
  uint32_t started = millis();
  String item = askCamera();
  lastAiMs = millis() - started;
  lastItem = item;
  lastItemAt = millis() / 1000;
  Serial.println("Camera says: " + item);

  if (item == "TIMEOUT") {
    cameraStatus = "no reply";
    logEvent("warn", "Camera did not answer - check its wiring and power");
  } else {
    cameraStatus = "online";
  }

  if (item == "PLASTIC") {
    countPlastic++;
    int before = fillPercent(plasticSinceEmpty++);
    logEvent("sorted", "Plastic detected - sorted to the plastic side (%.1f s)", lastAiMs / 1000.0);
    tilt(PLASTIC_ANGLE, "PLASTIC");
    checkFull("plastic", before, fillPercent(plasticSinceEmpty));
  } else if (item == "BIO") {
    countBio++;
    int before = fillPercent(bioSinceEmpty++);
    logEvent("sorted", "Biodegradable detected - sorted to the bio side (%.1f s)", lastAiMs / 1000.0);
    tilt(BIO_ANGLE, "BIODEGRADABLE");
    checkFull("biodegradable", before, fillPercent(bioSinceEmpty));
  } else if (item == "EMPTY") {
    logEvent("info", "Camera saw an empty lid - nothing to sort");
  } else if (item == "UNKNOWN") {
    logEvent("warn", "AI was not sure about the item - lid stays closed");
  }
  state = ST_IDLE;
}

// ----- Web server -----
static void handleStatus() {
  String j;
  j.reserve(2600);
  j += "{\"uptime\":";      j += millis() / 1000;
  j += ",\"state\":\"";     j += STATE_NAMES[state];
  j += "\",\"ir\":";        j += digitalRead(IR_PIN) == LOW ? "true" : "false";
  j += ",\"camera\":\"";    j += cameraStatus;
  j += "\",\"servo\":";     j += servoAngle;
  j += ",\"rssi\":";        j += WiFi.RSSI();
  j += ",\"last\":{\"item\":\""; j += lastItem;
  j += "\",\"at\":";        j += lastItemAt;
  j += ",\"ms\":";          j += lastAiMs;
  j += "},\"counts\":{\"plastic\":"; j += countPlastic;
  j += ",\"bio\":";         j += countBio;
  j += "},\"fill\":{\"plastic\":"; j += fillPercent(plasticSinceEmpty);
  j += ",\"bio\":";         j += fillPercent(bioSinceEmpty);
  j += ",\"source\":\"estimate\"},\"events\":[";
  for (int i = 0; i < eventCount; i++) {  // newest first
    const Event &e = events[(eventHead - 1 - i + MAX_EVENTS) % MAX_EVENTS];
    if (i) j += ',';
    j += "{\"t\":"; j += e.t;
    j += ",\"type\":\""; j += e.type;
    j += "\",\"msg\":\"";
    for (const char *p = e.msg; *p; p++) {
      if (*p == '"' || *p == '\\') j += '\\';
      j += *p;
    }
    j += "\"}";
  }
  j += "]}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

static void handleAction() {
  String a = server.arg("do");
  if (state != ST_IDLE) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"busy\"}");
    return;
  }
  if (a == "classify") pendingAction = 'c';
  else if (a == "plastic") pendingAction = 'p';
  else if (a == "bio") pendingAction = 'b';
  else if (a == "reset") pendingAction = 'r';
  else { server.send(400, "application/json", "{\"ok\":false}"); return; }
  server.send(200, "application/json", "{\"ok\":true}");
}

static void runAction(char a) {
  if (a == 'p') { logEvent("info", "Manual tilt to plastic from the dashboard"); tilt(PLASTIC_ANGLE, "PLASTIC"); }
  else if (a == 'b') { logEvent("info", "Manual tilt to bio from the dashboard"); tilt(BIO_ANGLE, "BIODEGRADABLE"); }
  else if (a == 'c') { logEvent("info", "Manual classify requested from the dashboard"); sortItem(); }
  else if (a == 'r') {
    plasticSinceEmpty = bioSinceEmpty = 0;
    logEvent("info", "BIN-CBD-001 marked as emptied by the collection crew");
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // don't reset on short 5V dips when the servo moves
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, CAM_RX, CAM_TX);
  pinMode(IR_PIN, INPUT_PULLUP);

  // Servo stays limp at boot (no move = no current spike while starting up)
  ledcSetup(SERVO_CH, SERVO_HZ, SERVO_BITS);
  ledcAttachPin(SERVO_PIN, SERVO_CH);
  servoRelease();

  // WiFi connects in the background; sorting works even without it
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.config(LOCAL_IP, GATEWAY, SUBNET, GATEWAY);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  server.on("/", [] { server.send_P(200, "text/html", DASHBOARD_HTML); });
  server.on("/api/status", handleStatus);
  server.on("/api/action", handleAction);
  server.onNotFound([] { server.send(404, "text/plain", "Not found"); });
  server.begin();

  Serial.println();
  Serial.println("=== Smart Waste Separator ready ===");
  Serial.println("Put trash on the lid. Serial commands: p = plastic, b = bio, c = ask camera");
  logEvent("info", "Smart bin powered on");
}

void loop() {
  pump();

  // Report the dashboard address once WiFi connects
  static bool wifiWasUp = false;
  bool wifiUp = WiFi.status() == WL_CONNECTED;
  if (wifiUp && !wifiWasUp) logEvent("info", "Dashboard online at http://%s/", WiFi.localIP().toString().c_str());
  if (!wifiUp && wifiWasUp) logEvent("warn", "WiFi connection lost - reconnecting");
  wifiWasUp = wifiUp;

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'p' || c == 'b' || c == 'c') runAction(c);
  }

  if (pendingAction) {
    char a = pendingAction;
    pendingAction = 0;
    runAction(a);
  }

  if (digitalRead(IR_PIN) == LOW) {  // trash detected on the lid
    state = ST_DETECTED;
    logEvent("info", "Trash detected on the lid");
    waitMs(800);                      // let it settle
    sortItem();

    // Wait until the lid is clear (or 5 s) before checking again
    unsigned long start = millis();
    while (digitalRead(IR_PIN) == LOW && millis() - start < 5000) waitMs(50);
    waitMs(500);
  }
  delay(5);
}
