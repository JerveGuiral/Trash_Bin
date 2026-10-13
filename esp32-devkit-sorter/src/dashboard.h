// CABWMS web dashboard, served by the DevKit at "/".
// Views: Main dashboard, Interactive map, Analytics, History, Settings (hash routing).
// Live data: /api/status (1.5 s), /api/history; actions: /api/action, /api/feedback, /api/settings, /api/time.
// Only BIN-CBD-001 is the real prototype bin - the other 11 map bins are sample data for the wireframe.
#pragma once

static const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>CABWMS Dashboard</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>
:root{--navy:#0d1b2e;--bg:#f3f5f8;--card:#fff;--line:#e3e7ee;--text:#1b2433;--muted:#6b7686;
--brand:#1e8a4c;--red:#d93f3f;--orange:#e98a1f;--ok:#2f9e5a;--soft:#eef1f5;
--s-plastic:#2a78d6;--s-bio:#1baf7a;--s-unsure:#898781;--grid:#e1e0d9;--axis:#c3c2b7;--axislbl:#898781}
*{box-sizing:border-box}html,body{margin:0;height:100%}
body{font-family:Inter,system-ui,-apple-system,Segoe UI,Roboto,sans-serif;background:var(--bg);color:var(--text);font-size:14px}
a{color:inherit;text-decoration:none}
.top{height:56px;background:var(--navy);color:#fff;display:flex;align-items:center;gap:12px;padding:0 18px;position:sticky;top:0;z-index:1000}
.logo{width:30px;height:30px;border-radius:7px;background:var(--brand);display:grid;place-items:center}
.brand{font-weight:700;letter-spacing:.3px}.brand span{font-weight:400;opacity:.85}
.top .sp{flex:1}
.bell{position:relative;width:34px;height:34px;display:grid;place-items:center;border-radius:8px}
.bell b{position:absolute;top:2px;right:2px;background:var(--red);color:#fff;font-size:10px;min-width:16px;height:16px;border-radius:8px;display:grid;place-items:center;padding:0 4px}
.user{text-align:right;line-height:1.2;font-size:12px}.user small{opacity:.7}
.av{width:32px;height:32px;border-radius:50%;background:#2c3e5c;display:grid;place-items:center;font-weight:600;font-size:12px}
.shell{display:flex;min-height:calc(100% - 56px)}
.side{width:210px;background:#fff;border-right:1px solid var(--line);padding:14px 10px;display:flex;flex-direction:column;gap:4px;position:sticky;top:56px;height:calc(100vh - 56px)}
.nav{display:flex;align-items:center;gap:10px;padding:9px 10px;border-radius:8px;color:var(--muted);font-weight:500}
.nav.on{background:var(--soft);color:var(--text)}.nav:hover{background:var(--soft)}
.side .sp{flex:1}
.cw{background:var(--soft);border:1px solid var(--line);border-radius:10px;padding:12px;font-size:12px;color:var(--muted);line-height:1.5}
.cw b{display:block;color:var(--text);margin-bottom:4px}
main{flex:1;padding:22px 26px;min-width:0}
.head{display:flex;align-items:flex-start;justify-content:space-between;gap:16px;margin-bottom:18px;flex-wrap:wrap}
h1{font-size:22px;margin:0 0 4px}.sub{margin:0;color:var(--muted)}
h2{font-size:15px;margin:0 0 2px}
.btn{display:inline-flex;align-items:center;justify-content:center;gap:8px;border:1px solid var(--line);background:#fff;border-radius:8px;padding:9px 14px;font:inherit;font-weight:600;cursor:pointer;color:var(--text)}
.btn.dark{background:var(--navy);color:#fff;border-color:var(--navy)}
.btn.sm{padding:5px 10px;font-size:12px;font-weight:500}
.btn.danger{color:var(--red);border-color:#f0c4c4}
.btn:disabled{opacity:.45;cursor:not-allowed}
.cols{display:grid;grid-template-columns:minmax(0,1.15fr) minmax(0,1fr);gap:18px;align-items:start}
.stack{display:grid;gap:14px}.row2{display:grid;grid-template-columns:1fr 1fr;gap:14px}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:16px}
.card.alert{border-color:#f3c2c2}
.kh{display:flex;justify-content:space-between;align-items:flex-start;color:var(--muted);font-size:12px;font-weight:500}
.ki{width:28px;height:28px;border-radius:7px;display:grid;place-items:center}
.big{font-size:34px;font-weight:700;margin:4px 0 6px}.alert .big,.alert .kh{color:var(--red)}
.note{color:var(--muted);font-size:12px;line-height:1.45}
.live-dot{display:inline-flex;align-items:center;gap:6px;font-size:12px;color:var(--muted)}
.live-dot i{width:7px;height:7px;border-radius:50%;background:var(--ok)}
.al{display:flex;gap:10px;padding:10px 0;border-top:1px solid var(--line)}.al:first-child{border-top:0}
.al .t{font-weight:600;font-size:12px;min-width:62px}.al .m{color:var(--muted);font-size:13px}
.dot{width:8px;height:8px;border-radius:50%;margin-top:5px;flex:none;display:inline-block}
.pill{font-size:11px;font-weight:700;padding:3px 8px;border-radius:999px;letter-spacing:.4px}
.pill.on{background:#e3f4ea;color:var(--ok)}.pill.off{background:#eceef2;color:var(--muted)}
.result{display:flex;align-items:center;gap:14px;margin:14px 0 10px;padding:14px;border-radius:10px;background:var(--soft)}
.result .ic{width:46px;height:46px;border-radius:10px;display:grid;place-items:center;color:#fff;font-size:22px;font-weight:700;flex:none}
.result .lbl{font-size:22px;font-weight:700}
.fb{display:none;align-items:center;gap:6px;flex-wrap:wrap;margin-bottom:12px;font-size:12px;color:var(--muted)}
.bar{height:10px;background:var(--soft);border-radius:6px;overflow:hidden;margin:6px 0 12px}
.bar i{display:block;height:100%;border-radius:6px;transition:width .4s}
.bl{display:flex;justify-content:space-between;font-size:13px;font-weight:500}
.sens{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin:10px 0 14px}
.sens div{background:var(--soft);border-radius:8px;padding:8px 10px;font-size:12px;color:var(--muted)}
.sens b{display:block;color:var(--text);font-size:13px}
.ctl{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.legend{display:flex;gap:14px;flex-wrap:wrap;font-size:12px;color:var(--muted);margin-top:6px}
.legend span{display:inline-flex;align-items:center;gap:6px}.legend b{color:var(--text)}
.legend .dot{margin:0}
.sw{width:10px;height:10px;border-radius:3px;display:inline-block;flex:none}
.mapwrap{display:grid;grid-template-columns:minmax(0,1fr) 300px;gap:14px;height:calc(100vh - 190px);min-height:420px}
#map{border-radius:12px;border:1px solid var(--line);background:#dfe7ef;height:100%}
.na{overflow:auto;padding:0}.na h3{margin:0;font-size:14px;padding:14px 16px 0}.na .note{padding:2px 16px 8px}
.bi{display:flex;gap:10px;align-items:flex-start;padding:11px 16px;border-top:1px solid var(--line);cursor:pointer}
.bi:hover,.bi.sel{background:var(--soft)}.bi .n{flex:1}.bi .n b{display:block;font-size:12px}.bi .n small{color:var(--muted)}
.bi .pc{font-weight:700}
.tag{font-size:10px;font-weight:700;color:var(--muted);border:1px solid var(--line);border-radius:4px;padding:1px 4px;margin-left:4px}
.tag.live{color:var(--ok);border-color:#bfe3cd}
.pin{width:34px;height:34px;border-radius:50% 50% 50% 0;transform:rotate(-45deg);display:grid;place-items:center;color:#fff;font-weight:700;font-size:11px;border:2px solid #fff;box-shadow:0 2px 6px rgba(0,0,0,.3)}
.pin span{transform:rotate(45deg)}
.pin.live{outline:3px solid rgba(47,158,90,.45);outline-offset:2px}
.s-full{background:var(--red)}.s-near{background:var(--orange)}.s-ok{background:var(--ok)}
.offline{display:none;background:#fff4e5;border:1px solid #f5d29a;color:#8a5a12;border-radius:10px;padding:10px 14px;margin-bottom:14px;font-size:13px}
.view{display:none}.view.on{display:block}
.mapfallback{display:grid;place-items:center;height:100%;color:var(--muted);text-align:center;padding:20px}
/* analytics */
.tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:14px;margin-bottom:14px}
.tile .v{font-size:26px;font-weight:700;margin:6px 0 2px}
.seg{display:inline-flex;background:var(--soft);border-radius:9px;padding:3px;gap:2px}
.seg button{border:0;background:transparent;font:inherit;font-weight:500;color:var(--muted);padding:6px 12px;border-radius:7px;cursor:pointer}
.seg button.on{background:#fff;color:var(--text);box-shadow:0 1px 2px rgba(0,0,0,.08)}
.agrid{display:grid;grid-template-columns:minmax(0,1.4fr) minmax(0,1fr);gap:14px;margin-bottom:14px}
.chart{position:relative;margin-top:10px}
.chart svg{display:block;width:100%;overflow:visible}
.chart text{font-size:11px;fill:var(--axislbl);font-variant-numeric:tabular-nums}
.empty{color:var(--muted);font-size:13px;padding:40px 0;text-align:center}
#tip{position:fixed;pointer-events:none;background:#fff;border:1px solid var(--line);box-shadow:0 4px 14px rgba(13,27,46,.12);border-radius:8px;padding:8px 10px;font-size:12px;display:none;z-index:2000;min-width:130px}
#tip b{display:block;margin-bottom:4px}
#tip .r{display:flex;align-items:center;gap:6px;justify-content:space-between}
#tip .r span{display:inline-flex;align-items:center;gap:6px;color:var(--muted)}
.comp{display:flex;height:18px;gap:2px;margin:14px 0 12px}
.comp i{display:block;height:100%}.comp i:first-child{border-radius:4px 0 0 4px}.comp i:last-child{border-radius:0 4px 4px 0}
.comp i:only-child{border-radius:4px}
.crow{display:flex;align-items:center;gap:8px;padding:6px 0;font-size:13px}.crow .nm{flex:1}.crow .pct{color:var(--muted);min-width:44px;text-align:right}
table{width:100%;border-collapse:collapse;font-size:13px}
th{text-align:left;font-size:11px;font-weight:600;color:var(--muted);text-transform:uppercase;letter-spacing:.4px;padding:8px 10px;border-bottom:1px solid var(--line)}
td{padding:9px 10px;border-bottom:1px solid var(--line);vertical-align:middle}
td.num,th.num{text-align:right;font-variant-numeric:tabular-nums}
.chip{display:inline-flex;align-items:center;gap:6px;font-weight:600;font-size:12px}
.cm td{text-align:center;font-variant-numeric:tabular-nums}.cm td.diag{background:#e8f5ee;font-weight:700}.cm th{text-align:center}
.insight{display:flex;gap:10px;padding:8px 0;font-size:13px}.insight i{font-style:normal}
.tbar{display:flex;justify-content:space-between;align-items:center;gap:10px;flex-wrap:wrap;margin-bottom:12px}
.fbsel{font:inherit;font-size:12px;border:1px solid var(--line);border-radius:6px;padding:4px 6px;background:#fff}
/* settings */
.form{display:grid;grid-template-columns:1fr 1fr;gap:12px 16px;margin-top:12px}
.form label{font-size:12px;color:var(--muted);font-weight:500;display:grid;gap:5px}
.form input{font:inherit;border:1px solid var(--line);border-radius:8px;padding:8px 10px;color:var(--text)}
.kv{display:grid;grid-template-columns:auto 1fr;gap:6px 16px;font-size:13px;margin-top:10px}.kv span{color:var(--muted)}
.saved{color:var(--ok);font-size:12px;font-weight:600;display:none}
@media (max-width:900px){
 .side{display:none}.cols,.agrid{grid-template-columns:1fr}.mapwrap{grid-template-columns:1fr;height:auto}#map{height:380px}
 .mtabs{display:flex!important}.user{display:none}main{padding:16px}.form{grid-template-columns:1fr}
}
.mtabs{display:none;gap:6px;margin-bottom:14px;overflow-x:auto}.mtabs a{flex:1;text-align:center;white-space:nowrap}
@media print{
 .top,.side,.mtabs,.offline,.noprint,#tip{display:none!important}
 body{background:#fff}main{padding:0}.card{break-inside:avoid}
}
</style>
</head><body>
<header class="top">
  <div class="logo"><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M7 19H4.8a1.8 1.8 0 0 1-1.6-2.7L7.2 9.6"/><path d="M11 19h8.2a1.8 1.8 0 0 0 1.6-2.7l-1.2-2.1"/><path d="M14 16l-3 3 3 3"/><path d="M8.3 13.6 7.2 9.6l-4 1.1"/><path d="m9.3 5.3.9-1.6a1.8 1.8 0 0 1 3.2 0l3.9 6.8"/><path d="m13.4 10.6 3.9-1.1 1.1 4"/></svg></div>
  <div class="brand">CABWMS <span>: Cabadbaran Waste Monitoring System</span></div>
  <div class="sp"></div>
  <a class="bell" href="#dashboard" title="Alerts"><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2"><path d="M6 8a6 6 0 0 1 12 0c0 7 3 9 3 9H3s3-2 3-9"/><path d="M10.3 21a1.9 1.9 0 0 0 3.4 0"/></svg><b id="bellN">0</b></a>
  <div class="user">Jimuel Alikuto<br><small>CENRO Supervisor</small></div>
  <div class="av">JA</div>
</header>
<div class="shell">
  <aside class="side">
    <a class="nav" href="#dashboard" data-v="dashboard"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="7" height="7" rx="1"/><rect x="14" y="3" width="7" height="7" rx="1"/><rect x="3" y="14" width="7" height="7" rx="1"/><rect x="14" y="14" width="7" height="7" rx="1"/></svg>Dashboard</a>
    <a class="nav" href="#map" data-v="map"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 4 3 6v14l6-2 6 2 6-2V4l-6 2-6-2z"/><path d="M9 4v14M15 6v14"/></svg>Map View</a>
    <a class="nav" href="#analytics" data-v="analytics"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 20V10M10 20V4M16 20v-7M22 20H2"/></svg>Analytics</a>
    <a class="nav" href="#history" data-v="history"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/></svg>History</a>
    <a class="nav" href="#settings" data-v="settings"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.7 1.7 0 0 0 .3 1.8l.1.1a2 2 0 1 1-2.8 2.8l-.1-.1a1.7 1.7 0 0 0-1.8-.3 1.7 1.7 0 0 0-1 1.5V21a2 2 0 1 1-4 0v-.1a1.7 1.7 0 0 0-1.1-1.5 1.7 1.7 0 0 0-1.8.3l-.1.1a2 2 0 1 1-2.8-2.8l.1-.1a1.7 1.7 0 0 0 .3-1.8 1.7 1.7 0 0 0-1.5-1H3a2 2 0 1 1 0-4h.1a1.7 1.7 0 0 0 1.5-1.1 1.7 1.7 0 0 0-.3-1.8l-.1-.1a2 2 0 1 1 2.8-2.8l.1.1a1.7 1.7 0 0 0 1.8.3H9a1.7 1.7 0 0 0 1-1.5V3a2 2 0 1 1 4 0v.1a1.7 1.7 0 0 0 1 1.5 1.7 1.7 0 0 0 1.8-.3l.1-.1a2 2 0 1 1 2.8 2.8l-.1.1a1.7 1.7 0 0 0-.3 1.8V9a1.7 1.7 0 0 0 1.5 1H21a2 2 0 1 1 0 4h-.1a1.7 1.7 0 0 0-1.5 1z"/></svg>Settings</a>
    <div class="sp"></div>
    <div class="cw"><b>&#128337; Collection window</b>Route trucks dispatch daily at 8:00 AM and 1:00 PM. Full bins logged after 1:00 PM roll to the next window.</div>
  </aside>
  <main>
    <div class="mtabs"><a class="btn sm" href="#dashboard">Dashboard</a><a class="btn sm" href="#map">Map</a><a class="btn sm" href="#analytics">Analytics</a><a class="btn sm" href="#history">History</a><a class="btn sm" href="#settings">Settings</a></div>
    <div class="offline" id="offline">&#9888; Cannot reach the smart bin. Check that the DevKit is powered and on the <b>lalalala</b> WiFi. Showing last known data.</div>

    <!-- MAIN DASHBOARD -->
    <section class="view" id="view-dashboard">
      <div class="head">
        <div><h1>Main dashboard</h1><p class="sub" id="subline">City Environment and Natural Resources Office</p></div>
        <a class="btn dark" href="#map">Open map view &rarr;</a>
      </div>
      <div class="cols">
        <div class="stack">
          <div class="card alert">
            <div class="kh">Full bins<div class="ki" style="background:#fdeaea">&#9888;&#65039;</div></div>
            <div class="big" id="kFull">0</div>
            <div class="note" id="kFullNote">At or above the alert threshold.</div>
          </div>
          <div class="row2">
            <div class="card"><div class="kh">Total bins<div class="ki" style="background:#e8eefb">&#128465;</div></div>
              <div class="big">12</div><div class="note">1 live prototype + 11 sample bins</div></div>
            <div class="card"><div class="kh">Active bins<div class="ki" style="background:#e3f4ea">&#128246;</div></div>
              <div class="big" id="kActive">11</div><div class="note" id="kActiveNote">Reporting now</div></div>
          </div>
          <div class="card"><div class="kh">Recent alerts<div class="ki" style="background:#fff2df">&#128276;</div></div>
            <div class="big" id="kAlerts">0</div><div class="note">From the live smart bin since it was powered on</div></div>
          <div class="card">
            <div class="kh" style="margin-bottom:6px"><b style="color:var(--text);font-size:13px">System alerts</b><span class="live-dot"><i></i>Live</span></div>
            <div id="alerts"><div class="note">No events yet. Put trash on the lid to start.</div></div>
          </div>
        </div>

        <div class="card" id="liveCard">
          <div class="kh"><b style="color:var(--text);font-size:15px">Live smart bin</b><span class="pill off" id="livePill">OFFLINE</span></div>
          <div class="note">BIN-CBD-001 &middot; City Plaza &middot; AI waste sorter prototype</div>
          <div class="result">
            <div class="ic" id="resIc" style="background:#9aa4b2">?</div>
            <div><div class="lbl" id="resLbl">Waiting for trash</div><div class="note" id="resNote">The camera classifies each item in about 10 seconds</div></div>
          </div>
          <div class="fb" id="fbRow">Was the AI right?
            <button class="btn sm" data-fb="correct">&#10003; Correct</button>
            <button class="btn sm" data-fb="plastic">It was plastic</button>
            <button class="btn sm" data-fb="bio">It was bio</button>
            <button class="btn sm" data-fb="nothing">Nothing there</button>
          </div>
          <div class="note" style="margin-bottom:10px">Status: <b id="stateTxt" style="color:var(--text)">&mdash;</b></div>
          <div class="bl"><span>&#9851; Plastic compartment</span><span id="fpTxt">0%</span></div>
          <div class="bar"><i id="fpBar" style="width:0;background:var(--s-plastic)"></i></div>
          <div class="bl"><span>&#127807; Biodegradable compartment</span><span id="fbTxt">0%</span></div>
          <div class="bar"><i id="fbBar" style="width:0;background:var(--s-bio)"></i></div>
          <div class="note" id="fillSrc" style="margin-top:-6px;margin-bottom:6px">Fill level estimated from the number of sorted items</div>
          <div class="sens">
            <div>Plastic sorted<b id="cP">0</b></div><div>Biodegradable sorted<b id="cB">0</b></div>
            <div>IR sensor<b id="sIr">&mdash;</b></div><div>Camera AI<b id="sCam">&mdash;</b></div>
            <div>Servo lid<b id="sServo">&mdash;</b></div><div>Wi-Fi signal<b id="sWifi">&mdash;</b></div>
          </div>
          <div class="ctl">
            <button class="btn dark" data-a="classify">&#128247; Classify now</button>
            <button class="btn" data-a="reset">&#10003; Mark as emptied</button>
            <button class="btn" data-a="plastic">&#8592; Tilt to plastic</button>
            <button class="btn" data-a="bio">Tilt to bio &#8594;</button>
          </div>
        </div>
      </div>
    </section>

    <!-- INTERACTIVE MAP VIEW -->
    <section class="view" id="view-map">
      <div class="head">
        <div><h1>Interactive map view</h1>
          <p class="sub">Cabadbaran City &middot; 12 monitored bins &middot; tap a pin for bin details</p>
          <div class="legend">
            <span><i class="dot" style="background:var(--red)"></i><b>FULL</b> <span class="thFull">85</span>% and above threshold</span>
            <span><i class="dot" style="background:var(--orange)"></i><b>NEARING FULL</b> 70% and above</span>
            <span><i class="dot" style="background:var(--ok)"></i><b>NORMAL</b> Below 70%</span>
          </div></div>
      </div>
      <div class="mapwrap">
        <div id="map"><div class="mapfallback" id="mapMsg">Loading map&hellip;</div></div>
        <div class="card na">
          <h3>Needs attention</h3><div class="note" id="naNote">0 bins at or nearing capacity</div>
          <div id="naList"></div>
        </div>
      </div>
    </section>

    <!-- ANALYTICS -->
    <section class="view" id="view-analytics">
      <div class="head">
        <div><h1>Analytics</h1><p class="sub">BIN-CBD-001 &middot; City Plaza &middot; sorting performance of the live smart bin</p></div>
        <div class="noprint" style="display:flex;gap:8px;align-items:center;flex-wrap:wrap">
          <div class="seg" id="range"><button data-r="24h" class="on">Last 24 hours</button><button data-r="7d">Last 7 days</button><button data-r="all">All time</button></div>
          <button class="btn" onclick="window.print()">&#128424; Print report</button>
        </div>
      </div>
      <div class="tiles">
        <div class="card tile"><div class="note">Items sorted</div><div class="v" id="tItems">0</div><div class="note" id="tItemsN">plastic + biodegradable</div></div>
        <div class="card tile"><div class="note">Sent to recycling</div><div class="v" id="tRecyc">&mdash;</div><div class="note">plastic share of sorted items</div></div>
        <div class="card tile"><div class="note">Field accuracy</div><div class="v" id="tAcc">&mdash;</div><div class="note" id="tAccN">no reviewed items yet</div></div>
        <div class="card tile"><div class="note">Average AI time</div><div class="v" id="tAi">&mdash;</div><div class="note">per photo on the ESP32-CAM</div></div>
        <div class="card tile"><div class="note">Pickups</div><div class="v" id="tPick">0</div><div class="note" id="tUns">times the bin was emptied</div></div>
      </div>
      <div class="agrid">
        <div class="card"><h2 id="barTitle">Items sorted per hour</h2><div class="note">Stacked by AI result &middot; hover a bar for counts</div>
          <div class="legend"><span><i class="sw" style="background:var(--s-plastic)"></i>Plastic</span><span><i class="sw" style="background:var(--s-bio)"></i>Biodegradable</span><span><i class="sw" style="background:var(--s-unsure)"></i>Unsure / no answer</span></div>
          <div class="chart" id="barChart"></div></div>
        <div class="card"><h2>Waste composition</h2><div class="note">Share of items in the selected range</div>
          <div id="comp"></div></div>
      </div>
      <div class="agrid">
        <div class="card"><h2>Compartment fill level</h2><div class="note">Estimated from sorted items &middot; resets when the bin is marked as emptied</div>
          <div class="legend"><span><i class="sw" style="background:var(--s-plastic)"></i>Plastic</span><span><i class="sw" style="background:var(--s-bio)"></i>Biodegradable</span><span><i class="sw" style="background:none;border-top:2px dashed var(--red);height:0;width:14px;border-radius:0"></i>Full threshold</span></div>
          <div class="chart" id="lineChart"></div></div>
        <div class="card"><h2>Field accuracy (AI vs actual)</h2><div class="note">From operator reviews on the dashboard and History page</div>
          <div id="cmWrap" style="margin-top:10px"></div>
          <h2 style="margin-top:16px">Insights</h2><div id="insights"></div></div>
      </div>
    </section>

    <!-- HISTORY -->
    <section class="view" id="view-history">
      <div class="head">
        <div><h1>History</h1><p class="sub">Every item the smart bin classified &middot; review results to measure field accuracy</p></div>
        <button class="btn" id="csvBtn">&#11015; Export CSV</button>
      </div>
      <div class="card">
        <div class="tbar">
          <div class="seg" id="hFilter"><button data-f="all" class="on">All</button><button data-f="P">Plastic</button><button data-f="B">Biodegradable</button><button data-f="EUT">Unsure / empty</button><button data-f="R">Pickups</button></div>
          <span class="note" id="hCount"></span>
        </div>
        <div style="overflow-x:auto"><table>
          <thead><tr><th>#</th><th>Time</th><th>AI result</th><th class="num">AI time</th><th>Review</th></tr></thead>
          <tbody id="hBody"><tr><td colspan="5" class="empty">Loading&hellip;</td></tr></tbody>
        </table></div>
      </div>
    </section>

    <!-- SETTINGS -->
    <section class="view" id="view-settings">
      <div class="head"><div><h1>Settings</h1><p class="sub">Calibrate the lid and fill estimate without re-uploading the program</p></div></div>
      <div class="cols">
        <div class="stack">
          <div class="card"><h2>Servo lid calibration</h2>
            <div class="note">Center is 90&deg;. Save, then use the test buttons to check that items slide into the right compartment.</div>
            <div class="form">
              <label>Plastic side angle (&deg;)<input type="number" id="setPA" min="0" max="180"></label>
              <label>Biodegradable side angle (&deg;)<input type="number" id="setBA" min="0" max="180"></label>
              <label>Hold time while tilted (ms)<input type="number" id="setTM" min="300" max="5000" step="100"></label>
            </div>
          </div>
          <div class="card"><h2>Fill level estimate</h2>
            <div class="note">No ultrasonic level sensor is fitted yet, so each sorted item adds a fixed amount.</div>
            <div class="form">
              <label>Fill added per item (%)<input type="number" id="setFI" min="1" max="50"></label>
              <label>Full alert threshold (%)<input type="number" id="setFT" min="50" max="100"></label>
            </div>
          </div>
          <div style="display:flex;gap:8px;align-items:center;flex-wrap:wrap">
            <button class="btn dark" id="saveBtn">Save settings</button>
            <button class="btn" data-a="plastic">Test plastic tilt</button>
            <button class="btn" data-a="bio">Test bio tilt</button>
            <span class="saved" id="savedMsg">&#10003; Saved</span>
          </div>
        </div>
        <div class="stack">
          <div class="card"><h2>Device</h2><div class="kv" id="devInfo"></div></div>
          <div class="card"><h2>Danger zone</h2>
            <div class="note" style="margin-bottom:10px">Clears the sorting history, totals and reviews stored on the smart bin. Settings are kept.</div>
            <button class="btn danger" id="clearBtn">Clear statistics</button></div>
        </div>
      </div>
    </section>
  </main>
</div>
<div id="tip"></div>

<script>
let TH_FULL=85; const TH_NEAR=70;
// BIN-CBD-001 is the real prototype; the rest are sample bins (approximate locations) to fill the wireframe.
const BINS=[
 {id:'BIN-CBD-001',name:'City Plaza',lat:9.12365,lng:125.53405,fill:0,live:true},
 {id:'BIN-CBD-002',name:'Cabadbaran City Hall',lat:9.1251,lng:125.5372,fill:92},
 {id:'BIN-CBD-007',name:'Integrated Bus Terminal',lat:9.1185,lng:125.5318,fill:91},
 {id:'BIN-CBD-003',name:'Public Market — Wet Section',lat:9.1222,lng:125.5329,fill:88},
 {id:'BIN-CBD-011',name:'Barangay Sanghan Covered Court',lat:9.1145,lng:125.5345,fill:79},
 {id:'BIN-CBD-004',name:'Agusan del Norte Provincial Capitol',lat:9.1290,lng:125.5395,fill:74},
 {id:'BIN-CBD-009',name:'Barangay La Union Chapel',lat:9.1160,lng:125.5375,fill:61},
 {id:'BIN-CBD-006',name:'Barangay Comagascas Hall',lat:9.1310,lng:125.5340,fill:55},
 {id:'BIN-CBD-010',name:'District Hospital',lat:9.1270,lng:125.5320,fill:50},
 {id:'BIN-CBD-005',name:'Central Elementary School',lat:9.1205,lng:125.5362,fill:42},
 {id:'BIN-CBD-008',name:'Rizal Park',lat:9.1240,lng:125.5390,fill:33},
 {id:'BIN-CBD-012',name:'Port Area',lat:9.1300,lng:125.5280,fill:21}
];
const SER={P:{n:'Plastic',c:'var(--s-plastic)'},B:{n:'Biodegradable',c:'var(--s-bio)'},U:{n:'Unsure / no answer',c:'var(--s-unsure)'}};
const RES={P:'Plastic',B:'Biodegradable',E:'Empty lid',U:'Unsure',T:'No answer',R:'Bin emptied'};
const RESC={P:'var(--s-plastic)',B:'var(--s-bio)',E:'#c3c2b7',U:'var(--s-unsure)',T:'var(--red)',R:'var(--navy)'};
const FBV=['','correct','plastic','bio','nothing'];
const $=id=>document.getElementById(id);
const esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const stOf=f=>f>=TH_FULL?'full':f>=TH_NEAR?'near':'ok';
const stColor={full:'var(--red)',near:'var(--orange)',ok:'var(--ok)'};
let S=null,H=null,lastOk=0,online=false,map=null,markers={},mapTried=false,view='dashboard',range='24h',hFilter='all',histAt=0,lastSeenId=-1,settingsLoaded=false;

$('subline').textContent='City Environment and Natural Resources Office · '+
  new Date().toLocaleDateString('en-US',{weekday:'long',month:'long',day:'numeric'})+' · alert threshold '+TH_FULL+'%';
fetch('/api/time?epoch='+Math.floor(Date.now()/1000),{method:'POST'}).catch(()=>{});

// ----- navigation -----
function route(){
  const h=location.hash.slice(1);
  view=['map','analytics','history','settings'].includes(h)?h:'dashboard';
  document.querySelectorAll('.view').forEach(e=>e.classList.toggle('on',e.id==='view-'+view));
  document.querySelectorAll('.nav').forEach(e=>e.classList.toggle('on',e.dataset.v===view));
  if(view==='map'){ initMap(); if(map) setTimeout(()=>map.invalidateSize(),50); }
  if(view==='analytics'||view==='history') loadHistory();
  if(view==='settings') settingsLoaded=false;
  render();
}
addEventListener('hashchange',route);

// ----- data -----
async function refresh(){
  try{ const r=await fetch('/api/status',{cache:'no-store'}); S=await r.json(); lastOk=Date.now(); online=true; }
  catch(e){ online=false; }
  if(S){ TH_FULL=S.settings.fullThreshold; }
  if((view==='analytics'||view==='history') && S && (S.last.id!==lastSeenId || Date.now()-histAt>15000)) loadHistory();
  render();
}
async function loadHistory(){
  histAt=Date.now(); if(S) lastSeenId=S.last.id;
  try{ const r=await fetch('/api/history',{cache:'no-store'}); H=await r.json(); }catch(e){}
  renderAnalytics(); renderHistory();
}
// record: [id, epoch, uptime, aiMs, boot, result, feedback] -> ms timestamp or null
function recTime(r){
  if(r[1]>0) return r[1]*1000;
  if(H && r[4]===H.boot) return Date.now()-(H.uptime-r[2])*1000;
  return null;
}
function evTime(t){ return new Date(Date.now()-(S.uptime-t)*1000).toLocaleTimeString([],{hour:'numeric',minute:'2-digit'}); }
function ago(t){ const s=Math.max(0,S.uptime-t); return s<60?s+'s ago':Math.floor(s/60)+' min ago'; }
const fmtTime=ms=>ms===null?'Unknown time':new Date(ms).toLocaleString([],{month:'short',day:'numeric',hour:'numeric',minute:'2-digit'});

function render(){
  $('offline').style.display=online||!lastOk?'none':'block';
  document.querySelectorAll('.thFull').forEach(e=>e.textContent=TH_FULL);
  const live=BINS[0];
  if(S){ live.fill=Math.max(S.fill.plastic,S.fill.bio); }
  const full=BINS.filter(b=>b.fill>=TH_FULL);
  $('kFull').textContent=full.length;
  $('kFullNote').textContent='At or above the alert threshold. '+(full.some(b=>b.live)?'Includes the live smart bin — schedule a pickup.':'Live smart bin is below the threshold.');
  $('kActive').textContent=11+(online?1:0);
  $('kActiveNote').textContent=online?'All bins reporting · live bin connected':'Live smart bin not reachable';
  $('livePill').textContent=online?'LIVE':'OFFLINE';
  $('livePill').className='pill '+(online?'on':'off');
  if(S){
    const it=S.last.item, M={PLASTIC:['Plastic','var(--s-plastic)','P'],BIO:['Biodegradable','var(--s-bio)','B'],
      EMPTY:['Nothing detected','#9aa4b2','–'],UNKNOWN:['Not sure','var(--s-unsure)','?'],TIMEOUT:['Camera did not answer','var(--red)','!']};
    const m=M[it]||['Waiting for trash','#9aa4b2','?'];
    $('resLbl').textContent=m[0]; $('resIc').style.background=m[1]; $('resIc').textContent=m[2];
    $('resNote').textContent=it?('Detected '+ago(S.last.at)+(S.last.ms?' · AI took '+(S.last.ms/1000).toFixed(1)+' s':'')+(S.last.feedback?' · reviewed ✓':'')):'The camera classifies each item in about 10 seconds';
    $('fbRow').style.display=(S.last.id&&!S.last.feedback&&['PLASTIC','BIO','EMPTY','UNKNOWN'].includes(it))?'flex':'none';
    const ST={idle:'Waiting for trash',detected:'Trash detected on the lid',classifying:'AI is classifying the item (about 10 s)…',tilting:'Tilting the lid'};
    $('stateTxt').textContent=ST[S.state]||S.state;
    $('fpTxt').textContent=S.fill.plastic+'%'; $('fpBar').style.width=S.fill.plastic+'%';
    $('fbTxt').textContent=S.fill.bio+'%'; $('fbBar').style.width=S.fill.bio+'%';
    $('fpBar').style.background=S.fill.plastic>=TH_FULL?'var(--red)':'var(--s-plastic)';
    $('fbBar').style.background=S.fill.bio>=TH_FULL?'var(--red)':'var(--s-bio)';
    $('cP').textContent=S.totals.plastic; $('cB').textContent=S.totals.bio;
    $('sIr').textContent=S.ir?'Object on lid':'Clear';
    $('sCam').textContent={online:'Online',waiting:'Waiting for first item','no reply':'No reply'}[S.camera]||S.camera;
    $('sServo').textContent=S.servo+'° '+(S.state==='tilting'?'(moving)':'(at rest)');
    $('sWifi').textContent=S.rssi+' dBm';
    document.querySelectorAll('[data-a]').forEach(b=>b.disabled=!online||S.state!=='idle');
    const alerts=S.events.filter(e=>e.type==='alert'||e.type==='warn');
    $('kAlerts').textContent=alerts.length; $('bellN').textContent=alerts.length;
    const dotC={alert:'var(--red)',warn:'var(--orange)',sorted:'var(--s-plastic)',info:'var(--ok)'};
    $('alerts').innerHTML=S.events.length?S.events.slice(0,8).map(e=>
      '<div class="al"><i class="dot" style="background:'+(dotC[e.type]||'var(--muted)')+'"></i><div class="t">'+evTime(e.t)+'</div><div class="m">'+esc(e.msg)+'</div></div>').join('')
      :'<div class="note">No events yet. Put trash on the lid to start.</div>';
    if(view==='settings') renderSettings();
  } else {
    document.querySelectorAll('[data-a]').forEach(b=>b.disabled=true);
  }
  renderMapData();
}

// ----- actions -----
document.querySelectorAll('[data-a]').forEach(b=>b.onclick=async()=>{
  b.disabled=true;
  try{ await fetch('/api/action?do='+b.dataset.a,{method:'POST'}); }catch(e){}
  setTimeout(refresh,300);
});
async function sendFeedback(id,v){
  try{ await fetch('/api/feedback?id='+id+'&v='+v,{method:'POST'}); }catch(e){}
  await refresh(); loadHistory();
}
document.querySelectorAll('[data-fb]').forEach(b=>b.onclick=()=>{ if(S&&S.last.id) sendFeedback(S.last.id,b.dataset.fb); });

// ----- map -----
function pinIcon(b){
  return L.divIcon({className:'',iconSize:[34,34],iconAnchor:[17,34],popupAnchor:[0,-30],
    html:'<div class="pin s-'+stOf(b.fill)+(b.live?' live':'')+'"><span>'+b.fill+'</span></div>'});
}
function popup(b){
  return '<b>'+esc(b.id)+'</b><br>'+esc(b.name)+'<br>Fill level: <b>'+b.fill+'%</b> ('+
    {full:'Full',near:'Nearing full',ok:'Normal'}[stOf(b.fill)]+')<br><small>'+
    (b.live?(online?'Live prototype · connected':'Live prototype · offline'):'Sample data — not connected')+'</small>';
}
function initMap(){
  if(mapTried) return; mapTried=true;
  const css=document.createElement('link'); css.rel='stylesheet'; css.href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css'; document.head.appendChild(css);
  const js=document.createElement('script'); js.src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js';
  js.onload=()=>{
    $('map').innerHTML='';
    map=L.map('map',{zoomControl:true}).setView([9.1235,125.5345],15);
    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',{maxZoom:19,attribution:'&copy; OpenStreetMap'}).addTo(map);
    BINS.forEach(b=>{ markers[b.id]=L.marker([b.lat,b.lng],{icon:pinIcon(b)}).addTo(map).bindPopup(popup(b)); });
    renderMapData();
  };
  js.onerror=()=>{ $('mapMsg').innerHTML='Map tiles need internet.<br>The bin list on the right still updates live.'; };
  document.head.appendChild(js);
}
function renderMapData(){
  const na=BINS.filter(b=>b.fill>=TH_NEAR).sort((a,b)=>b.fill-a.fill);
  $('naNote').textContent=na.length+' bins at or nearing capacity';
  $('naList').innerHTML=na.map(b=>'<div class="bi" data-id="'+b.id+'"><i class="dot" style="background:'+stColor[stOf(b.fill)]+'"></i><div class="n"><b>'+esc(b.id)+
    (b.live?'<span class="tag live">LIVE</span>':'<span class="tag">SAMPLE</span>')+'</b><small>'+esc(b.name)+'</small></div><div class="pc">'+b.fill+'%</div></div>').join('')
    +(BINS[0].fill<TH_NEAR?'<div class="bi" data-id="BIN-CBD-001"><i class="dot" style="background:var(--ok)"></i><div class="n"><b>BIN-CBD-001<span class="tag live">LIVE</span></b><small>City Plaza · normal</small></div><div class="pc">'+BINS[0].fill+'%</div></div>':'');
  document.querySelectorAll('.bi').forEach(el=>el.onclick=()=>{
    const m=markers[el.dataset.id]; if(m&&map){ map.setView(m.getLatLng(),17); m.openPopup(); }
  });
  if(map){ const b=BINS[0], m=markers[b.id]; m.setIcon(pinIcon(b)); m.setPopupContent(popup(b)); }
}

// ----- tooltip -----
const tip=$('tip');
function showTip(e,html){ tip.innerHTML=html; tip.style.display='block';
  const x=Math.min(e.clientX+14,innerWidth-tip.offsetWidth-8), y=Math.min(e.clientY+14,innerHeight-tip.offsetHeight-8);
  tip.style.left=x+'px'; tip.style.top=y+'px'; }
function hideTip(){ tip.style.display='none'; }
const tipRow=(c,n,v)=>'<div class="r"><span><i class="sw" style="background:'+c+'"></i>'+n+'</span><b style="margin:0">'+v+'</b></div>';

// ----- charts (inline SVG) -----
const NS='http://www.w3.org/2000/svg';
function el(tag,attrs,parent){ const e=document.createElementNS(NS,tag); for(const k in attrs) e.setAttribute(k,attrs[k]); if(parent) parent.appendChild(e); return e; }
function niceMax(v){ if(v<=4) return 4; const p=Math.pow(10,Math.floor(Math.log10(v))); const n=v/p; return (n<=2?2:n<=5?5:10)*p; }
// rectangle with 4px rounded top corners, square at the bottom (anchored to the baseline)
function topRounded(x,y,w,h,r){ r=Math.min(r,w/2,h); return 'M'+x+','+(y+h)+'V'+(y+r)+'Q'+x+','+y+' '+(x+r)+','+y+'H'+(x+w-r)+'Q'+(x+w)+','+y+' '+(x+w)+','+(y+r)+'V'+(y+h)+'Z'; }

function barChart(box,buckets){
  box.innerHTML='';
  const total=buckets.reduce((a,b)=>a+b.P+b.B+b.U,0);
  if(!total){ box.innerHTML='<div class="empty">No items sorted in this range yet</div>'; return; }
  const W=box.clientWidth||600, Hh=220, m={l:30,r:6,t:8,b:24}, iw=W-m.l-m.r, ih=Hh-m.t-m.b;
  const svg=el('svg',{viewBox:'0 0 '+W+' '+Hh,height:Hh},box);
  const ymax=niceMax(Math.max(...buckets.map(b=>b.P+b.B+b.U)));
  for(let i=0;i<=4;i++){ const v=ymax*i/4, y=m.t+ih-ih*i/4;
    el('line',{x1:m.l,x2:W-m.r,y1:y,y2:y,stroke:i?'var(--grid)':'var(--axis)','stroke-width':1},svg);
    if(i%2===0||ymax<=4){ const t=el('text',{x:m.l-6,y:y+4,'text-anchor':'end'},svg); t.textContent=Math.round(v); } }
  const bw=iw/buckets.length, barW=Math.max(4,Math.min(28,bw*0.62)), every=Math.ceil(buckets.length/8);
  buckets.forEach((b,i)=>{
    const x=m.l+i*bw+(bw-barW)/2; let y=m.t+ih;
    const segs=['P','B','U'].filter(k=>b[k]>0);
    segs.forEach((k,j)=>{
      const h=ih*b[k]/ymax, gap=j<segs.length-1?0:0, top=j===segs.length-1;
      const hh=Math.max(1,h-(j>0?2:0));          // 2px surface gap between stacked segments
      const yy=y-h;
      if(top) el('path',{d:topRounded(x,yy,barW,hh,4),fill:SER[k].c},svg);
      else el('rect',{x:x,y:yy,width:barW,height:hh,fill:SER[k].c},svg);
      y-=h;
    });
    if(i%every===0){ const t=el('text',{x:m.l+i*bw+bw/2,y:Hh-6,'text-anchor':'middle'},svg); t.textContent=b.label; }
    const hit=el('rect',{x:m.l+i*bw,y:m.t,width:bw,height:ih,fill:'transparent'},svg);
    hit.addEventListener('mousemove',e=>showTip(e,'<b>'+esc(b.full)+'</b>'+tipRow(SER.P.c,'Plastic',b.P)+tipRow(SER.B.c,'Biodegradable',b.B)+tipRow(SER.U.c,'Unsure',b.U)));
    hit.addEventListener('mouseleave',hideTip);
  });
}

function lineChart(box,series,t0,t1){
  box.innerHTML='';
  if(!series.some(s=>s.pts.length)){ box.innerHTML='<div class="empty">No fill changes in this range yet</div>'; return; }
  const W=box.clientWidth||600, Hh=220, m={l:34,r:118,t:10,b:24}, iw=W-m.l-m.r, ih=Hh-m.t-m.b;
  const svg=el('svg',{viewBox:'0 0 '+W+' '+Hh,height:Hh},box);
  if(t1<=t0) t1=t0+3600e3;
  const X=t=>m.l+iw*(t-t0)/(t1-t0), Y=v=>m.t+ih-ih*v/100;
  [0,50,100].forEach(v=>{ el('line',{x1:m.l,x2:m.l+iw,y1:Y(v),y2:Y(v),stroke:v?'var(--grid)':'var(--axis)'},svg);
    const t=el('text',{x:m.l-6,y:Y(v)+4,'text-anchor':'end'},svg); t.textContent=v+'%'; });
  el('line',{x1:m.l,x2:m.l+iw,y1:Y(TH_FULL),y2:Y(TH_FULL),stroke:'var(--red)','stroke-dasharray':'4 4','stroke-width':1.5},svg);
  const tl=el('text',{x:m.l+iw-4,y:Y(TH_FULL)-6,'text-anchor':'end'},svg); tl.textContent='Full alert '+TH_FULL+'%';
  const span=t1-t0, fmt=t=>new Date(t).toLocaleString([],span>2*864e5?{month:'short',day:'numeric'}:{hour:'numeric'});
  for(let i=0;i<=4;i++){ const t=t0+span*i/4; const tx=el('text',{x:X(t),y:Hh-6,'text-anchor':i===0?'start':i===4?'end':'middle'},svg); tx.textContent=fmt(t); }
  const ends=[];
  series.forEach(s=>{
    let d='M'+X(t0)+','+Y(s.start);
    let prev=s.start;
    s.pts.forEach(([t,v])=>{ d+='H'+X(t)+'V'+Y(v); prev=v; });
    d+='H'+X(t1);
    el('path',{d:d,fill:'none',stroke:s.c,'stroke-width':2,'stroke-linejoin':'round'},svg);
    ends.push({s:s,v:prev,y:Y(prev)+4});
  });
  // direct labels at the line ends, pushed apart so they never overlap
  ends.sort((a,b)=>a.y-b.y);
  for(let i=1;i<ends.length;i++) if(ends[i].y-ends[i-1].y<14) ends[i].y=ends[i-1].y+14;
  ends.forEach(e=>{ const lbl=el('text',{x:m.l+iw+6,y:e.y},svg); lbl.textContent=e.s.n+' '+e.v+'%'; lbl.style.fill='var(--text)'; });
  const valAt=(s,t)=>{ let v=s.start; s.pts.forEach(p=>{ if(p[0]<=t) v=p[1]; }); return v; };
  const cross=el('line',{y1:m.t,y2:m.t+ih,stroke:'var(--axis)',visibility:'hidden'},svg);
  const dots=series.map(s=>el('circle',{r:4,fill:s.c,stroke:'#fff','stroke-width':2,visibility:'hidden'},svg));
  const hit=el('rect',{x:m.l,y:m.t,width:iw,height:ih,fill:'transparent'},svg);
  hit.addEventListener('mousemove',e=>{
    const r=svg.getBoundingClientRect(), px=(e.clientX-r.left)*W/r.width, t=t0+(px-m.l)/iw*(t1-t0);
    cross.setAttribute('x1',px); cross.setAttribute('x2',px); cross.setAttribute('visibility','visible');
    let html='<b>'+new Date(t).toLocaleString([],{month:'short',day:'numeric',hour:'numeric',minute:'2-digit'})+'</b>';
    series.forEach((s,i)=>{ const v=valAt(s,t); dots[i].setAttribute('cx',px); dots[i].setAttribute('cy',Y(v)); dots[i].setAttribute('visibility','visible'); html+=tipRow(s.c,s.n,v+'%'); });
    showTip(e,html);
  });
  hit.addEventListener('mouseleave',()=>{ hideTip(); cross.setAttribute('visibility','hidden'); dots.forEach(d=>d.setAttribute('visibility','hidden')); });
}

// ----- analytics -----
document.querySelectorAll('#range button').forEach(b=>b.onclick=()=>{
  range=b.dataset.r; document.querySelectorAll('#range button').forEach(x=>x.classList.toggle('on',x===b)); renderAnalytics(); });

function renderAnalytics(){
  if(!H||view!=='analytics') return;
  const now=Date.now();
  const all=H.recs.map(r=>({id:r[0],t:recTime(r),ai:r[3],res:r[5],fb:r[6]}));
  const timed=all.filter(r=>r.t!==null);
  const firstT=timed.length?Math.min(...timed.map(r=>r.t)):now;
  let from= range==='24h'?now-24*3600e3: range==='7d'?now-7*864e5: Math.min(firstT,now-24*3600e3);
  const inR=range==='all'?all:timed.filter(r=>r.t>=from);
  const cnt=k=>inR.filter(r=>k.includes(r.res)).length;
  const P=cnt('P'),B=cnt('B'),U=cnt('UT'),E=cnt('E'),R=cnt('R');
  $('tItems').textContent=P+B;
  $('tItemsN').textContent=P+' plastic · '+B+' biodegradable';
  $('tRecyc').textContent=P+B?Math.round(100*P/(P+B))+'%':'—';
  $('tPick').textContent=R;
  $('tUns').textContent=U+' unsure or unanswered · '+E+' empty-lid checks';
  const aiR=inR.filter(r=>'PBEU'.includes(r.res)&&r.ai>0);
  $('tAi').textContent=aiR.length?(aiR.reduce((a,r)=>a+r.ai,0)/aiR.length/1000).toFixed(1)+' s':'—';
  // field accuracy
  const rev=inR.filter(r=>r.fb>0&&'PBE'.includes(r.res));
  const actualOf=r=>r.fb===1?({P:'P',B:'B',E:'N'}[r.res]):({2:'P',3:'B',4:'N'}[r.fb]);
  const ok=rev.filter(r=>actualOf(r)===({P:'P',B:'B',E:'N'}[r.res])).length;
  $('tAcc').textContent=rev.length?Math.round(100*ok/rev.length)+'%':'—';
  $('tAccN').textContent=rev.length?ok+' of '+rev.length+' reviewed items correct':'review items to measure';
  const cols=[['P','Plastic'],['B','Biodegradable'],['N','Nothing']], rows=[['P','Plastic'],['B','Biodegradable'],['E','Empty lid']];
  $('cmWrap').innerHTML=rev.length?'<table class="cm"><thead><tr><th style="text-align:left">AI said &darr; / Actual &rarr;</th>'+cols.map(c=>'<th>'+c[1]+'</th>').join('')+'</tr></thead><tbody>'+
    rows.map(rw=>'<tr><th style="text-align:left">'+rw[1]+'</th>'+cols.map(c=>{ const n=rev.filter(r=>r.res===rw[0]&&actualOf(r)===c[0]).length;
      const diag=({P:'P',B:'B',E:'N'}[rw[0]])===c[0]; return '<td class="'+(diag?'diag':'')+'">'+n+'</td>'; }).join('')+'</tr>').join('')+'</tbody></table>'
    :'<div class="note">No reviews yet. After each item, press <b>&#10003; Correct</b> or the real type on the dashboard, or review items on the History page.</div>';
  // composition
  const tot=P+B+U;
  const parts=[['P',P],['B',B],['U',U]].filter(p=>p[1]>0);
  $('comp').innerHTML=tot?'<div class="comp">'+parts.map(p=>'<i style="flex:'+p[1]+';background:'+SER[p[0]].c+'"></i>').join('')+'</div>'+
    [['P',P],['B',B],['U',U]].map(p=>'<div class="crow"><i class="sw" style="background:'+SER[p[0]].c+'"></i><span class="nm">'+SER[p[0]].n+'</span><b>'+p[1]+'</b><span class="pct">'+Math.round(100*p[1]/tot)+'%</span></div>').join('')
    :'<div class="empty">No items in this range yet</div>';
  // bar buckets
  let buckets=[];
  if(range==='24h'){
    const start=new Date(now); start.setMinutes(0,0,0); const s0=start.getTime()-23*3600e3;
    for(let i=0;i<24;i++){ const t=s0+i*3600e3, d=new Date(t);
      buckets.push({from:t,to:t+3600e3,label:d.toLocaleTimeString([],{hour:'numeric'}),full:d.toLocaleString([],{month:'short',day:'numeric',hour:'numeric'})+' – '+new Date(t+3600e3).toLocaleTimeString([],{hour:'numeric'}),P:0,B:0,U:0}); }
    $('barTitle').textContent='Items sorted per hour';
  } else {
    const days=range==='7d'?7:Math.min(30,Math.max(7,Math.ceil((now-firstT)/864e5)+1));
    const start=new Date(now); start.setHours(0,0,0,0); const s0=start.getTime()-(days-1)*864e5;
    for(let i=0;i<days;i++){ const t=s0+i*864e5, d=new Date(t);
      buckets.push({from:t,to:t+864e5,label:d.toLocaleDateString([],{month:'short',day:'numeric'}),full:d.toLocaleDateString([],{weekday:'long',month:'short',day:'numeric'}),P:0,B:0,U:0}); }
    $('barTitle').textContent='Items sorted per day';
  }
  timed.forEach(r=>{ const b=buckets.find(b=>r.t>=b.from&&r.t<b.to); if(!b) return;
    if(r.res==='P') b.P++; else if(r.res==='B') b.B++; else if(r.res==='U'||r.res==='T') b.U++; });
  barChart($('barChart'),buckets);
  // fill timeline (replay sorted items, reset on pickups)
  const fpi=S?S.settings.fillPerItem:10;
  let fp=0,fbv=0; const sp={n:'Plastic',c:'var(--s-plastic)',pts:[],start:0}, sb={n:'Biodegradable',c:'var(--s-bio)',pts:[],start:0};
  timed.slice().sort((a,b)=>a.t-b.t).forEach(r=>{
    if(r.res==='P') fp=Math.min(100,fp+fpi); else if(r.res==='B') fbv=Math.min(100,fbv+fpi); else if(r.res==='R'){ fp=0; fbv=0; } else return;
    if(r.t<from){ sp.start=fp; sb.start=fbv; return; }
    sp.pts.push([r.t,fp]); sb.pts.push([r.t,fbv]);
  });
  lineChart($('lineChart'),[sp,sb],from,now);
  // insights
  const ins=[];
  const best=buckets.reduce((a,b)=>(b.P+b.B+b.U)>(a?a.P+a.B+a.U:0)?b:a,null);
  if(best&&best.P+best.B+best.U>0) ins.push(['&#128337;','Busiest '+(range==='24h'?'hour':'day')+': <b>'+esc(best.full)+'</b> with '+(best.P+best.B+best.U)+' items']);
  if(S){ const left=k=>Math.max(0,Math.ceil((TH_FULL-S.fill[k])/fpi));
    ins.push(['&#9851;','Plastic compartment: <b>'+S.fill.plastic+'%</b> — about '+left('plastic')+' more items until the '+TH_FULL+'% alert']);
    ins.push(['&#127807;','Biodegradable compartment: <b>'+S.fill.bio+'%</b> — about '+left('bio')+' more items until the '+TH_FULL+'% alert']); }
  if(U) ins.push(['&#9888;&#65039;',U+' item'+(U>1?'s were':' was')+' not sorted because the AI was unsure or the camera did not answer']);
  const noTime=all.length-timed.length; if(noTime&&range!=='all') ins.push(['&#8505;&#65039;',noTime+' older records have no clock time and appear only under All time']);
  $('insights').innerHTML=ins.map(i=>'<div class="insight"><i>'+i[0]+'</i><span>'+i[1]+'</span></div>').join('')||'<div class="note">Sort some items to see insights.</div>';
}

// ----- history -----
document.querySelectorAll('#hFilter button').forEach(b=>b.onclick=()=>{
  hFilter=b.dataset.f; document.querySelectorAll('#hFilter button').forEach(x=>x.classList.toggle('on',x===b)); renderHistory(); });
function renderHistory(){
  if(!H||view!=='history') return;
  const rows=H.recs.slice().reverse().filter(r=>hFilter==='all'||hFilter.includes(r[5]));
  $('hCount').textContent=rows.length+' of '+H.recs.length+' records (latest 250 kept on the device)';
  $('hBody').innerHTML=rows.length?rows.slice(0,150).map(r=>{
    const res=r[5], canReview='PBEU'.includes(res);
    const sel=canReview?'<select class="fbsel" data-id="'+r[0]+'">'+[['clear','Not reviewed'],['correct','✓ Correct'],['plastic','Wrong: was plastic'],['bio','Wrong: was bio'],['nothing','Wrong: nothing there']]
      .map(o=>'<option value="'+o[0]+'"'+((FBV[r[6]]||'clear')===o[0]?' selected':'')+'>'+o[1]+'</option>').join('')+'</select>':'<span class="note">&mdash;</span>';
    return '<tr><td class="note">'+r[0]+'</td><td>'+fmtTime(recTime(r))+'</td><td><span class="chip"><i class="sw" style="background:'+RESC[res]+'"></i>'+RES[res]+'</span></td><td class="num">'+(r[3]?(r[3]/1000).toFixed(1)+' s':'&mdash;')+'</td><td>'+sel+'</td></tr>';
  }).join(''):'<tr><td colspan="5" class="empty">No records yet</td></tr>';
  document.querySelectorAll('.fbsel').forEach(s=>s.onchange=()=>sendFeedback(s.dataset.id,s.value));
}
$('csvBtn').onclick=()=>{
  if(!H) return;
  const lines=['id,time,result,ai_seconds,review'].concat(H.recs.map(r=>{ const t=recTime(r);
    return [r[0],t===null?'':new Date(t).toISOString(),RES[r[5]],r[3]?(r[3]/1000).toFixed(1):'',FBV[r[6]]||''].join(','); }));
  const a=document.createElement('a'); a.href=URL.createObjectURL(new Blob([lines.join('\n')],{type:'text/csv'}));
  a.download='cabwms-bin-cbd-001-history.csv'; a.click(); setTimeout(()=>URL.revokeObjectURL(a.href),5000);
};

// ----- settings -----
function renderSettings(){
  if(!S) return;
  if(!settingsLoaded){ const c=S.settings; $('setPA').value=c.plasticAngle; $('setBA').value=c.bioAngle; $('setTM').value=c.tiltMs; $('setFI').value=c.fillPerItem; $('setFT').value=c.fullThreshold; settingsLoaded=true; }
  const up=S.uptime, upTxt=Math.floor(up/3600)+' h '+Math.floor(up%3600/60)+' min';
  $('devInfo').innerHTML=[['Dashboard address','http://'+esc(S.ip)+'/'],['Wi-Fi signal',S.rssi+' dBm'],['Uptime',upTxt],['Clock',S.time.known?('set by '+S.time.source):'not set yet'],
    ['Records stored',S.records+' of 250'],['Free memory',Math.round(S.heap/1024)+' KB'],['Restarts',S.boot],['Program built',esc(S.build)]]
    .map(k=>'<span>'+k[0]+'</span><b>'+k[1]+'</b>').join('');
}
$('saveBtn').onclick=async()=>{
  const q='plasticAngle='+$('setPA').value+'&bioAngle='+$('setBA').value+'&tiltMs='+$('setTM').value+'&fillPerItem='+$('setFI').value+'&fullThreshold='+$('setFT').value;
  try{ await fetch('/api/settings?'+q,{method:'POST'}); $('savedMsg').style.display='inline'; setTimeout(()=>$('savedMsg').style.display='none',2500); settingsLoaded=false; }catch(e){}
  refresh();
};
$('clearBtn').onclick=async()=>{
  if(!confirm('Clear all sorting history, totals and reviews on the smart bin? This cannot be undone.')) return;
  try{ await fetch('/api/action?do=clearstats',{method:'POST'}); }catch(e){}
  setTimeout(()=>{ refresh(); loadHistory(); },500);
};
addEventListener('resize',()=>{ if(view==='analytics') renderAnalytics(); });

route(); refresh(); setInterval(refresh,1500);
</script>
</body></html>)rawliteral";
