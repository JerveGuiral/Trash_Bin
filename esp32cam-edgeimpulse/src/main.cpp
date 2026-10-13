// ESP32-CAM (AI Thinker, OV3660) image collector for Edge Impulse.
// Open http://<ip>/ on a phone connected to the same WiFi.
// Pick a label, press Capture: the JPEG downloads as <label>.<timestamp>.jpg,
// so Edge Impulse can infer the label from the filename on upload.
#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "secrets.h"  // WIFI_SSID / WIFI_PASS (not uploaded - copy secrets.example.h)

// AI Thinker ESP32-CAM pin map
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASH_LED_GPIO     4

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static bool flashOn = false;

static const char INDEX_HTML[] = R"rawliteral(<!doctype html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Waste Bin Camera</title>
<style>
*{box-sizing:border-box}
body{margin:0;padding:12px;background:#111;color:#eee;font-family:sans-serif;text-align:center}
h2{margin:4px 0 10px;font-size:20px}
img{width:100%;max-width:480px;aspect-ratio:4/3;background:#222;border-radius:8px;display:block;margin:0 auto}
.row{display:flex;gap:8px;justify-content:center;flex-wrap:wrap;margin:10px auto;max-width:480px}
.lbl{flex:1;min-width:30%;padding:12px 6px;border:2px solid #444;border-radius:8px;background:#222;color:#eee;font-size:15px}
.lbl.on{border-color:#4caf50;background:#1e3a1f}
#cap{width:100%;max-width:480px;padding:18px;font-size:20px;font-weight:bold;border:0;border-radius:10px;background:#4caf50;color:#fff}
#cap:disabled{background:#555}
#flash{padding:10px 18px;font-size:15px;border:2px solid #444;border-radius:8px;background:#222;color:#eee}
#flash.on{background:#ffeb3b;color:#111;border-color:#ffeb3b}
#status{min-height:1.4em;color:#aaa;font-size:14px}
.counts{font-size:13px;color:#999}
</style></head>
<body>
<h2>Waste Bin Camera</h2>
<img id="view" alt="live preview">
<div class="row">
  <button class="lbl on" data-l="plastic">Plastic <span class="counts" id="c-plastic">0</span></button>
  <button class="lbl" data-l="biodegradable">Biodegradable <span class="counts" id="c-biodegradable">0</span></button>
  <button class="lbl" data-l="background">Empty / background <span class="counts" id="c-background">0</span></button>
</div>
<button id="cap">Capture</button>
<div class="row"><button id="flash">Flash: OFF</button></div>
<div id="status"></div>
<script>
const host = location.hostname;
const view = document.getElementById('view');
const status = document.getElementById('status');
let label = 'plastic';
const counts = {plastic:0, biodegradable:0, background:0};

function startStream(){ view.src = 'http://' + host + ':81/stream?t=' + Date.now(); }
view.onerror = () => setTimeout(startStream, 2000);
startStream();

document.querySelectorAll('.lbl').forEach(b => b.onclick = () => {
  document.querySelectorAll('.lbl').forEach(x => x.classList.remove('on'));
  b.classList.add('on');
  label = b.dataset.l;
});

document.getElementById('cap').onclick = async () => {
  const btn = document.getElementById('cap');
  btn.disabled = true;
  status.textContent = 'Capturing...';
  try {
    const r = await fetch('/capture?t=' + Date.now());
    if (!r.ok) throw new Error('HTTP ' + r.status);
    const blob = await r.blob();
    const name = label + '.' + Date.now() + '.jpg';
    const a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = name;
    document.body.appendChild(a); a.click(); a.remove();
    setTimeout(() => URL.revokeObjectURL(a.href), 5000);
    counts[label]++;
    document.getElementById('c-' + label).textContent = counts[label];
    status.textContent = 'Saved ' + name + ' (' + Math.round(blob.size / 1024) + ' KB)';
  } catch (e) {
    status.textContent = 'Capture failed: ' + e.message;
  }
  btn.disabled = false;
};

const flashBtn = document.getElementById('flash');
flashBtn.onclick = async () => {
  const on = !flashBtn.classList.contains('on');
  try {
    const r = await fetch('/flash?on=' + (on ? 1 : 0));
    const state = (await r.text()).trim() === '1';
    flashBtn.classList.toggle('on', state);
    flashBtn.textContent = 'Flash: ' + (state ? 'ON' : 'OFF');
  } catch (e) { status.textContent = 'Flash toggle failed'; }
};
</script>
</body></html>)rawliteral";

static esp_err_t indexHandler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t captureHandler(httpd_req_t *req) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return res;
}

static esp_err_t flashHandler(httpd_req_t *req) {
  char query[16], val[4];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
      httpd_query_key_value(query, "on", val, sizeof(val)) == ESP_OK) {
    flashOn = val[0] == '1';
    digitalWrite(FLASH_LED_GPIO, flashOn ? HIGH : LOW);
  }
  httpd_resp_set_type(req, "text/plain");
  return httpd_resp_send(req, flashOn ? "1" : "0", 1);
}

static esp_err_t streamHandler(httpd_req_t *req) {
  char part[64];
  esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Frame capture failed");
      res = ESP_FAIL;
      break;
    }
    size_t hlen = snprintf(part, sizeof(part), STREAM_PART, fb->len);
    res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, part, hlen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    if (res != ESP_OK) break;  // client disconnected
  }
  return res;
}

