// ESP32-CAM (AI Thinker, OV3660): Arduino CameraWebServer + Edge Impulse capture page.
//   http://192.168.137.50/       Arduino camera settings page (stream, Get Still + Save, resolution, flash...)
//   http://192.168.137.50/label  Labelled capture page for Edge Impulse training images
// Based on the arduino-esp32 CameraWebServer example.
#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "ping/ping_sock.h"

#define CAMERA_MODEL_AI_THINKER  // Has PSRAM
#include "camera_pins.h"

#include "secrets.h"  // WIFI_SSID / WIFI_PASS (not uploaded - copy secrets.example.h)

void startCameraServer();
void setupLedFlash(int pin);

static bool cameraReady = false;
static bool serverStarted = false;

// Serial link to the ESP32 DevKit: CAM IO15 (RX) <- DevKit GPIO17 (TX), CAM IO14 (TX) -> DevKit GPIO16 (RX)
#define LINK_RX_PIN 15
#define LINK_TX_PIN 14
#define LINK_BAUD   115200

static void handleLink() {
  static String line;
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n' || c == '\r') {
      line.trim();
      if (line == "PING") {
        Serial1.printf("PONG cam=%s wifi=%s\n", cameraReady ? "ok" : "fail",
                       WiFi.status() == WL_CONNECTED ? "ok" : "down");
        Serial.println("Link: PING from DevKit -> PONG");
      } else if (line.length()) {
        Serial.printf("Link: received \"%s\"\n", line.c_str());
      }
      line = "";
    } else if (line.length() < 64) {
      line += c;
    }
  }
}

// Network watchdog: ping the hotspot (laptop) every 15 s. If the link silently
// freezes, restart. Only armed after the laptop has answered at least once, so a
// firewall that blocks ping can never cause restart loops.
static volatile bool pingOk = false;
static volatile bool pingDone = true;
static bool gatewayAnswered = false;
static int missedPings = 0;

static void onPingSuccess(esp_ping_handle_t h, void *args) { pingOk = true; }
static void onPingEnd(esp_ping_handle_t h, void *args) {
  pingDone = true;
  esp_ping_delete_session(h);
}

static void startGatewayPing() {
  esp_ping_config_t cfg = ESP_PING_DEFAULT_CONFIG();
  ip_addr_t target;
  IPAddress gw = WiFi.gatewayIP();
  IP_ADDR4(&target, gw[0], gw[1], gw[2], gw[3]);
  cfg.target_addr = target;
  cfg.count = 2;
  cfg.timeout_ms = 2000;
  esp_ping_callbacks_t cbs = {};
  cbs.on_ping_success = onPingSuccess;
  cbs.on_ping_end = onPingEnd;
  esp_ping_handle_t h;
  pingOk = false;
  if (esp_ping_new_session(&cfg, &cbs, &h) == ESP_OK) {
    pingDone = false;
    esp_ping_start(h);
  }
}

static void checkNetwork() {
  if (!pingDone) return;  // previous check still running
  static bool checked = false;
  if (checked) {
    if (pingOk) {
      if (!gatewayAnswered) Serial.println("Network watchdog armed (laptop answers ping)");
      gatewayAnswered = true;
      missedPings = 0;
    } else if (gatewayAnswered && ++missedPings >= 4) {
      Serial.println("Laptop unreachable for ~1 minute - restarting to recover WiFi");
      delay(100);
      ESP.restart();
    }
  }
  checked = true;
  startGatewayPing();
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // ignore USB voltage dips

  Serial.begin(115200);
  Serial1.begin(LINK_BAUD, SERIAL_8N1, LINK_RX_PIN, LINK_TX_PIN);
  Serial.println();
  Serial.println("=== ESP32-CAM CameraWebServer + Edge Impulse page ===");

  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  // Allocate buffers for the largest size so resolution can be raised from the settings page
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  // Camera occasionally fails to probe on first power-up; retry before giving up
  esp_err_t err = ESP_FAIL;
  for (int attempt = 1; attempt <= 3; attempt++) {
    err = esp_camera_init(&config);
    if (err == ESP_OK) break;
    Serial.printf("Camera init attempt %d failed: 0x%x\n", attempt, err);
    esp_camera_deinit();
    delay(500);
  }
  if (err != ESP_OK) {
    Serial.println("CAMERA FAIL - check the ribbon cable");
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // OV3660 mounts upside down on the AI Thinker board
    s->set_brightness(s, 1);   // OV3660 is dark by default
    s->set_saturation(s, -2);  // and over-saturated
  }
  s->set_framesize(s, FRAMESIZE_QVGA);  // 320x240 for Edge Impulse
  Serial.println("Camera OK (QVGA 320x240)");

#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  // Fixed address on the Windows mobile hotspot network so the URL never changes
  WiFi.config(IPAddress(192, 168, 137, 50), IPAddress(192, 168, 137, 1),
              IPAddress(255, 255, 255, 0), IPAddress(192, 168, 137, 1));
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to WiFi \"%s\"", WIFI_SSID);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  cameraReady = true;
  // If the hotspot is off, loop() keeps retrying and starts the web server once connected
}

static void startServerOnce() {
  if (serverStarted) return;
  serverStarted = true;
  Serial.printf("WiFi connected, signal %d dBm\n", WiFi.RSSI());
  startCameraServer();
  String ip = WiFi.localIP().toString();
  Serial.printf("Camera settings page: http://%s/\n", ip.c_str());
  Serial.printf("Edge Impulse capture page: http://%s/label\n", ip.c_str());
}

void loop() {
  static unsigned long lastStatus = 0, lastPing = 0;
  handleLink();
  if (!cameraReady) {
    delay(1000);
    return;
  }

  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected) startServerOnce();

  if (millis() - lastStatus > 10000) {
    lastStatus = millis();
    if (connected) {
      Serial.printf("Running - http://%s/label\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.printf("WiFi \"%s\" not available (is the laptop hotspot on?), retrying...\n", WIFI_SSID);
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
  }

  if (connected && millis() - lastPing > 15000) {
    lastPing = millis();
    checkNetwork();
  }
  delay(100);
}
