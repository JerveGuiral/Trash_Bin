// CABWMS web dashboard (Main dashboard + Interactive map view), served by the DevKit at "/".
// Live data comes from /api/status; buttons call /api/action?do=classify|plastic|bio|reset.
// Only BIN-CBD-001 is the real prototype bin - the other 11 bins are sample data for the wireframe.
#pragma once

static const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>CABWMS Dashboard</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>
:root{--navy:#0d1b2e;--bg:#f3f5f8;--card:#fff;--line:#e3e7ee;--text:#1b2433;--muted:#6b7686;
--brand:#1e8a4c;--red:#d93f3f;--orange:#e98a1f;--ok:#2f9e5a;--blue:#2b6cb0;--soft:#eef1f5}
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
.btn{display:inline-flex;align-items:center;gap:8px;border:1px solid var(--line);background:#fff;border-radius:8px;padding:9px 14px;font:inherit;font-weight:600;cursor:pointer}
.btn.dark{background:var(--navy);color:#fff;border-color:var(--navy)}
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
.dot{width:8px;height:8px;border-radius:50%;margin-top:5px;flex:none}
.pill{font-size:11px;font-weight:700;padding:3px 8px;border-radius:999px;letter-spacing:.4px}
.pill.on{background:#e3f4ea;color:var(--ok)}.pill.off{background:#eceef2;color:var(--muted)}
.result{display:flex;align-items:center;gap:14px;margin:14px 0;padding:14px;border-radius:10px;background:var(--soft)}
.result .ic{width:46px;height:46px;border-radius:10px;display:grid;place-items:center;color:#fff;font-size:22px;font-weight:700}
.result .lbl{font-size:22px;font-weight:700}
.bar{height:10px;background:var(--soft);border-radius:6px;overflow:hidden;margin:6px 0 12px}
.bar i{display:block;height:100%;border-radius:6px;transition:width .4s}
.bl{display:flex;justify-content:space-between;font-size:13px;font-weight:500}
.sens{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin:10px 0 14px}
.sens div{background:var(--soft);border-radius:8px;padding:8px 10px;font-size:12px;color:var(--muted)}
.sens b{display:block;color:var(--text);font-size:13px}
.ctl{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.legend{display:flex;gap:14px;flex-wrap:wrap;font-size:12px;color:var(--muted);margin-top:6px}
.legend span{display:inline-flex;align-items:center;gap:6px}.legend b{color:var(--text)}
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
@media (max-width:900px){
 .side{display:none}.cols{grid-template-columns:1fr}.mapwrap{grid-template-columns:1fr;height:auto}#map{height:380px}
 .mtabs{display:flex!important}.user{display:none}main{padding:16px}
}
.mtabs{display:none;gap:6px;margin-bottom:14px}.mtabs a{flex:1;text-align:center}
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
    <div class="sp"></div>
    <div class="cw"><b>&#128337; Collection window</b>Route trucks dispatch daily at 8:00 AM and 1:00 PM. Full bins logged after 1:00 PM roll to the next window.</div>
  </aside>
  <main>
    <div class="mtabs"><a class="btn" href="#dashboard">Dashboard</a><a class="btn" href="#map">Map View</a></div>
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

        <!-- LIVE SMART BIN (prototype hardware) -->
        <div class="card" id="liveCard">
          <div class="kh"><b style="color:var(--text);font-size:15px">Live smart bin</b><span class="pill off" id="livePill">OFFLINE</span></div>
          <div class="note">BIN-CBD-001 &middot; City Plaza &middot; AI waste sorter prototype</div>
          <div class="result">
            <div class="ic" id="resIc" style="background:#9aa4b2">?</div>
            <div><div class="lbl" id="resLbl">Waiting for trash</div><div class="note" id="resNote">The camera classifies each item in about 10 seconds</div></div>
          </div>
          <div class="note" style="margin-bottom:10px">Status: <b id="stateTxt" style="color:var(--text)">&mdash;</b></div>
          <div class="bl"><span>&#9851; Plastic compartment</span><span id="fpTxt">0%</span></div>
          <div class="bar"><i id="fpBar" style="width:0;background:var(--blue)"></i></div>
          <div class="bl"><span>&#127807; Biodegradable compartment</span><span id="fbTxt">0%</span></div>
          <div class="bar"><i id="fbBar" style="width:0;background:var(--ok)"></i></div>
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
            <span><i class="dot" style="background:var(--red);margin:0"></i><b>FULL</b> 85% and above threshold</span>
            <span><i class="dot" style="background:var(--orange);margin:0"></i><b>NEARING FULL</b> 70% and above</span>
            <span><i class="dot" style="background:var(--ok);margin:0"></i><b>NORMAL</b> Below 70%</span>
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
  </main>
</div>

<script>
const TH_FULL=85, TH_NEAR=70;
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
const $=id=>document.getElementById(id);
const esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const stOf=f=>f>=TH_FULL?'full':f>=TH_NEAR?'near':'ok';
const stColor={full:'var(--red)',near:'var(--orange)',ok:'var(--ok)'};
let S=null, lastOk=0, online=false, map=null, markers={}, mapTried=false;

$('subline').textContent='City Environment and Natural Resources Office · '+
  new Date().toLocaleDateString('en-US',{weekday:'long',month:'long',day:'numeric'})+' · alert threshold '+TH_FULL+'%';

// ----- navigation (#dashboard / #map) -----
function route(){
  const v=location.hash==='#map'?'map':'dashboard';
  document.querySelectorAll('.view').forEach(e=>e.classList.toggle('on',e.id==='view-'+v));
  document.querySelectorAll('.nav').forEach(e=>e.classList.toggle('on',e.dataset.v===v));
  if(v==='map'){ initMap(); if(map) setTimeout(()=>map.invalidateSize(),50); }
}
addEventListener('hashchange',route);

// ----- live data -----
async function refresh(){
  try{
    const r=await fetch('/api/status',{cache:'no-store'});
    S=await r.json(); lastOk=Date.now(); online=true;
  }catch(e){ online=false; }
  render();
}
function evTime(t){ return new Date(Date.now()-(S.uptime-t)*1000).toLocaleTimeString([],{hour:'numeric',minute:'2-digit'}); }
function ago(t){ const s=Math.max(0,S.uptime-t); return s<60?s+'s ago':Math.floor(s/60)+' min ago'; }

function render(){
  $('offline').style.display=online||!lastOk?'none':'block';
  const live=BINS[0];
  if(S){ live.fill=Math.max(S.fill.plastic,S.fill.bio); }
  const full=BINS.filter(b=>b.fill>=TH_FULL);
  $('kFull').textContent=full.length;
  $('kFullNote').textContent='At or above the alert threshold. '+(full.some(b=>b.live)?'Includes the live smart bin — schedule a pickup.':'Live smart bin is below the threshold.');
  const active=11+(online?1:0);
  $('kActive').textContent=active;
  $('kActiveNote').textContent=online?'All bins reporting · live bin connected':'Live smart bin not reachable';

  // live card
  $('livePill').textContent=online?'LIVE':'OFFLINE';
  $('livePill').className='pill '+(online?'on':'off');
  if(S){
    const it=S.last.item, M={PLASTIC:['Plastic','var(--blue)','P'],BIO:['Biodegradable','var(--ok)','B'],
      EMPTY:['Nothing detected','#9aa4b2','–'],UNKNOWN:['Not sure','var(--orange)','?'],TIMEOUT:['Camera did not answer','var(--red)','!']};
    const m=M[it]||['Waiting for trash','#9aa4b2','?'];
    $('resLbl').textContent=m[0]; $('resIc').style.background=m[1]; $('resIc').textContent=m[2];
    $('resNote').textContent=it?('Detected '+ago(S.last.at)+(S.last.ms?' · AI took '+(S.last.ms/1000).toFixed(1)+' s':'')):'The camera classifies each item in about 10 seconds';
    const ST={idle:'Waiting for trash',detected:'Trash detected on the lid',classifying:'AI is classifying the item (about 10 s)…',tilting:'Tilting the lid'};
    $('stateTxt').textContent=ST[S.state]||S.state;
    $('fpTxt').textContent=S.fill.plastic+'%'; $('fpBar').style.width=S.fill.plastic+'%';
    $('fbTxt').textContent=S.fill.bio+'%'; $('fbBar').style.width=S.fill.bio+'%';
    $('fpBar').style.background=S.fill.plastic>=TH_FULL?'var(--red)':'var(--blue)';
    $('fbBar').style.background=S.fill.bio>=TH_FULL?'var(--red)':'var(--ok)';
    $('cP').textContent=S.counts.plastic; $('cB').textContent=S.counts.bio;
    $('sIr').textContent=S.ir?'Object on lid':'Clear';
    $('sCam').textContent={online:'Online',waiting:'Waiting for first item','no reply':'No reply'}[S.camera]||S.camera;
    $('sServo').textContent=S.servo+'° '+(S.state==='tilting'?'(moving)':'(at rest)');
    $('sWifi').textContent=S.rssi+' dBm';
    document.querySelectorAll('[data-a]').forEach(b=>b.disabled=!online||S.state!=='idle');
    // alerts
    const alerts=S.events.filter(e=>e.type==='alert'||e.type==='warn');
    $('kAlerts').textContent=alerts.length; $('bellN').textContent=alerts.length;
    const dotC={alert:'var(--red)',warn:'var(--orange)',sorted:'var(--blue)',info:'var(--ok)'};
    $('alerts').innerHTML=S.events.length?S.events.slice(0,8).map(e=>
      '<div class="al"><i class="dot" style="background:'+(dotC[e.type]||'var(--muted)')+'"></i><div class="t">'+evTime(e.t)+'</div><div class="m">'+esc(e.msg)+'</div></div>').join('')
      :'<div class="note">No events yet. Put trash on the lid to start.</div>';
  } else {
    document.querySelectorAll('[data-a]').forEach(b=>b.disabled=true);
  }
  renderMapData();
}

// ----- buttons -----
document.querySelectorAll('[data-a]').forEach(b=>b.onclick=async()=>{
  b.disabled=true;
  try{ await fetch('/api/action?do='+b.dataset.a,{method:'POST'}); }catch(e){}
  setTimeout(refresh,300);
});

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

route(); refresh(); setInterval(refresh,1500);
</script>
</body></html>)rawliteral";