static void startServers() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t webServer = NULL, streamServer = NULL;

  httpd_uri_t indexUri = {"/", HTTP_GET, indexHandler, NULL};
  httpd_uri_t captureUri = {"/capture", HTTP_GET, captureHandler, NULL};
  httpd_uri_t flashUri = {"/flash", HTTP_GET, flashHandler, NULL};
  if (httpd_start(&webServer, &config) == ESP_OK) {
    httpd_register_uri_handler(webServer, &indexUri);
    httpd_register_uri_handler(webServer, &captureUri);
    httpd_register_uri_handler(webServer, &flashUri);
  }

  // Stream runs on its own server so Capture and Flash stay responsive
  config.server_port = 81;
  config.ctrl_port += 1;
  httpd_uri_t streamUri = {"/stream", HTTP_GET, streamHandler, NULL};
  if (httpd_start(&streamServer, &config) == ESP_OK) {
    httpd_register_uri_handler(streamServer, &streamUri);
  }
}

static bool initCamera() {
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
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.frame_size = FRAMESIZE_QVGA;  // 320x240
  config.jpeg_quality = 10;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;

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
    return false;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);       // OV3660 mounts upside down on the AI Thinker board
    s->set_brightness(s, 1);  // OV3660 is dark by default
    s->set_saturation(s, -2); // and over-saturated
  }
  s->set_framesize(s, FRAMESIZE_QVGA);
  return true;
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // ignore USB voltage dips

  pinMode(FLASH_LED_GPIO, OUTPUT);
  digitalWrite(FLASH_LED_GPIO, LOW);

  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== ESP32-CAM Edge Impulse image collector ===");

  if (!initCamera()) return;
  Serial.println("Camera OK (QVGA 320x240)");

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  // Fixed address on the Windows mobile hotspot network so the URL never changes
  WiFi.config(IPAddress(192, 168, 137, 50), IPAddress(192, 168, 137, 1),
              IPAddress(255, 255, 255, 0), IPAddress(192, 168, 137, 1));
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to WiFi \"%s\"", WIFI_SSID);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 30000) {
      Serial.println("\nWiFi FAIL: could not connect. Check name/password and that the hotspot is 2.4 GHz.");
      return;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.printf("WiFi connected, signal %d dBm\n", WiFi.RSSI());

  startServers();
  Serial.printf("OPEN IN BROWSER: http://%s/\n", WiFi.localIP().toString().c_str());
}

void loop() {
  static unsigned long last = 0;
  if (millis() - last > 10000) {
    last = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("Running - http://%s/\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.println("WiFi disconnected, reconnecting...");
      WiFi.reconnect();
    }
  }
  delay(100);
}
