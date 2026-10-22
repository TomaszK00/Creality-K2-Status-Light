#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

WebServer server(80);
Preferences prefs;
String ssid, wifiPassword, printerIP;
uint16_t printerPort = 7125;
String progressColor = "#00c8f0";
int brightness = 65;
String printerState = "unknown", printerFile = "";
float printerProgress = 0;
bool printerConnected = false;
String lastError = "Not polled yet";
unsigned long lastPoll = 0;
const char* AP_NAME = "K2-LED-Setup";
const char* AP_PASSWORD = "k2ledsetup"; // Change before regular use.
bool apMode = false;

const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="pl"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>K2 Status Light</title><style>
:root{color-scheme:dark;font-family:system-ui,Arial}body{background:#101827;color:#eef5fa;max-width:640px;margin:24px auto;padding:0 16px}section{background:#1e293b;padding:18px;border-radius:14px;margin-bottom:16px}h1{font-size:24px}h2{font-size:18px;margin-top:0}input,button{font:inherit;padding:11px;border-radius:8px;border:1px solid #526078}input{background:#0e1726;color:white;box-sizing:border-box;width:100%;margin:5px 0 12px}button{background:#05b6d3;color:#03131c;border:0;font-weight:bold;cursor:pointer}label{font-size:14px}.bar{display:grid;grid-template-columns:repeat(12,1fr);gap:5px;margin:20px 0}.seg{height:24px;border-radius:5px;background:#354254}small{color:#a5b4c6}strong{font-size:26px}#message{min-height:1em;color:#a7edbc}
</style></head><body><h1>Creality K2 · Status Light</h1><section><h2>Drukarka</h2><div id="conn">Odczyt...</div><p><strong id="pct">--%</strong> · <span id="state">---</span></p><div class="bar" id="bar"></div><div id="file"></div><small id="error"></small></section>
<section><h2>Ustawienia</h2><form id="form"><label>SSID Wi-Fi (2.4 GHz)</label><input id="ssid" name="ssid" placeholder="Nazwa domowego Wi-Fi"><label>Hasło Wi-Fi (pozostaw puste, aby nie zmieniać)</label><input id="pass" name="pass" type="password" autocomplete="new-password"><label>IP drukarki K2</label><input id="ip" name="ip" required><label>Port Moonraker</label><input id="port" name="port" type="number" min="1" max="65535" required><label>Kolor postępu</label><input id="color" name="color" type="color"><label>Jasność LED (%) — do użycia po podłączeniu paska</label><input id="bright" name="bright" type="number" min="1" max="100"><button type="submit">Zapisz ustawienia</button></form><p id="message"></p></section><small>Firmware v2.0 · odczyt Moonraker tylko do odczytu. Panel działa lokalnie.</small>
<script>
let initialized=false;const bar=document.getElementById('bar');for(let i=0;i<12;i++){const el=document.createElement('div');el.className='seg';bar.append(el)}
async function refresh(){try{const r=await fetch('/api/status');const s=await r.json();document.getElementById('conn').textContent=(s.connected?'● Połączono z K2':'○ Brak połączenia z K2')+' · '+s.network;document.getElementById('pct').textContent=s.connected?Math.round(s.progress*100)+'%':'--%';document.getElementById('state').textContent=s.state;document.getElementById('file').textContent=s.filename;document.getElementById('error').textContent=s.error||'';const count=Math.ceil(s.progress*12);[...bar.children].forEach((e,i)=>e.style.background=(s.connected&&i<count?s.color:'#354254'));if(!initialized){document.getElementById('ssid').value=s.ssid;document.getElementById('ip').value=s.ip;document.getElementById('port').value=s.port;document.getElementById('color').value=s.color;document.getElementById('bright').value=s.brightness;initialized=true}}catch(e){document.getElementById('conn').textContent='Brak komunikacji ze sterownikiem'}}
document.getElementById('form').addEventListener('submit',async e=>{e.preventDefault();const data=new URLSearchParams(new FormData(e.target));const r=await fetch('/api/settings',{method:'POST',body:data});document.getElementById('message').textContent=await r.text();});refresh();setInterval(refresh,2000);
</script></body></html>
)HTML";

