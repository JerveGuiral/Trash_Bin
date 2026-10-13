// =====================================================================
// SMART AUTOMATED WASTE SEPARATOR - ESP32 DevKit (main controller)
//
// IR sensor sees trash on the lid -> asks the ESP32-CAM 'C' -> camera answers
// PLASTIC / BIO / EMPTY / UNKNOWN -> servo tilts the lid to the right side.
// Also serves the CABWMS web dashboard at http://192.168.137.60/ (WiFi in secrets.h):
// dashboard, map, analytics, history (CSV export), settings (servo calibration).
// Sorting history, totals and settings are saved in flash (NVS) and survive restarts.
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
#include <Preferences.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "secrets.h"
#include "dashboard.h"

// ----- Pins -----
#define IR_PIN      27
#define SERVO_PIN   13
#define CAM_RX      16
#define CAM_TX      17

// ----- Fixed settings -----
const int CENTER = 90;                      // lid flat
const bool RELEASE_SERVO_WHEN_IDLE = true;  // stop holding the servo when idle (saves a lot of power)
const int STEP_MS = 12;                     // ms per degree: slow moves draw much less current

// ----- Settings changeable from the dashboard (saved in flash) -----
struct Settings {
  uint8_t plasticAngle = 45;    // tilt toward Plastic side
  uint8_t bioAngle = 135;       // tilt toward Biodegradable side
  uint16_t tiltMs = 1500;       // how long the lid stays tilted so trash slides off
  uint8_t fillPerItem = 10;     // estimated fill added per sorted item (no level sensor fitted)
  uint8_t fullThreshold = 85;   // % that counts as full
} cfg;

// ----- Network (Windows mobile hotspot "lalalala") -----
const IPAddress LOCAL_IP(192, 168, 137, 60), GATEWAY(192, 168, 137, 1), SUBNET(255, 255, 255, 0);
WebServer server(80);
Preferences prefs;

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
static uint32_t lastRecId = 0;        // history record of the last classification (for feedback)
static int plasticSinceEmpty = 0, bioSinceEmpty = 0;
static const char *cameraStatus = "waiting";  // waiting / online / no reply
static char pendingAction = 0;        // set by the dashboard, run from loop()

// ----- Sorting history (saved in flash) -----
// result: P plastic, B bio, E empty lid, U unsure, T camera timeout, R bin emptied (pickup)
// feedback: 0 none, 1 correct, 2 actually plastic, 3 actually bio, 4 actually nothing
struct Rec { uint32_t id, epoch, uptime; uint16_t aiMs, boot; char result; uint8_t feedback; uint8_t pad[2]; };
#define MAX_RECS 250
static Rec recs[MAX_RECS];
static uint16_t recCount = 0, bootCount = 0;
static uint32_t nextId = 1;
struct Totals { uint32_t plastic, bio, empty, unknown, timeout, pickups; } totals = {};
static const char *timeSource = "none";  // none / ntp / browser

// ----- Event log for the System alerts list (RAM only) -----
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

// ----- Time (NTP through the hotspot, or synced from the dashboard browser) -----
static bool timeKnown() { return time(nullptr) > 1700000000; }
static uint32_t nowEpoch() { return timeKnown() ? (uint32_t)time(nullptr) : 0; }

// ----- Persistence -----
static void saveHistory() {
  prefs.putBytes("recs", recs, recCount * sizeof(Rec));
  prefs.putUShort("rc", recCount);
  prefs.putUInt("nid", nextId);
  prefs.putBytes("tot", &totals, sizeof(totals));
  prefs.putInt("pse", plasticSinceEmpty);
  prefs.putInt("bse", bioSinceEmpty);
}

