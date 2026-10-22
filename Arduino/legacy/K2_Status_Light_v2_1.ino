#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Update.h>

// K2 Status Light v2.1 – FatGiantCat
// Set only after confirming the physical strip wiring. Never connect 24 V to ESP32/LEDs.
#define LED_PIN 4
#define LED_COUNT 24
#define LED_ENABLED true
// Browser authentication for configuration and firmware uploads.
// CHANGE THIS password before sharing the firmware or using on any untrusted LAN.
const char* ADMIN_USER = "admin";
const char* ADMIN_PASSWORD = "REPLACE_WITH_YOUR_OWN_PASSWORD";
const char* AP_NAME = "K2-LED-Setup";
const char* AP_PASSWORD = "k2ledsetup"; // Change before using in public settings.

WebServer server(80);
Preferences prefs;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);
String ssid, wifiPassword, printerIP;
uint16_t printerPort = 7125;
String progressColor = "#00c8f0";
String stateColors[6] = {"#00c8f0", "#ff9e35", "#ffce45", "#2fe182", "#ff445c", "#ffffff"};
const char* const stateNames[6] = {"printing", "heating", "paused", "complete", "error", "standby"};
int brightness = 65;
int intervalMs = 500;
String printerState = "unknown", printerFile = "";
float printerProgress = 0;
bool printerConnected = false;
String lastError = "Waiting for first poll";
uint32_t lastPoll = 0, lastSimulationStep = 0;
bool apMode = false, simEnabled = false, simRun = false;
int simStatus = 0, simProgress = 0;
String updateError = "";