void saveSettings() {
  prefs.begin("k2led", false);
  prefs.putString("ssid", ssid); prefs.putString("pass", wifiPassword);
  prefs.putString("ip", printerIP); prefs.putUShort("port", printerPort);
  prefs.putString("color", progressColor); prefs.putInt("bright", brightness);
  prefs.end();
}
void loadSettings() {
  prefs.begin("k2led", true);
  ssid = prefs.getString("ssid", ""); wifiPassword = prefs.getString("pass", "");
  printerIP = prefs.getString("ip", "192.168.1.9"); printerPort = prefs.getUShort("port", 7125);
  progressColor = prefs.getString("color", "#00c8f0"); brightness = prefs.getInt("bright", 65);
  prefs.end();
}
void startNetwork() {
  WiFi.mode(WIFI_STA);
  if(ssid.length()) {
    WiFi.begin(ssid.c_str(), wifiPassword.c_str());
    Serial.print("Connecting to Wi-Fi");
    for(int i=0;i<40 && WiFi.status()!=WL_CONNECTED;i++){delay(250);Serial.print(".");}
  }
  if(WiFi.status()==WL_CONNECTED) {
    Serial.println("\nConnected: http://"+WiFi.localIP().toString());
    if(MDNS.begin("k2-led")) Serial.println("mDNS: http://k2-led.local");
  } else {
    apMode=true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_NAME, AP_PASSWORD);
    Serial.println("\nSetup AP: K2-LED-Setup / password: k2ledsetup");
    Serial.println("Open http://192.168.4.1");
  }
}
void pollPrinter() {
  if(apMode || WiFi.status()!=WL_CONNECTED) { printerConnected=false;lastError="ESP not connected to home Wi-Fi";return; }
  WiFiClient client;
  HTTPClient http;
  String url="http://"+printerIP+":"+String(printerPort)+"/printer/objects/query?print_stats&virtual_sdcard";
  http.setTimeout(1500);
  if(!http.begin(client,url)) {printerConnected=false;lastError="Invalid URL";return;}
  int code=http.GET();
  if(code!=200){printerConnected=false;lastError="Moonraker HTTP "+String(code);http.end();return;}
  // Parse while response stream is available to avoid a large temporary String.
  JsonDocument doc;
  DeserializationError err=deserializeJson(doc,http.getStream());
  http.end();
  if(err){printerConnected=false;lastError=String("JSON: ")+err.c_str();return;}
  JsonVariant status=doc["result"]["status"];
  if(status["print_stats"].isNull() || status["virtual_sdcard"].isNull()){
    printerConnected=false;lastError="Missing print_stats or virtual_sdcard";return;
  }
  printerState=status["print_stats"]["state"].as<String>();
  printerFile=status["print_stats"]["filename"].as<String>();
  printerProgress=constrain(status["virtual_sdcard"]["progress"].as<float>(),0.0f,1.0f);
  printerConnected=true;lastError="";
}
void setupRoutes(){
  server.on("/", HTTP_GET, [](){server.send_P(200,"text/html; charset=utf-8",PAGE);});
  server.on("/api/status", HTTP_GET, [](){
    JsonDocument doc;
    doc["connected"]=printerConnected;doc["network"]=apMode?"Setup AP":"Home Wi-Fi";
    doc["state"]=printerState;doc["progress"]=printerProgress;doc["filename"]=printerFile;
    doc["error"]=lastError;doc["ssid"]=ssid;doc["ip"]=printerIP;doc["port"]=printerPort;
    doc["color"]=progressColor;doc["brightness"]=brightness;
    String out;serializeJson(doc,out);server.send(200,"application/json",out);
  });
  server.on("/api/settings", HTTP_POST, [](){
    String newSSID=server.arg("ssid");newSSID.trim();
    String newIP=server.arg("ip");newIP.trim();
    int newPort=server.arg("port").toInt();int newBrightness=server.arg("bright").toInt();
    String newColor=server.arg("color");
    IPAddress parsed;
    if(!parsed.fromString(newIP)||newPort<1||newPort>65535||newBrightness<1||newBrightness>100||newColor.length()!=7||newColor[0]!='#'){
      server.send(400,"text/plain; charset=utf-8","Niepoprawne IP, port, kolor lub jasność");return;
    }
    bool newWiFi=(ssid!=newSSID || server.arg("pass").length());
    ssid=newSSID; printerIP=newIP;printerPort=newPort;brightness=newBrightness;progressColor=newColor;
    if(server.arg("pass").length())wifiPassword=server.arg("pass");
    saveSettings();
    server.send(200,"text/plain; charset=utf-8",newWiFi?"Zapisano. Zrestartuj ESP przyciskiem RESET, aby połączyć z nowym Wi-Fi.":"Zapisano ustawienia.");
    if(newWiFi)Serial.println("New Wi-Fi saved. Press RESET to reconnect.");
  });
}
void setup(){Serial.begin(115200);delay(500);loadSettings();startNetwork();setupRoutes();server.begin();Serial.println("Web server ready");}
void loop(){server.handleClient();if(millis()-lastPoll>=2000){lastPoll=millis();pollPrinter();}delay(2);}
