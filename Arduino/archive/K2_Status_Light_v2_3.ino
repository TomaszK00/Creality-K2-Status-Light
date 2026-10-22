#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Update.h>

// K2 Status Light v2.3 – FatGiantCat
// Set only after confirming the physical strip wiring. Never connect 24 V to ESP32/LEDs.
#define LED_PIN 4
#define LED_COUNT 24 // physical maximum; select 12 or 24 from the panel
#define STATUS_LED_PIN 8 // ESP32-C3 SuperMini onboard blue LED (active LOW)
#define LED_ENABLED true
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
int activeLedCount = 24, pulsePeriodMs = 1400;
String pulseMode = "off"; // off, all, last
uint32_t lastPulseFrame = 0;
bool simLoop = false;
String printerState = "unknown", printerFile = "";
float printerProgress = 0;
bool printerConnected = false;
String lastError = "Waiting for first poll";
uint32_t lastPoll = 0, lastSimulationStep = 0;
bool apMode = false, simEnabled = false, simRun = false;
int simStatus = 0, simProgress = 0;
String updateError = "";

const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>K2 Status Light · FatGiantCat</title>
<style>
:root{font-family:Inter,system-ui,-apple-system,Segoe UI,sans-serif;color-scheme:light;--bg:#f5f7f6;--panel:#fff;--field:#fff;--border:#dce3df;--text:#181d1a;--muted:#65736b;--green:#20b461}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text)}main{max-width:1100px;margin:auto;padding:24px 18px 44px}header,.headbar{display:flex;justify-content:space-between;align-items:center;gap:14px;flex-wrap:wrap}header{margin-bottom:24px;padding:12px 0;border-bottom:1px solid var(--border)}h1{font-size:clamp(22px,3vw,30px);margin:0;font-weight:700}h1 span,.brand{color:var(--green)}h2{font-size:17px;margin:0 0 17px}p{margin:9px 0}.muted,small{font-size:13px;color:var(--muted)}.card{background:var(--panel);border:1px solid var(--border);border-radius:12px;padding:22px;margin-bottom:17px}.cols{display:grid;grid-template-columns:1fr 1fr;gap:17px}.chips{display:flex;gap:8px;flex-wrap:wrap;margin-top:10px}.chip{background:#edf5f0;padding:6px 11px;border-radius:6px;font-size:13px}.chip.ok{color:#137a43}.chip.error{color:#b53737}label{display:block;margin:13px 0 6px;color:#26362c;font-size:13px}input,select,button{font:inherit}input:not([type=color]):not([type=range]):not([type=checkbox]):not([type=file]),select{display:block;width:100%;max-width:340px;background:var(--field);border:1px solid #b8c7be;color:#18241c;border-radius:7px;padding:8px 10px;min-height:39px}input[type=number]{max-width:130px!important}#ip{max-width:205px}#ssid,#pass{max-width:300px}#wifiList{max-width:310px}input[type=color]{width:60px;height:37px;border:1px solid #96a79b;border-radius:7px;cursor:pointer;background:var(--field);padding:2px}input[type=range]{width:100%;accent-color:var(--green)}.fields{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:4px 18px}.colorrow{display:grid;grid-template-columns:minmax(110px,1fr) 60px 80px;gap:8px;align-items:center;margin:8px 0}.colorrow label{margin:0}.colorcode{font:12px monospace;color:var(--muted)}button{border:1px solid var(--green);border-radius:7px;background:var(--green);color:#fff;font-weight:700;padding:9px 14px;cursor:pointer}button.alt{background:#edf5f0;color:#245536;border-color:#bfd4c6}button.ghost{background:transparent;color:#356344;border-color:#a9c7b2}button.active{border-color:var(--green);color:#147a42;background:#e3f6e9}button:disabled{opacity:.5}.langs{display:flex;gap:5px;flex-wrap:wrap}.langs button{padding:6px 9px;font-size:13px}.bar{display:grid;grid-template-columns:repeat(12,minmax(0,1fr));gap:7px;margin:18px 0 9px}.pair{display:grid;grid-template-columns:1fr 1fr;gap:3px}.led{height:28px;border-radius:4px;background:#e3ebe5;transition:background .12s}.pct{font-size:34px;font-weight:750}.controls{display:flex;flex-wrap:wrap;gap:8px;margin-top:14px}.notice{min-height:20px;color:#16864a;font-size:13px}.switch{display:flex;align-items:center;gap:9px;margin:12px 0}.switch label{margin:0}.switch input{width:18px;height:18px;accent-color:var(--green)}hr{border:0;border-top:1px solid var(--border);margin:19px 0}.spaced{margin-top:15px}.scanrow{display:flex;gap:8px;align-items:end}.scanrow select{flex:1}.warning{color:#8e5b19}footer{text-align:center;color:var(--muted);font-size:13px;margin-top:24px}input[type=file]{display:block;max-width:100%;margin:12px 0}@media(max-width:750px){.cols{grid-template-columns:1fr}main{padding:12px}.card{padding:16px}.fields{grid-template-columns:1fr}.bar{gap:3px}.led{height:22px}.colorrow{grid-template-columns:minmax(105px,1fr) 60px 75px}}
</style></head><body><main>
<header><div><h1><span>K2</span> Status Light</h1><div class="muted">FatGiantCat · ESP32-C3 · v2.3</div></div><nav class="langs" aria-label="Language"><button type="button" id="en" onclick="setLang('en')">🇬🇧 EN</button><button type="button" id="pl" onclick="setLang('pl')">🇵🇱 PL</button><button type="button" id="es" onclick="setLang('es')">🇪🇸 ES</button><button type="button" id="de" onclick="setLang('de')">🇩🇪 DE</button></nav></header>
<section class="card"><div class="headbar"><h2 data-i="preview">LED preview and test</h2><span id="network" class="chip">…</span></div><div class="chips"><span id="connection" class="chip">…</span><span id="displaystate" class="chip">…</span><span id="file" class="chip"></span></div><div class="headbar spaced"><span class="pct" id="percent">0%</span><span class="muted" id="ledcount">0 / 12</span></div><div class="bar" id="bar"></div><div id="err" class="muted"></div><hr>
<div class="switch"><input type="checkbox" id="sim" onchange="simToggle()"><label for="sim" data-i="testMode">Test mode (overrides printer)</label></div><div class="fields"><div><label for="testStatus" data-i="testStatus">Test state</label><select id="testStatus" onchange="sendSimulation()"><option value="printing" data-i="printing">Printing</option><option value="heating" data-i="heating">Heating</option><option value="paused" data-i="paused">Paused</option><option value="complete" data-i="complete">Completed</option><option value="error" data-i="error">Error</option><option value="standby" data-i="standby">Idle</option></select></div><div><label for="manualProgress"><span data-i="testProgress">Test progress</span>: <span id="manualPct">0%</span></label><input type="range" min="0" max="100" value="0" id="manualProgress" oninput="document.getElementById('manualPct').textContent=this.value+'%';sendSimulation()"></div></div>
<div class="switch"><input type="checkbox" id="simLoop" onchange="sendSimulation()"><label for="simLoop" data-i="loopTest">Repeat test after 100%</label></div><div class="controls"><button type="button" onclick="startRun()" data-i="run">▶ Run test</button><button type="button" class="alt" onclick="stopRun()" data-i="stop">■ Stop</button><button type="button" class="ghost" onclick="resetSim()" data-i="reset">Reset</button></div><p class="muted" data-i="simHint">The test also controls connected LEDs.</p><p id="simMessage" class="notice"></p></section>
<div class="cols"><section class="card"><h2 data-i="networkSettings">Network and printer</h2><form id="networkForm"><label for="wifiList" data-i="scanWifi">Available 2.4 GHz Wi-Fi</label><div class="scanrow"><select id="wifiList" onchange="if(this.value)document.getElementById('ssid').value=this.value"><option data-i="scanFirst">Scan first</option></select><button type="button" class="alt" onclick="scanWifi()" data-i="scan">Scan</button></div><label for="ssid" data-i="wifiName">Wi-Fi name (SSID)</label><input id="ssid" name="ssid" maxlength="32"><label for="pass" data-i="wifiPass">Wi-Fi password (blank keeps saved)</label><input id="pass" name="pass" type="password" autocomplete="new-password"><label for="ip" data-i="printerIP">Printer IP address</label><input id="ip" name="ip" required><label for="port">Moonraker port</label><input id="port" name="port" type="number" min="1" max="65535" required><div class="controls"><button data-i="saveNetwork" type="submit">Save network settings</button></div><p id="netMessage" class="notice"></p></form></section>
<section class="card"><h2 data-i="lighting">Colours and LED settings</h2><form id="lightForm"><div id="colors"></div><div class="fields"><div><label for="ledCount" data-i="ledCount">Physical LEDs</label><select name="ledCount" id="ledCount"><option value="24">24 LED · 12 pairs</option><option value="12">12 LED · 6 pairs</option></select></div><div><label for="bright" data-i="brightness">Brightness (%)</label><input type="number" name="bright" id="bright" min="1" max="100"></div><div><label for="interval" data-i="stepTime">Test step (ms)</label><input type="number" name="interval" id="interval" min="100" max="10000" step="50"></div><div><label for="pulseMode" data-i="pulseMode">Pulsing mode</label><select name="pulseMode" id="pulseMode"><option value="off" data-i="pulseOff">Off</option><option value="all" data-i="pulseAll">All lit LEDs</option><option value="last" data-i="pulseLast">Last lit pair during printing</option></select></div><div><label for="pulsePeriod" data-i="pulsePeriod">Pulse cycle (ms)</label><input type="number" name="pulsePeriod" id="pulsePeriod" min="200" max="10000" step="100"></div></div><p class="muted" data-i="powerHint">Check LED power wiring before increasing brightness.</p><button type="submit" data-i="saveLights">Save LED settings</button><p id="lightMessage" class="notice"></p></form></section></div>
<section class="card"><h2 data-i="updateTitle">Firmware update · OTA</h2><p class="muted" data-i="updateHint">Select the compiled ESP32-C3 .bin file. Keep power connected during upload.</p><form id="fwform"><input required id="bin" type="file" accept=".bin,application/octet-stream"><button type="submit" data-i="upload">Upload and restart</button></form><p id="fwMessage" class="notice"></p><p class="muted warning" data-i="security">The panel and OTA have no login. Use only on a trusted local network; do not expose it to the Internet.</p></section>
<footer><span data-i="credits">Designed by</span> <strong>FatGiantCat</strong> · K2 Status Light v2.3</footer>
</main><script>
const T={
en:{preview:'LED preview and test',testMode:'Test mode (overrides printer)',testStatus:'Test state',testProgress:'Test progress',printing:'Printing',heating:'Heating',paused:'Paused',complete:'Completed',error:'Error',standby:'Idle',run:'▶ Run test',stop:'■ Stop',reset:'Reset',loopTest:'Repeat test after 100%',simHint:'The test also controls connected LEDs.',networkSettings:'Network and printer',scanWifi:'Available 2.4 GHz Wi-Fi',scanFirst:'Scan first',scan:'Scan',wifiName:'Wi-Fi name (SSID)',wifiPass:'Wi-Fi password (blank keeps saved)',printerIP:'Printer IP address',saveNetwork:'Save network settings',lighting:'Colours and LED settings',ledCount:'Physical LEDs',brightness:'Brightness (%)',stepTime:'Test step (ms)',pulseMode:'Pulsing mode',pulseOff:'Off',pulseAll:'All lit LEDs',pulseLast:'Last lit pair during printing',pulsePeriod:'Pulse cycle (ms)',powerHint:'Check LED power wiring before increasing brightness.',saveLights:'Save LED settings',updateTitle:'Firmware update · OTA',updateHint:'Select the compiled ESP32-C3 .bin file. Keep power connected during upload.',upload:'Upload and restart',security:'The panel and OTA have no login. Use only on a trusted local network; do not expose it to the Internet.',credits:'Designed by',progressColor:'Print progress',colour_printing:'Printing',colour_heating:'Heating',colour_paused:'Paused',colour_complete:'Finished',colour_error:'Error',colour_standby:'Idle',saved:'Settings saved',scanError:'Scan failed',updateDone:'Firmware uploaded; device restarting…',updateFailed:'Upload failed',offline:'Printer offline',connected:'Printer connected',setup:'Setup access point',home:'Local Wi-Fi',test:'TEST',uploadConfirm:'Upload firmware and restart?',chooseBin:'Select a .bin file',scanning:'Scanning…'},
pl:{preview:'Podgląd i test LED',testMode:'Tryb testowy (zastępuje drukarkę)',testStatus:'Stan testowy',testProgress:'Postęp testu',printing:'Drukowanie',heating:'Nagrzewanie',paused:'Pauza',complete:'Zakończono',error:'Błąd',standby:'Bezczynność',run:'▶ Uruchom test',stop:'■ Zatrzymaj',reset:'Resetuj',loopTest:'Powtarzaj test po 100%',simHint:'Test steruje także podłączonym paskiem LED.',networkSettings:'Sieć i drukarka',scanWifi:'Dostępne Wi-Fi 2,4 GHz',scanFirst:'Najpierw skanuj',scan:'Skanuj',wifiName:'Nazwa Wi-Fi (SSID)',wifiPass:'Hasło Wi-Fi (puste zachowuje zapisane)',printerIP:'Adres IP drukarki',saveNetwork:'Zapisz ustawienia sieci',lighting:'Kolory i ustawienia LED',ledCount:'Fizyczne diody LED',brightness:'Jasność (%)',stepTime:'Krok testu (ms)',pulseMode:'Tryb pulsowania',pulseOff:'Wyłączone',pulseAll:'Wszystkie zapalone diody',pulseLast:'Ostatnia zapalona para podczas druku',pulsePeriod:'Cykl pulsowania (ms)',powerHint:'Sprawdź zasilanie LED przed zwiększeniem jasności.',saveLights:'Zapisz ustawienia LED',updateTitle:'Aktualizacja oprogramowania · OTA',updateHint:'Wybierz skompilowany plik .bin dla ESP32-C3. Nie odłączaj zasilania.',upload:'Wgraj i uruchom ponownie',security:'Panel i OTA nie mają logowania. Używaj tylko w zaufanej sieci lokalnej; nie udostępniaj w Internecie.',credits:'Projekt:',progressColor:'Postęp druku',colour_printing:'Drukowanie',colour_heating:'Nagrzewanie',colour_paused:'Pauza',colour_complete:'Zakończono',colour_error:'Błąd',colour_standby:'Bezczynność',saved:'Zapisano ustawienia',scanError:'Błąd skanowania',updateDone:'Wgrano firmware; urządzenie uruchamia się ponownie…',updateFailed:'Błąd aktualizacji',offline:'Brak połączenia z drukarką',connected:'Połączono z drukarką',setup:'Sieć konfiguracyjna',home:'Domowe Wi-Fi',test:'TEST',uploadConfirm:'Wgrać firmware i uruchomić ponownie?',chooseBin:'Wybierz plik .bin',scanning:'Skanowanie…'},
es:{preview:'Vista previa y prueba LED',testMode:'Modo de prueba (sustituye la impresora)',testStatus:'Estado de prueba',testProgress:'Progreso de prueba',printing:'Imprimiendo',heating:'Calentando',paused:'En pausa',complete:'Finalizado',error:'Error',standby:'En espera',run:'▶ Iniciar prueba',stop:'■ Detener',reset:'Restablecer',loopTest:'Repetir al llegar al 100%',simHint:'La prueba también controla los LED conectados.',networkSettings:'Red e impresora',scanWifi:'Redes Wi-Fi de 2,4 GHz',scanFirst:'Escanear primero',scan:'Escanear',wifiName:'Nombre Wi-Fi (SSID)',wifiPass:'Contraseña Wi-Fi (vacío conserva la guardada)',printerIP:'Dirección IP de la impresora',saveNetwork:'Guardar ajustes de red',lighting:'Colores y ajustes LED',ledCount:'LED físicos',brightness:'Brillo (%)',stepTime:'Paso de prueba (ms)',pulseMode:'Modo de pulsación',pulseOff:'Desactivado',pulseAll:'Todos los LED encendidos',pulseLast:'Último par encendido al imprimir',pulsePeriod:'Ciclo de pulsación (ms)',powerHint:'Comprueba la alimentación antes de aumentar el brillo.',saveLights:'Guardar ajustes LED',updateTitle:'Actualizar firmware · OTA',updateHint:'Selecciona un archivo .bin compilado para ESP32-C3. Mantén la alimentación.',upload:'Subir y reiniciar',security:'El panel y OTA no tienen contraseña. Úsalos solo en una red local de confianza; no los expongas a Internet.',credits:'Diseñado por',progressColor:'Progreso de impresión',colour_printing:'Imprimiendo',colour_heating:'Calentando',colour_paused:'En pausa',colour_complete:'Finalizado',colour_error:'Error',colour_standby:'En espera',saved:'Ajustes guardados',scanError:'Error al escanear',updateDone:'Firmware cargado; reiniciando…',updateFailed:'Error al subir',offline:'Impresora desconectada',connected:'Impresora conectada',setup:'Punto de acceso',home:'Wi-Fi local',test:'PRUEBA',uploadConfirm:'¿Subir firmware y reiniciar?',chooseBin:'Selecciona un archivo .bin',scanning:'Escaneando…'},
de:{preview:'LED-Vorschau und Test',testMode:'Testmodus (ersetzt Druckerdaten)',testStatus:'Teststatus',testProgress:'Testfortschritt',printing:'Druckt',heating:'Heizt',paused:'Pausiert',complete:'Abgeschlossen',error:'Fehler',standby:'Bereit',run:'▶ Test starten',stop:'■ Stoppen',reset:'Zurücksetzen',loopTest:'Test nach 100 % wiederholen',simHint:'Der Test steuert auch die angeschlossenen LEDs.',networkSettings:'Netzwerk und Drucker',scanWifi:'Verfügbare 2,4-GHz-WLANs',scanFirst:'Zuerst suchen',scan:'Suchen',wifiName:'WLAN-Name (SSID)',wifiPass:'WLAN-Passwort (leer lässt gespeichert)',printerIP:'IP-Adresse des Druckers',saveNetwork:'Netzwerk speichern',lighting:'Farben und LED-Einstellungen',ledCount:'Physische LEDs',brightness:'Helligkeit (%)',stepTime:'Testschritt (ms)',pulseMode:'Pulsiermodus',pulseOff:'Aus',pulseAll:'Alle leuchtenden LEDs',pulseLast:'Letztes leuchtendes Paar beim Drucken',pulsePeriod:'Pulszyklus (ms)',powerHint:'LED-Stromversorgung vor höherer Helligkeit prüfen.',saveLights:'LED-Einstellungen speichern',updateTitle:'Firmware-Update · OTA',updateHint:'Kompilierte ESP32-C3-.bin-Datei auswählen. Stromversorgung nicht trennen.',upload:'Hochladen und neu starten',security:'Panel und OTA haben kein Login. Nur in einem vertrauenswürdigen lokalen Netz verwenden; nicht im Internet freigeben.',credits:'Entwickelt von',progressColor:'Druckfortschritt',colour_printing:'Druckt',colour_heating:'Heizt',colour_paused:'Pausiert',colour_complete:'Fertig',colour_error:'Fehler',colour_standby:'Bereit',saved:'Einstellungen gespeichert',scanError:'Suche fehlgeschlagen',updateDone:'Firmware hochgeladen; Neustart…',updateFailed:'Upload fehlgeschlagen',offline:'Drucker offline',connected:'Drucker verbunden',setup:'Einrichtungsnetz',home:'Lokales WLAN',test:'TEST',uploadConfirm:'Firmware hochladen und neu starten?',chooseBin:'.bin-Datei auswählen',scanning:'Suche läuft…'}};
let lang='en',initialized=false,lastStatus={},simBusy=false,simQueued=null;
function setLang(l){if(!T[l])l='en';lang=l;document.documentElement.lang=l;localStorage.setItem('k2lang',l);document.querySelectorAll('[data-i]').forEach(e=>{e.textContent=T[l][e.dataset.i]||T.en[e.dataset.i]||e.textContent});for(const id of Object.keys(T).filter(x=>['en','pl','es','de'].includes(x)))document.getElementById(id).classList.toggle('active',l===id);paintStatus(lastStatus)}
const cols=document.getElementById('colors');for(const [id,key] of [['progress','progressColor'],['printing','colour_printing'],['heating','colour_heating'],['paused','colour_paused'],['complete','colour_complete'],['error','colour_error'],['standby','colour_standby']]){const r=document.createElement('div');r.className='colorrow';r.innerHTML=`<label data-i="${key}" for="c_${id}"></label><input type="color" id="c_${id}" name="c_${id}"><span class="colorcode" id="v_${id}"></span>`;cols.append(r);const inp=r.querySelector('input');inp.addEventListener('input',()=>{document.getElementById('v_'+id).textContent=inp.value;paintStatus(lastStatus)})}
function buildBar(pairs){const bar=document.getElementById('bar');if(bar.children.length===pairs)return;bar.replaceChildren();bar.style.gridTemplateColumns=`repeat(${pairs},minmax(0,1fr))`;for(let i=0;i<pairs;i++){const pair=document.createElement('div');pair.className='pair';for(let j=0;j<2;j++){const led=document.createElement('div');led.className='led';pair.append(led)}bar.append(pair)}}
async function api(path,options){const r=await fetch(path,options);if(!r.ok)throw Error(await r.text());return r.json()}
function paintStatus(s){if(!s||!Object.keys(s).length)return;const state=s.sim?s.simState:(s.connected?s.state:'standby'),prog=s.sim?s.simProgress:s.progress*100,pairs=s.ledCount/2,count=state==='printing'?Math.ceil(prog*pairs/100):(state==='standby'?0:pairs);buildBar(pairs);const input=document.getElementById('c_'+(state==='printing'?'progress':state)),color=input?.value||s.colors[state]||'#00c8f0';document.getElementById('percent').textContent=Math.round(prog)+'%';document.getElementById('ledcount').textContent=count+' / '+pairs;document.getElementById('connection').textContent=s.connected?T[lang].connected:T[lang].offline;document.getElementById('connection').className='chip '+(s.connected?'ok':'error');document.getElementById('network').textContent=s.ap?T[lang].setup:T[lang].home;document.getElementById('displaystate').textContent=(s.sim?T[lang].test+' · ':'')+(T[lang][state]||state);document.getElementById('file').textContent=s.filename||'';document.getElementById('file').style.display=s.filename?'':'none';document.getElementById('err').textContent=s.sim?'':(s.error||'');const period=Math.max(200,s.pulsePeriod||1400),wave=(1-Math.cos(2*Math.PI*(Date.now()%period)/period))/2,factor=.25+.75*wave;[...document.querySelectorAll('.pair')].forEach((p,i)=>[...p.children].forEach(led=>{const pulse=i<count&&(s.pulseMode==='all'||(s.pulseMode==='last'&&state==='printing'&&i===count-1));led.style.background=i<count?color:'#e3ebe5';led.style.opacity=pulse?factor:1;led.style.boxShadow=i<count?'0 0 7px '+color+'40':'none'}))}
async function refresh(){try{const s=await api('/api/status');lastStatus=s;if(!initialized){for(const [id,v] of Object.entries({ssid:s.ssid,ip:s.ip,port:s.port,bright:s.brightness,interval:s.interval,ledCount:s.ledCount,pulseMode:s.pulseMode,pulsePeriod:s.pulsePeriod}))document.getElementById(id).value=v;for(const id of ['progress','printing','heating','paused','complete','error','standby']){document.getElementById('c_'+id).value=s.colors[id];document.getElementById('v_'+id).textContent=s.colors[id]}document.getElementById('testStatus').value=s.simState;document.getElementById('simLoop').checked=s.simLoop;initialized=true}document.getElementById('manualProgress').value=s.simProgress;document.getElementById('manualPct').textContent=s.simProgress+'%';document.getElementById('sim').checked=s.sim;paintStatus(s)}catch(e){document.getElementById('err').textContent=e.message}}
async function postForm(path,form){const r=await fetch(path,{method:'POST',body:new URLSearchParams(new FormData(form))});if(!r.ok)throw Error(await r.text());return r.text()}
document.getElementById('networkForm').onsubmit=async e=>{e.preventDefault();try{document.getElementById('netMessage').textContent=await postForm('/api/network',e.target)}catch(err){document.getElementById('netMessage').textContent=err.message}};
document.getElementById('lightForm').onsubmit=async e=>{e.preventDefault();try{await postForm('/api/lighting',e.target);document.getElementById('lightMessage').textContent=T[lang].saved;await refresh()}catch(err){document.getElementById('lightMessage').textContent=err.message}};
async function scanWifi(){const list=document.getElementById('wifiList');list.replaceChildren(new Option(T[lang].scanning,''));try{const s=await api('/api/wifi/scan');list.replaceChildren(new Option('—',''));s.networks.forEach(n=>list.add(new Option(n.ssid+' ('+n.rssi+' dBm) '+(n.secure?'🔒':''),n.ssid)))}catch(e){list.replaceChildren(new Option(T[lang].scanError,''))}}
async function sendSimulation(extra={}){simQueued={...(simQueued||{}),...extra};if(simBusy)return;simBusy=true;try{while(simQueued){const action=simQueued;simQueued=null;const p=new URLSearchParams({enabled:document.getElementById('sim').checked?'1':'0',status:document.getElementById('testStatus').value,progress:document.getElementById('manualProgress').value,loop:document.getElementById('simLoop').checked?'1':'0',...action});await api('/api/simulation',{method:'POST',body:p});await refresh()}}catch(e){document.getElementById('simMessage').textContent=e.message}finally{simBusy=false}}
function simToggle(){sendSimulation({run:'0'})}function startRun(){document.getElementById('sim').checked=true;document.getElementById('testStatus').value='printing';document.getElementById('manualProgress').value=0;sendSimulation({run:'1',progress:'0'})}function stopRun(){sendSimulation({run:'0'})}function resetSim(){document.getElementById('manualProgress').value=0;sendSimulation({run:'0',progress:'0'})}
document.getElementById('fwform').onsubmit=async e=>{e.preventDefault();const file=document.getElementById('bin').files[0];if(!file||!file.name.toLowerCase().endsWith('.bin')){document.getElementById('fwMessage').textContent=T[lang].chooseBin;return}if(!confirm(T[lang].uploadConfirm))return;const f=new FormData();f.append('firmware',file,file.name);document.getElementById('fwMessage').textContent='…';try{const r=await fetch('/api/update',{method:'POST',body:f});if(!r.ok)throw Error(await r.text());document.getElementById('fwMessage').textContent=T[lang].updateDone}catch(err){document.getElementById('fwMessage').textContent=T[lang].updateFailed+': '+err.message}};
setLang(T[localStorage.getItem('k2lang')]?localStorage.getItem('k2lang'):'en');refresh();setInterval(refresh,1200);setInterval(()=>paintStatus(lastStatus),70);
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
  activeLedCount=prefs.getInt("ledcnt",24)==12?12:24;
  pulsePeriodMs=constrain(prefs.getInt("pulsems",1400),200,10000);
  pulseMode=prefs.getString("pulsemode","off");if(pulseMode!="all"&&pulseMode!="last")pulseMode="off";
  for(int i=0;i<6;i++)stateColors[i]=normalizeColor(prefs.getString((String("c")+i).c_str(),stateColors[i]),stateColors[i]);
  prefs.end();
}
void saveNetwork(){prefs.begin("k2led",false);prefs.putString("ssid",ssid);prefs.putString("pass",wifiPassword);prefs.putString("ip",printerIP);prefs.putUShort("port",printerPort);prefs.end();}
void saveLighting(){prefs.begin("k2led",false);prefs.putString("color",progressColor);prefs.putInt("bright",brightness);prefs.putInt("intvl",intervalMs);prefs.putInt("ledcnt",activeLedCount);prefs.putInt("pulsems",pulsePeriodMs);prefs.putString("pulsemode",pulseMode);for(int i=0;i<6;i++)prefs.putString((String("c")+i).c_str(),stateColors[i]);prefs.end();}
int statusIndex(String st){if(st=="printing")return 0;if(st=="heating")return 1;if(st=="paused")return 2;if(st=="complete")return 3;if(st=="error")return 4;return 5;}
String normalState(String s){s.toLowerCase();if(s=="complete"||s=="completed")return "complete";if(s=="printing")return "printing";if(s=="paused")return "paused";if(s=="error"||s=="cancelled")return "error";if(s=="heating")return "heating";return "standby";}
void updateLEDs(){
  if(!LED_ENABLED)return;
  String st=simEnabled?String(stateNames[simStatus]):normalState(printerState);
  if(!simEnabled&&!printerConnected)st="standby";
  int pairs=activeLedCount/2;
  int count=st=="printing"?(int)ceilf((simEnabled?simProgress:printerProgress*100.0f)*pairs/100.0f):(st=="standby"?0:pairs);
  count=constrain(count,0,pairs);
  uint32_t rgb=color24(st=="printing"?progressColor:stateColors[statusIndex(st)]);
  uint8_t r=(rgb>>16)&255,g=(rgb>>8)&255,b=rgb&255;
  // A smooth pulse from 25% to 100% of the selected brightness.
  float phase=6.2831853f*(millis()%pulsePeriodMs)/(float)pulsePeriodMs;
  float scale=0.625f-0.375f*cosf(phase);
  strip.setBrightness(map(brightness,0,100,0,255));
  for(int i=0;i<LED_COUNT;i++){
    int pair=i/2;
    bool lit=i<activeLedCount&&pair<count;
    bool pulse=lit&&(pulseMode=="all"||(pulseMode=="last"&&st=="printing"&&pair==count-1));
    float level=pulse?scale:1.0f;
    strip.setPixelColor(i,lit?strip.Color((uint8_t)(r*level),(uint8_t)(g*level),(uint8_t)(b*level),0):strip.Color(0,0,0,0));
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
void jsonReply(int code, JsonDocument& d){String s;serializeJson(d,s);server.send(code,"application/json",s);}
void routes(){
  server.on("/",HTTP_GET,[](){server.send_P(200,"text/html; charset=utf-8",PAGE);});
  server.on("/api/status",HTTP_GET,[](){JsonDocument d;d["connected"]=printerConnected;d["ap"]=apMode;d["state"]=normalState(printerState);d["progress"]=printerProgress;d["filename"]=printerFile;d["error"]=lastError;d["ssid"]=ssid;d["ip"]=printerIP;d["port"]=printerPort;d["brightness"]=brightness;d["interval"]=intervalMs;d["ledCount"]=activeLedCount;d["pulseMode"]=pulseMode;d["pulsePeriod"]=pulsePeriodMs;d["simLoop"]=simLoop;d["sim"]=simEnabled;d["simState"]=stateNames[simStatus];d["simProgress"]=simProgress;d["simRunning"]=simRun;d["colors"]["progress"]=progressColor;for(int i=0;i<6;i++)d["colors"][stateNames[i]]=stateColors[i];jsonReply(200,d);});
  server.on("/api/wifi/scan",HTTP_GET,[](){int n=WiFi.scanNetworks(false,true);JsonDocument d;JsonArray a=d["networks"].to<JsonArray>();for(int i=0;i<n && i<35;i++){String name=WiFi.SSID(i);if(name.isEmpty())continue;JsonObject net=a.add<JsonObject>();net["ssid"]=name;net["rssi"]=WiFi.RSSI(i);net["secure"]=WiFi.encryptionType(i)!=WIFI_AUTH_OPEN;}WiFi.scanDelete();jsonReply(200,d);});
  server.on("/api/network",HTTP_POST,[](){String newSSID=server.arg("ssid"),newIP=server.arg("ip");newSSID.trim();newIP.trim();int port=server.arg("port").toInt();IPAddress parsed;if(!parsed.fromString(newIP)||port<1||port>65535||newSSID.length()>32){server.send(400,"text/plain","Invalid IP, port or SSID");return;}bool changed=(ssid!=newSSID||server.arg("pass").length()>0);ssid=newSSID;printerIP=newIP;printerPort=port;if(server.arg("pass").length())wifiPassword=server.arg("pass");saveNetwork();server.send(200,"text/plain; charset=utf-8",changed?"Saved. Press RESET to reconnect to the new Wi-Fi. / Zapisano. Naciśnij RESET.":"Saved / Zapisano");});
  server.on("/api/lighting",HTTP_POST,[](){
    int b=server.arg("bright").toInt(),t=server.arg("interval").toInt();
    int leds=server.arg("ledCount").toInt(),period=server.arg("pulsePeriod").toInt();
    String mode=server.arg("pulseMode");
    if(b<1||b>100||t<100||t>10000||(leds!=12&&leds!=24)||period<200||period>10000||(mode!="off"&&mode!="all"&&mode!="last")){server.send(400,"text/plain","Invalid LED settings");return;}
    String c=normalizeColor(server.arg("c_progress"),"");if(c.isEmpty()){server.send(400,"text/plain","Invalid progress colour");return;}
    String next[6];for(int i=0;i<6;i++){next[i]=normalizeColor(server.arg(String("c_")+stateNames[i]),"");if(next[i].isEmpty()){server.send(400,"text/plain","Invalid colour");return;}}
    brightness=b;intervalMs=t;activeLedCount=leds;pulseMode=mode;pulsePeriodMs=period;progressColor=c;
    for(int i=0;i<6;i++)stateColors[i]=next[i];saveLighting();updateLEDs();server.send(200,"text/plain","Saved");
  });
  server.on("/api/simulation",HTTP_POST,[](){simEnabled=server.arg("enabled")=="1";simStatus=statusIndex(server.arg("status"));simProgress=constrain(server.arg("progress").toInt(),0,100);simLoop=server.arg("loop")=="1";if(server.hasArg("run")){simRun=simEnabled&&server.arg("run")=="1";lastSimulationStep=millis();}if(!simEnabled)simRun=false;updateLEDs();JsonDocument d;d["ok"]=true;jsonReply(200,d);});
  server.on("/api/update",HTTP_POST,[](){bool ok=!Update.hasError()&&updateError.isEmpty();server.send(ok?200:500,"text/plain",ok?"OK":("Update failed: "+updateError));delay(350);if(ok)ESP.restart();},[](){HTTPUpload& upload=server.upload();if(upload.status==UPLOAD_FILE_START){updateError="";if(!upload.filename.endsWith(".bin")){updateError="Expected .bin";return;}if(!Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH))updateError=Update.errorString();}else if(upload.status==UPLOAD_FILE_WRITE){if(updateError.isEmpty()&&Update.write(upload.buf,upload.currentSize)!=upload.currentSize)updateError=Update.errorString();}else if(upload.status==UPLOAD_FILE_END){if(updateError.isEmpty()&&!Update.end(true))updateError=Update.errorString();}else if(upload.status==UPLOAD_FILE_ABORTED){Update.abort();updateError="Aborted";}});
}
void setup(){pinMode(STATUS_LED_PIN,OUTPUT);digitalWrite(STATUS_LED_PIN,HIGH);Serial.begin(115200);delay(450);loadSettings();if(LED_ENABLED){strip.begin();strip.clear();strip.show();}startNetwork();routes();server.begin();Serial.println("K2 Status Light v2.3 web server ready");}
void loop(){
  server.handleClient();
  uint32_t now=millis();
  // Two short blue flashes, then a longer pause; onboard LED is active LOW.
  uint32_t beat=now%1000;
  digitalWrite(STATUS_LED_PIN,(beat<100||(beat>=220&&beat<320))?LOW:HIGH);
  if(now-lastPoll>2200){lastPoll=now;pollPrinter();if(!simEnabled)updateLEDs();}
  if(simRun&&now-lastSimulationStep>=(uint32_t)intervalMs){
    lastSimulationStep=now;
    if(simProgress>=100){if(simLoop)simProgress=0;else simRun=false;}
    else {simProgress=min(100,simProgress+9);if(simProgress>=100&&!simLoop)simRun=false;}
    updateLEDs();
  }
  if(pulseMode!="off"&&now-lastPulseFrame>=40){lastPulseFrame=now;updateLEDs();}
  delay(2);
}