static void loadAll() {
  prefs.begin("cabwms", false);
  bootCount = prefs.getUShort("boot", 0) + 1;
  prefs.putUShort("boot", bootCount);
  if (prefs.getBytesLength("cfg") == sizeof(cfg)) prefs.getBytes("cfg", &cfg, sizeof(cfg));
  if (prefs.getBytesLength("tot") == sizeof(totals)) prefs.getBytes("tot", &totals, sizeof(totals));
  uint16_t rc = prefs.getUShort("rc", 0);
  if (rc <= MAX_RECS && prefs.getBytesLength("recs") == rc * sizeof(Rec)) {
    prefs.getBytes("recs", recs, rc * sizeof(Rec));
    recCount = rc;
  }
  nextId = prefs.getUInt("nid", 1);
  plasticSinceEmpty = prefs.getInt("pse", 0);
  bioSinceEmpty = prefs.getInt("bse", 0);
}

static uint32_t addRecord(char result, uint16_t aiMs) {
  if (recCount == MAX_RECS) {  // drop the oldest
    memmove(recs, recs + 1, (MAX_RECS - 1) * sizeof(Rec));
    recCount--;
  }
  Rec &r = recs[recCount++];
  memset(&r, 0, sizeof(r));
  r.id = nextId++;
  r.epoch = nowEpoch();
  r.uptime = millis() / 1000;
  r.aiMs = aiMs;
  r.boot = bootCount;
  r.result = result;
  saveHistory();
  return r.id;
}

// Records made before the clock was known get their real time once it is known
static void backfillTimes() {
  uint32_t now = nowEpoch(), up = millis() / 1000;
  bool changed = false;
  for (int i = 0; i < recCount; i++) {
    if (recs[i].epoch == 0 && recs[i].boot == bootCount) {
      recs[i].epoch = now - (up - recs[i].uptime);
      changed = true;
    }
  }
  if (changed) saveHistory();
}

static int fillPercent(int items) { return min(100, items * (int)cfg.fillPerItem); }

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
  waitMs(cfg.tiltMs);
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
  if (before < cfg.fullThreshold && after >= cfg.fullThreshold)
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
  uint16_t aiMs = min(lastAiMs, (uint32_t)65535);

  if (item == "TIMEOUT") {
    cameraStatus = "no reply";
    totals.timeout++;
    lastRecId = addRecord('T', aiMs);
    logEvent("warn", "Camera did not answer - check its wiring and power");
  } else {
    cameraStatus = "online";
  }

  if (item == "PLASTIC") {
    totals.plastic++;
    int before = fillPercent(plasticSinceEmpty++);
    lastRecId = addRecord('P', aiMs);
    logEvent("sorted", "Plastic detected - sorted to the plastic side (%.1f s)", lastAiMs / 1000.0);
    tilt(cfg.plasticAngle, "PLASTIC");
    checkFull("plastic", before, fillPercent(plasticSinceEmpty));
  } else if (item == "BIO") {
    totals.bio++;
    int before = fillPercent(bioSinceEmpty++);
    lastRecId = addRecord('B', aiMs);
    logEvent("sorted", "Biodegradable detected - sorted to the bio side (%.1f s)", lastAiMs / 1000.0);
    tilt(cfg.bioAngle, "BIODEGRADABLE");
    checkFull("biodegradable", before, fillPercent(bioSinceEmpty));
  } else if (item == "EMPTY") {
    totals.empty++;
    lastRecId = addRecord('E', aiMs);
    logEvent("info", "Camera saw an empty lid - nothing to sort");
  } else if (item == "UNKNOWN") {
    totals.unknown++;
    lastRecId = addRecord('U', aiMs);
    logEvent("warn", "AI was not sure about the item - lid stays closed");
  }
  state = ST_IDLE;
}

// ----- Web server -----
static void sendJson(const String &j) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

static uint8_t feedbackFor(uint32_t id) {
  for (int i = recCount - 1; i >= 0; i--) if (recs[i].id == id) return recs[i].feedback;
  return 0;
}