const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>FatGiantCat · K2 Status Light</title>
<style>
:root{font-family:system-ui,-apple-system,Segoe UI,sans-serif;color-scheme:dark}*{box-sizing:border-box}body{margin:0;background:#0c1320;color:#ebf3fd}main{max-width:1230px;margin:auto;padding:24px 18px 40px}header{display:flex;justify-content:space-between;align-items:center;gap:14px;margin-bottom:20px}h1{font-size:clamp(22px,3vw,30px);margin:0}h2{font-size:17px;margin:0 0 17px}h3{font-size:14px;color:#bdd0df;margin:18px 0 10px}p{margin:7px 0}small,.muted{color:#9aacc2}.card{background:#172338;border:1px solid #2b3b51;box-shadow:0 6px 23px #0002;border-radius:17px;padding:20px;margin-bottom:18px}.cols{display:grid;grid-template-columns:1fr 1fr;gap:18px}.chips{display:flex;gap:8px;flex-wrap:wrap;align-items:center}.chip{background:#26354a;color:#c8dae9;padding:7px 12px;border-radius:24px;font-size:13px}.chip.ok{background:#123b36;color:#7df0c2}.muted{font-size:13px}label{display:block;margin:12px 0 6px;color:#c3d3e4;font-size:13px}input,select,button{font:inherit}input:not([type=color]):not([type=range]),select{width:100%;background:#0d192a;border:1px solid #435572;color:white;border-radius:9px;padding:10px;min-height:42px}input[type=color]{width:66px;height:40px;border:2px solid #687c99;border-radius:8px;cursor:pointer;background:transparent;padding:3px}input[type=range]{width:100%;accent-color:#00c8f0}.cfields{display:grid;grid-template-columns:1fr 1fr;gap:12px}.colorrow{display:grid;grid-template-columns:1fr 68px 97px;gap:8px;align-items:center;margin:9px 0}.colorrow label{margin:0}.colorcode{font-size:12px;color:#bad0e0;font-family:monospace}button{border:0;border-radius:9px;background:#13c4e7;color:#071727;font-weight:700;padding:10px 16px;cursor:pointer}button.alt{background:#33465e;color:#ecf6fa}button.ghost{background:transparent;color:#cde9f5;border:1px solid #597189}button.active{outline:2px solid #f7fafe}button:disabled{opacity:.48;cursor:not-allowed}.langs button{padding:8px 12px}.bar{display:grid;grid-template-columns:repeat(12,minmax(0,1fr));gap:7px;margin:20px 0 8px}.pair{display:grid;grid-template-columns:1fr 1fr;gap:3px}.led{height:28px;border-radius:5px;background:#293547;transition:background .12s;box-shadow:inset 0 0 0 1px #ffffff13}.pct{font-size:32px;font-weight:750}.headbar{display:flex;justify-content:space-between;gap:12px;align-items:center}.controls{display:flex;flex-wrap:wrap;gap:9px;margin-top:13px}.notice{min-height:20px;color:#84e8a7;font-size:13px}.danger{color:#ff9aa7}footer{text-align:center;color:#9badc3;margin-top:24px;font-size:13px}input[type=file]{display:block;margin:12px 0;max-width:100%}.spaced{margin-top:18px}.switch{display:flex;align-items:center;gap:9px}.switch input{width:18px;height:18px}hr{border:0;border-top:1px solid #34455a;margin:18px 0}@media(max-width:820px){.cols{grid-template-columns:1fr}.cfields{grid-template-columns:1fr}main{padding:14px 12px}.card{padding:15px}.led{height:23px}.bar{gap:4px}}
</style></head><body><main>
<header><div><h1>FatGiantCat <span style="color:#21d1ed">K2 Status Light</span></h1><div class="muted">ESP32-C3 · firmware v2.1</div></div><div class="langs"><button id="en" class="alt" onclick="setLang('en')">🇬🇧 EN</button> <button id="pl" class="alt" onclick="setLang('pl')">🇵🇱 PL</button></div></header>
<section class="card"><div class="headbar"><h2 data-i="preview">LED simulator & live preview</h2><span id="network" class="chip">...</span></div><div class="chips"><span id="connection" class="chip">...</span><span id="displaystate" class="chip">...</span><span class="chip" id="file"></span></div><div class="headbar spaced"><span class="pct" id="percent">0%</span><span class="muted" id="ledcount">0 / 12</span></div><div class="bar" id="bar"></div><div id="err" class="muted"></div><hr>
<div class="switch"><input type="checkbox" id="sim" onchange="simToggle()"><label for="sim" data-i="testMode">Simulation mode (overrides printer data)</label></div>
<div class="cfields"><div><label data-i="testStatus">Simulated status</label><select id="testStatus" onchange="sendSimulation()"><option value="printing" data-i="printing">Printing</option><option value="heating" data-i="heating">Heating</option><option value="paused" data-i="paused">Paused</option><option value="complete" data-i="complete">Completed</option><option value="error" data-i="error">Error</option><option value="standby" data-i="standby">Idle</option></select></div><div><label><span data-i="testProgress">Simulation progress</span>: <span id="manualPct">0%</span></label><input type="range" min="0" max="100" value="0" id="manualProgress" oninput="document.getElementById('manualPct').textContent=this.value+'%';sendSimulation()"></div></div>
<div class="controls"><button onclick="startRun()" id="startBtn" data-i="run">▶ Run 12 pairs</button><button class="alt" onclick="stopRun()" data-i="stop">■ Stop</button><button class="ghost" onclick="resetSim()" data-i="reset">Reset</button></div><p class="muted" data-i="simHint">Each step lights two LEDs; the default step interval is 0.5 seconds. Changes are sent to the actual LED strip when wired.</p><p id="simMessage" class="notice"></p></section>
<div class="cols">
<section class="card"><h2 data-i="networkSettings">Network & printer</h2><form id="networkForm"><label data-i="scanWifi">Available Wi-Fi networks (2.4 GHz)</label><div style="display:flex;gap:8px"><select id="wifiList" onchange="if(this.value)document.getElementById('ssid').value=this.value"><option data-i="scanFirst">Click Scan</option></select><button type="button" class="alt" onclick="scanWifi()" data-i="scan">Scan</button></div><label data-i="wifiName">Wi-Fi network name (SSID)</label><input id="ssid" name="ssid" maxlength="32"><label data-i="wifiPass">Wi-Fi password (blank keeps saved password)</label><input id="pass" name="pass" type="password" autocomplete="new-password"><label data-i="printerIP">Printer IP address</label><input id="ip" name="ip" required><label>Moonraker port</label><input id="port" name="port" type="number" min="1" max="65535" required><button data-i="saveNetwork" type="submit">Save network settings</button><p id="netMessage" class="notice"></p></form></section>
<section class="card"><h2 data-i="lighting">Colours & LED settings</h2><form id="lightForm"><div id="colors"></div><div class="cfields"><div><label data-i="brightness">Brightness limit (%)</label><input type="number" name="bright" id="bright" min="1" max="100"></div><div><label data-i="stepTime">Progress simulation step (ms)</label><input type="number" name="interval" id="interval" min="100" max="10000" step="50"></div></div><p class="muted" data-i="powerHint">For physical LEDs, keep brightness low until power wiring is verified.</p><button data-i="saveLights" type="submit">Save lighting settings</button><p id="lightMessage" class="notice"></p></form></section></div>
<section class="card"><h2 data-i="updateTitle">Firmware update · OTA</h2><p class="muted" data-i="updateHint">Choose a compiled .bin firmware file. Do not upload .ino or .zip. Keep the device powered until reboot.</p><form id="fwform"><input required id="bin" type="file" accept=".bin,application/octet-stream"><button type="submit" data-i="upload">Upload firmware and restart</button></form><p id="fwMessage" class="notice"></p><p class="muted" data-i="security">This panel uses a local administrator password. Avoid exposing it to the Internet.</p></section>
<footer><span data-i="credits">Designed & developed by</span> <strong>FatGiantCat</strong> · K2 Status Light v2.1</footer>
</main><script>
const T={en:{preview:'LED simulator & live preview',testMode:'Simulation mode (overrides printer data)',testStatus:'Simulated status',testProgress:'Simulation progress',printing:'Printing',heating:'Heating',paused:'Paused',complete:'Completed',error:'Error',standby:'Idle',run:'▶ Run 12 pairs',stop:'■ Stop',reset:'Reset',simHint:'Each step lights two LEDs. Changes control the physical strip when it is connected.',networkSettings:'Network & printer',scanWifi:'Available Wi-Fi networks (2.4 GHz)',scanFirst:'Click Scan',scan:'Scan',wifiName:'Wi-Fi network name (SSID)',wifiPass:'Wi-Fi password (blank keeps saved password)',printerIP:'Printer IP address',saveNetwork:'Save network settings',lighting:'Colours & LED settings',brightness:'Brightness limit (%)',stepTime:'Progress simulation step (ms)',powerHint:'Keep brightness low until physical power wiring is verified.',saveLights:'Save lighting settings',updateTitle:'Firmware update · OTA',updateHint:'Choose a compiled .bin firmware file (not .ino/.zip). Do not disconnect power during upload.',upload:'Upload firmware and restart',security:'Local administrator password required. Do not expose the panel to the Internet.',credits:'Designed & developed by',progressColor:'Print progress',colour_printing:'Printing',colour_heating:'Heating',colour_paused:'Paused',colour_complete:'Finished',colour_error:'Error',colour_standby:'Idle',saved:'Settings saved',restart:'Wi-Fi changed: restart ESP32 to reconnect',scanError:'Scan failed',updateDone:'Firmware uploaded; device is restarting…',updateFailed:'Firmware upload failed',offline:'Printer offline',connected:'Printer connected',setup:'Setup access point',home:'Local Wi-Fi'},pl:{preview:'Symulator LED i podgląd na żywo',testMode:'Tryb symulacji (zastępuje dane drukarki)',testStatus:'Symulowany status',testProgress:'Postęp symulacji',printing:'Drukowanie',heating:'Nagrzewanie',paused:'Pauza',complete:'Zakończono',error:'Błąd',standby:'Bezczynność',run:'▶ Uruchom 12 par',stop:'■ Zatrzymaj',reset:'Resetuj',simHint:'Każdy krok zapala dwie diody. Po podłączeniu paska test steruje również prawdziwymi LED.',networkSettings:'Sieć i drukarka',scanWifi:'Dostępne sieci Wi-Fi (2,4 GHz)',scanFirst:'Kliknij Skanuj',scan:'Skanuj',wifiName:'Nazwa sieci Wi-Fi (SSID)',wifiPass:'Hasło Wi-Fi (puste = zachowaj zapisane)',printerIP:'Adres IP drukarki',saveNetwork:'Zapisz ustawienia sieci',lighting:'Kolory i ustawienia LED',brightness:'Limit jasności (%)',stepTime:'Krok postępu w symulacji (ms)',powerHint:'Ustaw niską jasność, zanim zweryfikujemy zasilanie fizycznego paska.',saveLights:'Zapisz ustawienia LED',updateTitle:'Aktualizacja oprogramowania · OTA',updateHint:'Wybierz skompilowany plik firmware .bin (nie .ino/.zip). Nie odłączaj zasilania podczas aktualizacji.',upload:'Wgraj firmware i uruchom ponownie',security:'Wymagane lokalne hasło administratora. Nie udostępniaj panelu przez Internet.',credits:'Projekt i oprogramowanie:',progressColor:'Postęp druku',colour_printing:'Drukowanie',colour_heating:'Nagrzewanie',colour_paused:'Pauza',colour_complete:'Zakończono',colour_error:'Błąd',colour_standby:'Bezczynność',saved:'Zapisano ustawienia',restart:'Zmieniono Wi-Fi: zrestartuj ESP32, aby połączyć się ponownie',scanError:'Błąd skanowania',updateDone:'Wgrano firmware; urządzenie uruchomi się ponownie…',updateFailed:'Błąd aktualizacji',offline:'Brak połączenia z drukarką',connected:'Połączono z drukarką',setup:'Sieć konfiguracyjna',home:'Domowe Wi-Fi'}};
let lang='en', initialized=false, lastStatus={}, simBusy=false;
function setLang(l){lang=l;document.documentElement.lang=l;localStorage.setItem('k2lang',l);document.querySelectorAll('[data-i]').forEach(e=>e.textContent=T[l][e.dataset.i]||e.textContent);document.getElementById('en').classList.toggle('active',l==='en');document.getElementById('pl').classList.toggle('active',l==='pl');paintStatus(lastStatus)}
const cols=document.getElementById('colors');for(const [id,key] of [['progress','progressColor'],['printing','colour_printing'],['heating','colour_heating'],['paused','colour_paused'],['complete','colour_complete'],['error','colour_error'],['standby','colour_standby']]){const r=document.createElement('div');r.className='colorrow';r.innerHTML=`<label data-i="${key}"></label><input type="color" id="c_${id}" name="c_${id}"><span class="colorcode" id="v_${id}"></span>`;cols.append(r);const inp=r.querySelector('input');inp.addEventListener('input',()=>{document.getElementById('v_'+id).textContent=inp.value;paintStatus(lastStatus)})}
for(let i=0;i<12;i++){const pair=document.createElement('div');pair.className='pair';for(let j=0;j<2;j++){const led=document.createElement('div');led.className='led';pair.append(led)}document.getElementById('bar').append(pair)}
async function api(path,options){const r=await fetch(path,options);if(!r.ok)throw new Error(await r.text());return r.json()}
function hexFor(status){const input=document.getElementById('c_'+(status==='printing'?'progress':status));return input?.value||'#00c8f0'}
function paintStatus(s){if(!s || !Object.keys(s).length)return;const state=s.sim?s.simState:s.state;const prog=s.sim?s.simProgress:s.progress*100;const count=(state==='printing')?Math.ceil(prog*12/100):(state==='standby'?0:12);const color=hexFor(state);document.getElementById('percent').textContent=Math.round(prog)+'%';document.getElementById('ledcount').textContent=count+' / 12';document.getElementById('connection').textContent=s.connected?T[lang].connected:T[lang].offline;document.getElementById('connection').className='chip '+(s.connected?'ok':'');document.getElementById('network').textContent=s.ap?T[lang].setup:T[lang].home;document.getElementById('displaystate').textContent=s.sim?'TEST · '+(T[lang][state]||state):(T[lang][state]||state);document.getElementById('file').textContent=s.filename||'';document.getElementById('err').textContent=s.sim?'':(s.error||'');[...document.querySelectorAll('.pair')].forEach((p,i)=>[...p.children].forEach(led=>{led.style.background=i<count?color:'#293547';led.style.boxShadow=i<count?'0 0 13px '+color+'70':'inset 0 0 0 1px #ffffff12'}))}
async function refresh(){try{const s=await api('/api/status');lastStatus=s;paintStatus(s);if(!initialized){document.getElementById('ssid').value=s.ssid;document.getElementById('ip').value=s.ip;document.getElementById('port').value=s.port;document.getElementById('bright').value=s.brightness;document.getElementById('interval').value=s.interval;document.getElementById('c_progress').value=s.colors.progress;for(const name of ['printing','heating','paused','complete','error','standby'])document.getElementById('c_'+name).value=s.colors[name];for(const id of ['progress','printing','heating','paused','complete','error','standby'])document.getElementById('v_'+id).textContent=document.getElementById('c_'+id).value;document.getElementById('sim').checked=s.sim;document.getElementById('testStatus').value=s.simState;document.getElementById('manualProgress').value=s.simProgress;initialized=true}document.getElementById('manualProgress').value=s.simProgress;document.getElementById('manualPct').textContent=s.simProgress+'%';document.getElementById('sim').checked=s.sim}catch(e){document.getElementById('err').textContent=e.message}}
async function postForm(path,form){const r=await fetch(path,{method:'POST',body:new URLSearchParams(new FormData(form))});if(!r.ok)throw Error(await r.text());return r.text()}
document.getElementById('networkForm').onsubmit=async e=>{e.preventDefault();try{const msg=await postForm('/api/network',e.target);document.getElementById('netMessage').textContent=msg}catch(err){document.getElementById('netMessage').textContent=err.message}};
document.getElementById('lightForm').onsubmit=async e=>{e.preventDefault();try{await postForm('/api/lighting',e.target);document.getElementById('lightMessage').textContent=T[lang].saved}catch(err){document.getElementById('lightMessage').textContent=err.message}};
async function scanWifi(){const list=document.getElementById('wifiList');list.innerHTML='<option>Scanning…</option>';try{const s=await api('/api/wifi/scan');list.innerHTML='<option value="">—</option>';s.networks.forEach(n=>{const o=document.createElement('option');o.value=n.ssid;o.textContent=n.ssid+' ('+n.rssi+' dBm) '+(n.secure?'🔒':'');list.append(o)})}catch(e){list.innerHTML='<option>'+T[lang].scanError+'</option>'}}
async function sendSimulation(extra={}){if(simBusy)return;simBusy=true;try{const p=new URLSearchParams({enabled:document.getElementById('sim').checked?'1':'0',status:document.getElementById('testStatus').value,progress:document.getElementById('manualProgress').value,...extra});await api('/api/simulation',{method:'POST',body:p});await refresh()}catch(e){document.getElementById('simMessage').textContent=e.message}finally{simBusy=false}}
function simToggle(){sendSimulation({run:'0'})}function startRun(){document.getElementById('sim').checked=true;document.getElementById('testStatus').value='printing';document.getElementById('manualProgress').value=0;sendSimulation({run:'1',progress:'0'})}function stopRun(){sendSimulation({run:'0'})}function resetSim(){document.getElementById('manualProgress').value=0;sendSimulation({run:'0',progress:'0'})}
document.getElementById('fwform').onsubmit=async e=>{e.preventDefault();const file=document.getElementById('bin').files[0];if(!file||!file.name.endsWith('.bin')){document.getElementById('fwMessage').textContent='Please choose .bin';return}if(!confirm('Upload firmware and restart? / Wgrać firmware i uruchomić ponownie?'))return;const f=new FormData();f.append('firmware',file,file.name);document.getElementById('fwMessage').textContent='Uploading…';try{const r=await fetch('/api/update',{method:'POST',body:f});if(!r.ok)throw Error(await r.text());document.getElementById('fwMessage').textContent=T[lang].updateDone}catch(err){document.getElementById('fwMessage').textContent=T[lang].updateFailed+': '+err.message}};
setLang(localStorage.getItem('k2lang')==='pl'?'pl':'en');refresh();setInterval(refresh,1200);
</script></body></html>
)HTML";

