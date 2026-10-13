// Edge Impulse image collection page, served at /label.
// Pick a label, press Capture: the JPEG downloads as <label>.<timestamp>.jpg,
// so Edge Impulse can infer the label from the filename on upload.
// Uses the CameraWebServer endpoints: /capture, :81/stream and /control (flash).
#include "esp_http_server.h"

static const char LABEL_HTML[] = R"rawliteral(<!doctype html>
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
#rec{width:100%;max-width:480px;margin-top:8px;padding:16px;font-size:18px;font-weight:bold;border:0;border-radius:10px;background:#c62828;color:#fff}
#rec.on{background:#fff;color:#c62828;animation:blink 1s infinite}
@keyframes blink{50%{opacity:.6}}
select{padding:10px;font-size:15px;border-radius:8px;background:#222;color:#eee;border:2px solid #444}
#flash{padding:10px 18px;font-size:15px;border:2px solid #444;border-radius:8px;background:#222;color:#eee}
#flash.on{background:#ffeb3b;color:#111;border-color:#ffeb3b}
#status{min-height:1.4em;color:#aaa;font-size:14px}
.counts{font-size:13px;color:#999}
a{color:#8cf}
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
<button id="rec">&#9679; Record</button>
<div class="row">
  <select id="every">
    <option value="500">every 0.5 s</option>
    <option value="1000" selected>every 1 s</option>
    <option value="2000">every 2 s</option>
  </select>
  <button id="flash">Flash: OFF</button>
</div>
<div id="status"></div>
<p><a href="/">Camera settings (Arduino page)</a></p>
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

async function takeShot() {
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
  return name + ' (' + Math.round(blob.size / 1024) + ' KB)';
}

const capBtn = document.getElementById('cap');
capBtn.onclick = async () => {
  capBtn.disabled = true;
  status.textContent = 'Capturing...';
  try { status.textContent = 'Saved ' + await takeShot(); }
  catch (e) { status.textContent = 'Capture failed: ' + e.message; }
  capBtn.disabled = false;
};

// Record: take one photo every N ms until stopped
const recBtn = document.getElementById('rec');
let recording = false;
recBtn.onclick = async () => {
  if (recording) { recording = false; return; }
  recording = true;
  recBtn.classList.add('on');
  recBtn.innerHTML = '&#9632; Stop';
  capBtn.disabled = true;
  let taken = 0, failed = 0;
  while (recording) {
    const started = Date.now();
    try { await takeShot(); taken++; } catch (e) { failed++; }
    status.textContent = 'Recording ' + label + ': ' + taken + ' photos' + (failed ? ', ' + failed + ' failed' : '');
    const wait = Number(document.getElementById('every').value) - (Date.now() - started);
    if (wait > 0) await new Promise(res => setTimeout(res, wait));
  }
  recBtn.classList.remove('on');
  recBtn.innerHTML = '&#9679; Record';
  capBtn.disabled = false;
  status.textContent = 'Stopped. Recorded ' + taken + ' ' + label + ' photos' + (failed ? ' (' + failed + ' failed)' : '');
};

const flashBtn = document.getElementById('flash');
function showFlash(on){
  flashBtn.classList.toggle('on', on);
  flashBtn.textContent = 'Flash: ' + (on ? 'ON' : 'OFF');
}
fetch('/status').then(r => r.json()).then(s => showFlash(s.led_intensity > 0)).catch(() => {});
flashBtn.onclick = async () => {
  const on = !flashBtn.classList.contains('on');
  try {
    const r = await fetch('/control?var=led_intensity&val=' + (on ? 255 : 0));
    if (!r.ok) throw new Error();
    showFlash(on);
  } catch (e) { status.textContent = 'Flash toggle failed'; }
};
</script>
</body></html>)rawliteral";

esp_err_t label_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, LABEL_HTML, HTTPD_RESP_USE_STRLEN);
}
