// web_dashboard.cpp — ESP32 SoftAP + WebServer dashboard (built-in libs only).
// Routes: / = dashboard page (auto-refresh via fetch), /data = JSON snapshot.
// Non-blocking: handleClient() each loop; JSON built with snprintf (no heap JSON lib).
#include "web_dashboard.h"
#include <WiFi.h>
#include <WebServer.h>

static WebServer server(80);
static WebSnapshot latest{{0, 0, 0, false, false}, ST_NORMAL, "--", false, false,
                          false, false, false, 0.0f, NAN, 0};
static bool webOk = false;

const char *webWordFor(StatusLevel st, const Vitals &v, bool sos) {
  if (sos) return "SOS-HELP!";
  if (st == ST_CRITICAL) return "ALERT";
  if (st == ST_WARNING) return "CHECK NEEDED";
  if (v.hrValid || v.tempValid) return "FEELING WELL";
  return "MEASURING...";
}

static const char *webActionFor(StatusLevel st, const Vitals &v, bool sos) {
  if (sos) return "HELP! Caregiver called!";
  if (st == ST_CRITICAL) return "Sit down, call caregiver now!";
  if (st == ST_WARNING) return "Please rest, recheck soon.";
  if (v.hrValid || v.tempValid) return "You are OK. Rest.";
  return "Keep finger still, wait 4s.";
}

static const char *webColorFor(StatusLevel st, bool sos) {
  if (sos || st == ST_CRITICAL) return "#d32f2f";  // red
  if (st == ST_WARNING) return "#f9a825";          // yellow
  return "#2e7d32";                                // green
}

// Dashboard: big word + digits always visible (defense needs values on screen;
// WORDS_ONLY still respected on Serial/TFT — web is the presentable mirror).
static const char PAGE[] PROGMEM = R"HTML(<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>PROTECT Monitor</title>
<style>body{font-family:system-ui,Arial,sans-serif;margin:0;background:#111;color:#fff;text-align:center}
.top{padding:10px;font-size:18px}.word{font-size:52px;font-weight:800;margin:8px;padding:24px;border-radius:12px}
.act{font-size:20px;margin:8px}.grid{display:flex;gap:10px;justify-content:center;flex-wrap:wrap;margin:12px}
.card{background:#222;border-radius:10px;padding:14px 22px;min-width:110px}.card b{font-size:34px;display:block}
.sys{color:#aaa;font-size:14px;margin:10px}.leg{font-size:13px;color:#ccc;background:#1b1b1b;padding:10px;margin:12px;border-radius:8px;text-align:left;max-width:560px;margin-left:auto;margin-right:auto}</style>
</head><body><div class="top">P.R.O.T.E.C.T. Live — <span id="clk">--</span></div>
<div class="word" id="word">MEASURING...</div><div class="act" id="act">Starting...</div>
<div class="grid"><div class="card">HR<b id="hr">--</b>bpm</div>
<div class="card">SpO2<b id="spo">--</b>%</div><div class="card">Temp<b id="tp">--</b>&deg;C</div></div>
<div class="sys" id="sys">Time:-- Fall:-- SMS:--</div>
<div class="leg"><b>Legend (real data only):</b><br>FEELING WELL: HR 50-100, Oxy 94-100%, Temp 35.5-37.3<br>
CHECK NEEDED: HR 101-120, Oxy 93%, Temp 37.3-38.0<br>ALERT: HR &lt;50 or &gt;120, Oxy &lt;93%, Temp &lt;35.5 or &gt;38.0, fall, SOS<br>
MEASURING: no finger / warming up — never a status without data</div>
<script>setInterval(async()=>{try{const r=await fetch('/data');const j=await r.json();
document.getElementById('clk').textContent=j.clock;
const w=document.getElementById('word');w.textContent=j.word;w.style.background=j.color;
document.getElementById('act').textContent=j.action;
document.getElementById('hr').textContent=j.hrValid?j.hr:'--';
document.getElementById('spo').textContent=j.hrValid?j.spo2:'--';
document.getElementById('tp').textContent=j.tempValid?j.tempC.toFixed(1):'--';
document.getElementById('sys').textContent='Time:'+(j.rtcOk?'OK':'--')+' Fall:'+(j.mpuOk?j.accelG.toFixed(2)+'g':'--')+' SMS:'+(j.gsmOk?'OK':'--')+(j.sos?' SOS':'')+(j.fall?' FALL':'');
}catch(e){}},2000);</script></body></html>)HTML";

static void handleRoot() { server.send_P(200, "text/html", PAGE); }

static void handleData() {
  char buf[400];
  snprintf(buf, sizeof(buf),
           "{\"hr\":%.0f,\"spo2\":%.0f,\"tempC\":%.1f,\"hrValid\":%d,\"tempValid\":%d,"
           "\"st\":%d,\"word\":\"%s\",\"action\":\"%s\",\"color\":\"%s\",\"clock\":\"%s\","
           "\"rtcOk\":%d,\"mpuOk\":%d,\"gsmOk\":%d,\"accelG\":%.2f,\"sos\":%d,\"fall\":%d}",
           latest.v.hr, latest.v.spo2, latest.v.tempC,
           latest.v.hrValid ? 1 : 0, latest.v.tempValid ? 1 : 0, (int)latest.st,
           webWordFor(latest.st, latest.v, latest.sos),
           webActionFor(latest.st, latest.v, latest.sos),
           webColorFor(latest.st, latest.sos), latest.clockStr,
           latest.rtcOk ? 1 : 0, latest.mpuOk ? 1 : 0, latest.gsmOk ? 1 : 0,
           latest.accelG, latest.sos ? 1 : 0, latest.fall ? 1 : 0);
  server.send(200, "application/json", buf);
}

void webBegin() {
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL, 0, WIFI_AP_MAXCONN);
  webOk = ok;
  if (!ok) {
    Serial.println(F("[WEB] softAP start FAILED — Serial/SD/SMS continue, web disabled."));
    return;
  }
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.print(F("[WEB] AP up: "));
  Serial.print(WIFI_AP_SSID);
  Serial.print(F(" | open http://"));
  Serial.print(WiFi.softAPIP());
  Serial.println(F(" on phone/laptop (AP password: protect123)"));
}

void webHandle() {
  if (webOk) server.handleClient();
}

void webUpdate(const WebSnapshot &s) { latest = s; }