String normalizeColor(String c, const String& fallback) {
  if(c.length()!=7 || c[0]!='#')return fallback;
  for(int i=1;i<7;i++)if(!isxdigit((unsigned char)c[i]))return fallback;
  c.toLowerCase();return c;
}
uint32_t color24(const String& h) {return strtoul(h.c_str()+1,nullptr,16);}
void loadSettings(){
  prefs.begin("k2led",true);
  ssid=prefs.getString("ssid","");wifiPassword=prefs.getString("pass","");
  printerIP=prefs.getString("ip","192.168.1.9");printerPort=prefs.getUShort("port",7125);
  progressColor=normalizeColor(prefs.getString("color","#00c8f0"),"#00c8f0");brightness=constrain(prefs.getInt("bright",65),1,100);
  intervalMs=constrain(prefs.getInt("intvl",500),100,10000);
  for(int i=0;i<6;i++)stateColors[i]=normalizeColor(prefs.getString((String("c")+i).c_str(),stateColors[i]),stateColors[i]);
  prefs.end();
}
void saveNetwork(){prefs.begin("k2led",false);prefs.putString("ssid",ssid);prefs.putString("pass",wifiPassword);prefs.putString("ip",printerIP);prefs.putUShort("port",printerPort);prefs.end();}
void saveLighting(){prefs.begin("k2led",false);prefs.putString("color",progressColor);prefs.putInt("bright",brightness);prefs.putInt("intvl",intervalMs);for(int i=0;i<6;i++)prefs.putString((String("c")+i).c_str(),stateColors[i]);prefs.end();}
int statusIndex(String st){if(st=="printing")return 0;if(st=="heating")return 1;if(st=="paused")return 2;if(st=="complete")return 3;if(st=="error")return 4;return 5;}
String normalState(String s){s.toLowerCase();if(s=="complete"||s=="completed")return "complete";if(s=="printing")return "printing";if(s=="paused")return "paused";if(s=="error"||s=="cancelled")return "error";if(s=="heating")return "heating";return "standby";}
void updateLEDs(){
  if(!LED_ENABLED)return;
  String st=simEnabled?String(stateNames[simStatus]):normalState(printerState);
  int count=0;
  if(!simEnabled&&!printerConnected)st="standby";
  if(st=="printing")count=(int)ceilf((simEnabled?simProgress:printerProgress*100.0f)*12.0f/100.0f);
  else if(st!="standby")count=12;
  count=constrain(count,0,12);
  String hex=(st=="printing")?progressColor:stateColors[statusIndex(st)];
  uint32_t rgb=color24(hex);uint8_t r=(rgb>>16)&255,g=(rgb>>8)&255,b=rgb&255;
  strip.setBrightness(map(brightness,0,100,0,255));
  for(int i=0;i<LED_COUNT;i++){
    if(i/2<count)strip.setPixelColor(i,strip.Color(r,g,b,0));
    else strip.setPixelColor(i,strip.Color(0,0,0,0));
  }
  strip.show();
}
void startNetwork(){
  WiFi.mode(WIFI_STA);
  if(ssid.length()){
    WiFi.begin(ssid.c_str(),wifiPassword.c_str());
    Serial.print("Connecting WiFi");
    for(int i=0;i<40 && WiFi.status()!=WL_CONNECTED;i++){delay(250);Serial.print('.');}
  }
  if(WiFi.status()==WL_CONNECTED){Serial.println("\nPanel: http://"+WiFi.localIP().toString());if(MDNS.begin("k2-led"))Serial.println("mDNS: http://k2-led.local");}
  else{apMode=true;WiFi.mode(WIFI_AP_STA);WiFi.softAP(AP_NAME,AP_PASSWORD);Serial.println("\nSetup AP: K2-LED-Setup · http://192.168.4.1");}
}
void pollPrinter(){
  if(apMode || WiFi.status()!=WL_CONNECTED){printerConnected=false;lastError="No printer connection (setup AP)";return;}
  WiFiClient client;HTTPClient http;
  String url="http://"+printerIP+":"+String(printerPort)+"/printer/objects/query?print_stats&virtual_sdcard";
  http.setTimeout(1700);
  if(!http.begin(client,url)){printerConnected=false;lastError="Invalid Moonraker URL";return;}
  int response=http.GET();if(response!=200){printerConnected=false;lastError="HTTP "+String(response);http.end();return;}
  JsonDocument doc;DeserializationError err=deserializeJson(doc,http.getStream());http.end();
  if(err){printerConnected=false;lastError=String("JSON: ")+err.c_str();return;}
  JsonVariant status=doc["result"]["status"];
  if(status["print_stats"].isNull()||status["virtual_sdcard"].isNull()){printerConnected=false;lastError="Missing printer fields";return;}
  printerState=status["print_stats"]["state"].as<String>();printerFile=status["print_stats"]["filename"].as<String>();
  printerProgress=constrain(status["virtual_sdcard"]["progress"].as<float>(),0.f,1.f);
  printerConnected=true;lastError="";
}
bool auth(){if(server.authenticate(ADMIN_USER,ADMIN_PASSWORD))return true;server.requestAuthentication();return false;}
void jsonReply(int code, JsonDocument& d){String s;serializeJson(d,s);server.send(code,"application/json",s);}
void routes(){
  server.on("/",HTTP_GET,[](){if(!auth())return;server.send_P(200,"text/html; charset=utf-8",PAGE);});
  server.on("/api/status",HTTP_GET,[](){if(!auth())return;JsonDocument d;d["connected"]=printerConnected;d["ap"]=apMode;d["state"]=normalState(printerState);d["progress"]=printerProgress;d["filename"]=printerFile;d["error"]=lastError;d["ssid"]=ssid;d["ip"]=printerIP;d["port"]=printerPort;d["brightness"]=brightness;d["interval"]=intervalMs;d["sim"]=simEnabled;d["simState"]=stateNames[simStatus];d["simProgress"]=simProgress;d["simRunning"]=simRun;d["colors"]["progress"]=progressColor;for(int i=0;i<6;i++)d["colors"][stateNames[i]]=stateColors[i];jsonReply(200,d);});
  server.on("/api/wifi/scan",HTTP_GET,[](){if(!auth())return;int n=WiFi.scanNetworks(false,true);JsonDocument d;JsonArray a=d["networks"].to<JsonArray>();for(int i=0;i<n && i<35;i++){String name=WiFi.SSID(i);if(name.isEmpty())continue;JsonObject net=a.add<JsonObject>();net["ssid"]=name;net["rssi"]=WiFi.RSSI(i);net["secure"]=WiFi.encryptionType(i)!=WIFI_AUTH_OPEN;}WiFi.scanDelete();jsonReply(200,d);});
  server.on("/api/network",HTTP_POST,[](){if(!auth())return;String newSSID=server.arg("ssid"),newIP=server.arg("ip");newSSID.trim();newIP.trim();int port=server.arg("port").toInt();IPAddress parsed;if(!parsed.fromString(newIP)||port<1||port>65535||newSSID.length()>32){server.send(400,"text/plain","Invalid IP, port or SSID");return;}bool changed=(ssid!=newSSID||server.arg("pass").length()>0);ssid=newSSID;printerIP=newIP;printerPort=port;if(server.arg("pass").length())wifiPassword=server.arg("pass");saveNetwork();server.send(200,"text/plain; charset=utf-8",changed?"Saved. Press RESET to reconnect to the new Wi-Fi. / Zapisano. Naciśnij RESET.":"Saved / Zapisano");});
  server.on("/api/lighting",HTTP_POST,[](){if(!auth())return;int b=server.arg("bright").toInt(),t=server.arg("interval").toInt();if(b<1||b>100||t<100||t>10000){server.send(400,"text/plain","Invalid brightness or interval");return;}String c=normalizeColor(server.arg("c_progress"),"");if(c.isEmpty()){server.send(400,"text/plain","Invalid progress colour");return;}String next[6];for(int i=0;i<6;i++){next[i]=normalizeColor(server.arg(String("c_")+stateNames[i]),"");if(next[i].isEmpty()){server.send(400,"text/plain","Invalid colour");return;}}brightness=b;intervalMs=t;progressColor=c;for(int i=0;i<6;i++)stateColors[i]=next[i];saveLighting();updateLEDs();server.send(200,"text/plain","Saved");});
  server.on("/api/simulation",HTTP_POST,[](){if(!auth())return;simEnabled=server.arg("enabled")=="1";simStatus=statusIndex(server.arg("status"));simProgress=constrain(server.arg("progress").toInt(),0,100);if(server.hasArg("run")){simRun=simEnabled&&server.arg("run")=="1";lastSimulationStep=millis();}if(!simEnabled)simRun=false;updateLEDs();JsonDocument d;d["ok"]=true;jsonReply(200,d);});
  server.on("/api/update",HTTP_POST,[](){if(!auth())return;bool ok=!Update.hasError()&&updateError.isEmpty();server.send(ok?200:500,"text/plain",ok?"OK":("Update failed: "+updateError));delay(350);if(ok)ESP.restart();},[](){if(!server.authenticate(ADMIN_USER,ADMIN_PASSWORD))return;HTTPUpload& upload=server.upload();if(upload.status==UPLOAD_FILE_START){updateError="";if(!upload.filename.endsWith(".bin")){updateError="Expected .bin";return;}if(!Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH))updateError=Update.errorString();}else if(upload.status==UPLOAD_FILE_WRITE){if(updateError.isEmpty()&&Update.write(upload.buf,upload.currentSize)!=upload.currentSize)updateError=Update.errorString();}else if(upload.status==UPLOAD_FILE_END){if(updateError.isEmpty()&&!Update.end(true))updateError=Update.errorString();}else if(upload.status==UPLOAD_FILE_ABORTED){Update.abort();updateError="Aborted";}});
}
void setup(){Serial.begin(115200);delay(450);loadSettings();if(LED_ENABLED){strip.begin();strip.clear();strip.show();}startNetwork();routes();server.begin();Serial.println("K2 Status Light v2.1 web server ready");}
void loop(){server.handleClient();if(millis()-lastPoll>2200){lastPoll=millis();pollPrinter();if(!simEnabled)updateLEDs();}if(simRun&&millis()-lastSimulationStep>=(uint32_t)intervalMs){lastSimulationStep=millis();simProgress=min(100,simProgress+9);if(simProgress>=100)simRun=false;updateLEDs();}delay(2);}