static void handleStatus() {
  String j;
  j.reserve(3200);
  j += "{\"uptime\":";      j += millis() / 1000;
  j += ",\"boot\":";        j += bootCount;
  j += ",\"state\":\"";     j += STATE_NAMES[state];
  j += "\",\"ir\":";        j += digitalRead(IR_PIN) == LOW ? "true" : "false";
  j += ",\"camera\":\"";    j += cameraStatus;
  j += "\",\"servo\":";     j += servoAngle;
  j += ",\"rssi\":";        j += WiFi.RSSI();
  j += ",\"ip\":\"";        j += WiFi.localIP().toString();
  j += "\",\"heap\":";      j += ESP.getFreeHeap();
  j += ",\"build\":\"" __DATE__ " " __TIME__ "\"";
  j += ",\"time\":{\"known\":"; j += timeKnown() ? "true" : "false";
  j += ",\"epoch\":";       j += nowEpoch();
  j += ",\"source\":\"";    j += timeSource;
  j += "\"},\"last\":{\"item\":\""; j += lastItem;
  j += "\",\"at\":";        j += lastItemAt;
  j += ",\"ms\":";          j += lastAiMs;
  j += ",\"id\":";          j += lastRecId;
  j += ",\"feedback\":";    j += feedbackFor(lastRecId);
  j += "},\"counts\":{\"plastic\":"; j += totals.plastic;
  j += ",\"bio\":";         j += totals.bio;
  j += "},\"totals\":{\"plastic\":"; j += totals.plastic;
  j += ",\"bio\":";         j += totals.bio;
  j += ",\"empty\":";       j += totals.empty;
  j += ",\"unknown\":";     j += totals.unknown;
  j += ",\"timeout\":";     j += totals.timeout;
  j += ",\"pickups\":";     j += totals.pickups;
  j += "},\"records\":";    j += recCount;
  j += ",\"fill\":{\"plastic\":"; j += fillPercent(plasticSinceEmpty);
  j += ",\"bio\":";         j += fillPercent(bioSinceEmpty);
  j += ",\"source\":\"estimate\"},\"settings\":{\"plasticAngle\":"; j += cfg.plasticAngle;
  j += ",\"bioAngle\":";    j += cfg.bioAngle;
  j += ",\"tiltMs\":";      j += cfg.tiltMs;
  j += ",\"fillPerItem\":"; j += cfg.fillPerItem;
  j += ",\"fullThreshold\":"; j += cfg.fullThreshold;
  j += "},\"events\":[";
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
  sendJson(j);
}

// Compact history: [id, epoch, uptime, aiMs, boot, "result", feedback]
static void handleHistory() {
  String j;
  j.reserve(recCount * 44 + 64);
  j += "{\"boot\":"; j += bootCount;
  j += ",\"uptime\":"; j += millis() / 1000;
  j += ",\"recs\":[";
  for (int i = 0; i < recCount; i++) {
    const Rec &r = recs[i];
    if (i) j += ',';
    j += '['; j += r.id; j += ','; j += r.epoch; j += ','; j += r.uptime; j += ',';
    j += r.aiMs; j += ','; j += r.boot; j += ",\""; j += r.result; j += "\","; j += r.feedback; j += ']';
  }
  j += "]}";
  sendJson(j);
}

static void handleFeedback() {
  uint32_t id = server.arg("id").toInt();
  String v = server.arg("v");
  uint8_t fb = v == "correct" ? 1 : v == "plastic" ? 2 : v == "bio" ? 3 : v == "nothing" ? 4 : v == "clear" ? 0 : 255;
  for (int i = recCount - 1; i >= 0 && fb != 255; i--) {
    if (recs[i].id == id) {
      recs[i].feedback = fb;
      saveHistory();
      sendJson("{\"ok\":true}");
      return;
    }
  }
  server.send(400, "application/json", "{\"ok\":false}");
}

static void handleTime() {
  uint32_t epoch = server.arg("epoch").toInt();
  if (epoch > 1700000000 && (!timeKnown() || abs((long)(epoch - nowEpoch())) > 120)) {
    struct timeval tv = {(time_t)epoch, 0};
    settimeofday(&tv, nullptr);
    timeSource = "browser";
    backfillTimes();
  }
  sendJson("{\"ok\":true}");
}

static void handleSettings() {
  if (server.hasArg("plasticAngle")) {
    Settings n = cfg;
    n.plasticAngle = constrain(server.arg("plasticAngle").toInt(), 0, 180);
    n.bioAngle = constrain(server.arg("bioAngle").toInt(), 0, 180);
    n.tiltMs = constrain(server.arg("tiltMs").toInt(), 300, 5000);
    n.fillPerItem = constrain(server.arg("fillPerItem").toInt(), 1, 50);
    n.fullThreshold = constrain(server.arg("fullThreshold").toInt(), 50, 100);
    cfg = n;
    prefs.putBytes("cfg", &cfg, sizeof(cfg));
    logEvent("info", "Settings saved: plastic %d deg, bio %d deg, tilt %d ms",
             cfg.plasticAngle, cfg.bioAngle, cfg.tiltMs);
  }
  sendJson("{\"ok\":true}");
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
  else if (a == "clearstats") pendingAction = 'x';
  else { server.send(400, "application/json", "{\"ok\":false}"); return; }
  sendJson("{\"ok\":true}");
}

static void runAction(char a) {
  if (a == 'p') { logEvent("info", "Manual tilt to plastic from the dashboard"); tilt(cfg.plasticAngle, "PLASTIC"); }
  else if (a == 'b') { logEvent("info", "Manual tilt to bio from the dashboard"); tilt(cfg.bioAngle, "BIODEGRADABLE"); }
  else if (a == 'c') { logEvent("info", "Manual classify requested from the dashboard"); sortItem(); }
  else if (a == 'r') {
    plasticSinceEmpty = bioSinceEmpty = 0;
    totals.pickups++;
    addRecord('R', 0);
    logEvent("info", "BIN-CBD-001 marked as emptied by the collection crew");
  } else if (a == 'x') {
    recCount = 0;
    totals = {};
    plasticSinceEmpty = bioSinceEmpty = 0;
    lastRecId = 0;
    saveHistory();
    logEvent("info", "Statistics and history cleared from the dashboard");
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // don't reset on short 5V dips when the servo moves
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, CAM_RX, CAM_TX);
  pinMode(IR_PIN, INPUT_PULLUP);
  loadAll();

  // Servo stays limp at boot (no move = no current spike while starting up)
  ledcSetup(SERVO_CH, SERVO_HZ, SERVO_BITS);
  ledcAttachPin(SERVO_PIN, SERVO_CH);
  servoRelease();

  // WiFi connects in the background; sorting works even without it
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.config(LOCAL_IP, GATEWAY, SUBNET, GATEWAY);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  configTime(0, 0, "pool.ntp.org", "time.google.com");  // works when the hotspot shares internet

  server.on("/", [] { server.send_P(200, "text/html", DASHBOARD_HTML); });
  server.on("/api/status", handleStatus);
  server.on("/api/history", handleHistory);
  server.on("/api/feedback", handleFeedback);
  server.on("/api/time", handleTime);
  server.on("/api/settings", handleSettings);
  server.on("/api/action", handleAction);
  server.onNotFound([] { server.send(404, "text/plain", "Not found"); });
  server.begin();

  Serial.println();
  Serial.println("=== Smart Waste Separator ready ===");
  Serial.println("Put trash on the lid. Serial commands: p = plastic, b = bio, c = ask camera");
  logEvent("info", "Smart bin powered on (%d records in history)", recCount);
}

void loop() {
  pump();

  // Report the dashboard address once WiFi connects
  static bool wifiWasUp = false;
  bool wifiUp = WiFi.status() == WL_CONNECTED;
  if (wifiUp && !wifiWasUp) logEvent("info", "Dashboard online at http://%s/", WiFi.localIP().toString().c_str());
  if (!wifiUp && wifiWasUp) logEvent("warn", "WiFi connection lost - reconnecting");
  wifiWasUp = wifiUp;

  // Clock became known through NTP
  static bool clockWasKnown = false;
  if (timeKnown() && !clockWasKnown) {
    if (strcmp(timeSource, "none") == 0) timeSource = "ntp";
    backfillTimes();
  }
  clockWasKnown = timeKnown();

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
