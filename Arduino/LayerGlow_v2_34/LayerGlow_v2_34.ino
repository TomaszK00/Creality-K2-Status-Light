#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "src/PubSubClient.h"
#include <time.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Update.h>
#include <esp_ota_ops.h>

// LayerGlow v2.34 – FatGiantCat
// Set only after confirming the physical strip wiring. Never connect 24 V to ESP32/LEDs.
#define LED_PIN 4
#define LED_COUNT 24 // default; configured total is 5..300 addressable RGBW LEDs
#define MAX_LED_COUNT 300
#define MAX_LED_PAIRS ((MAX_LED_COUNT+1)/2)
#define STATUS_LED_PIN 8 // ESP32-C3 SuperMini onboard blue LED (active LOW)
#define LED_ENABLED true
const char* AP_NAME = "LayerGlow-Setup";
const char* AP_PASSWORD = "layerglowsetup"; // Change before using in public settings.

WebServer server(80);
Preferences prefs;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);
String ssid, wifiPassword, printerIP, webHost="layerglow", mdnsHostActive="";
constexpr uint8_t MAX_WIFI_PROFILES=8;
struct WifiProfile { String name, password; };
WifiProfile wifiProfiles[MAX_WIFI_PROFILES];
uint8_t wifiProfileCount=0, activeWifi=0;
bool migrateLegacyWifi=false;
uint32_t wifiAttemptStarted=0;
bool mdnsStarted=false, mdnsNeedsRestart=false;
uint32_t lastMdnsAttempt=0,mdnsChangeAt=0;
String printerType="moonraker", moonIP, bambuIP, octoIP, prusaIP, bambuSerial, bambuAccessCode, octoKey, prusaKey;
WiFiClientSecure bambuTLS;
PubSubClient bambuMqtt(bambuTLS);
uint32_t lastBambuAttempt=0,lastBambuReport=0,lastBambuRequest=0;
uint16_t printerPort = 7125, moonPort=7125, octoPort=80, prusaPort=80;
const char* const DEFAULT_ROW_COLORS[6] = {"#00c8f0", "#ff9e35", "#ffce45", "#2fe182", "#ff445c", "#ffffff"};
const char* const stateNames[6] = {"printing", "heating", "paused", "complete", "error", "standby"};
String rowColors[6] = {"#00c8f0", "#ff9e35", "#ffce45", "#2fe182", "#ff445c", "#ffffff"};
const char* const DEFAULT_EXTRA_COLORS[11] = {"#ff9e35", "#ffce45", "#2fe182", "#9b5cff", "#ff445c", "#2878ff", "#ffffff", "#ff3366", "#3edbd1", "#f0df56", "#704dff"};
uint8_t rowColorCount[6] = {1,1,1,1,1,1};
String rowExtraColors[6][11];
uint8_t rowBrightness[6] = {65,65,65,65,65,65};
bool rowPulse[6] = {false,false,false,false,false,false};
bool rowEnabled[6] = {true,true,true,true,true,true};
uint8_t rowStyle[6] = {0,0,0,0,0,0}; // 0 whole bar, 1 last pair, 2 sweeping light
uint16_t rowPeriod[6] = {1400,1400,1400,1400,1400,1400};
int intervalMs = 500;
int activeLedCount = 24;
uint32_t lastPulseFrame = 0;
bool simLoop = false;
String printerState = "unknown", printerFile = "";
float printerProgress = 0;
bool printerConnected = false, printerEnabled = true;
uint32_t lastPrinterReadAt = 0;
String lastError = "Waiting for first poll";
uint32_t lastPoll = 0, lastSimulationStep = 0;
bool apMode = false, simEnabled = false, simRun = false;
int simStatus = 0, simProgress = 0;
String updateError = "";
bool updateAvailable = false;

uint32_t wifiLostSince = 0, lastWifiAttempt = 0, wifiConnectedSince = 0, lastApAttempt = 0;
uint8_t wifiRetryCount = 0;
bool wifiChangePending = false, otaUploadInProgress = false;
uint32_t wifiChangeAt = 0;
bool wifiScanActive = false, wifiScanPausedReconnect = false, lightingBackupAvailable = false;
uint32_t wifiScanStarted = 0;

const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>LayerGlow by FatGiantCat</title><meta name="theme-color" content="#143b2b"><link rel="icon" type="image/svg+xml" sizes="any" href="data:image/svg+xml,%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20viewBox%3D%220%200%2064%2064%22%20role%3D%22img%22%20aria-label%3D%22LayerGlow%22%3E%0A%20%20%3Cdefs%3E%0A%20%20%20%20%3ClinearGradient%20id%3D%22bg%22%20x1%3D%220%22%20y1%3D%220%22%20x2%3D%221%22%20y2%3D%221%22%3E%0A%20%20%20%20%20%20%3Cstop%20stop-color%3D%22%23143b2b%22%2F%3E%0A%20%20%20%20%20%20%3Cstop%20offset%3D%221%22%20stop-color%3D%22%230c251d%22%2F%3E%0A%20%20%20%20%3C%2FlinearGradient%3E%0A%20%20%20%20%3CradialGradient%20id%3D%22glow%22%3E%0A%20%20%20%20%20%20%3Cstop%20stop-color%3D%22%233ef2a8%22%20stop-opacity%3D%22.42%22%2F%3E%0A%20%20%20%20%20%20%3Cstop%20offset%3D%221%22%20stop-color%3D%22%233ef2a8%22%20stop-opacity%3D%220%22%2F%3E%0A%20%20%20%20%3C%2FradialGradient%3E%0A%20%20%20%20%3CclipPath%20id%3D%22round%22%3E%3Crect%20x%3D%222%22%20y%3D%222%22%20width%3D%2260%22%20height%3D%2260%22%20rx%3D%2215%22%2F%3E%3C%2FclipPath%3E%0A%20%20%3C%2Fdefs%3E%0A%20%20%3Cg%20clip-path%3D%22url%28%23round%29%22%3E%0A%20%20%20%20%3Crect%20x%3D%222%22%20y%3D%222%22%20width%3D%2260%22%20height%3D%2260%22%20fill%3D%22url%28%23bg%29%22%2F%3E%0A%20%20%20%20%3Ccircle%20cx%3D%2242%22%20cy%3D%2231%22%20r%3D%2227%22%20fill%3D%22url%28%23glow%29%22%2F%3E%0A%20%20%3C%2Fg%3E%0A%20%20%3Cpath%20d%3D%22M19%2015v31h18%22%20fill%3D%22none%22%20stroke%3D%22%23f8fff9%22%20stroke-width%3D%226%22%20stroke-linecap%3D%22round%22%20stroke-linejoin%3D%22round%22%2F%3E%0A%20%20%3Crect%20x%3D%2239%22%20y%3D%2213%22%20width%3D%2210%22%20height%3D%2210%22%20rx%3D%223%22%20fill%3D%22%23ffbd45%22%2F%3E%0A%20%20%3Crect%20x%3D%2244%22%20y%3D%2227%22%20width%3D%2210%22%20height%3D%2210%22%20rx%3D%223%22%20fill%3D%22%2341dea8%22%2F%3E%0A%20%20%3Crect%20x%3D%2240%22%20y%3D%2241%22%20width%3D%2210%22%20height%3D%2210%22%20rx%3D%223%22%20fill%3D%22%234aafff%22%2F%3E%0A%3C%2Fsvg%3E">
<style>
:root{font-family:Inter,system-ui,-apple-system,Segoe UI,sans-serif;color-scheme:light;--bg:#f5f7f6;--panel:#fff;--field:#fff;--border:#dce3df;--text:#181d1a;--muted:#65736b;--green:#20b461}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text)}main{max-width:1100px;margin:auto;padding:24px 18px 44px}header,.headbar{display:flex;justify-content:space-between;align-items:center;gap:14px;flex-wrap:wrap}header{margin-bottom:24px;padding:12px 0;border-bottom:1px solid var(--border)}h1{font-size:clamp(22px,3vw,30px);margin:0;font-weight:700}h1 span,.brand{color:var(--green)}h2{font-size:17px;margin:0 0 17px}p{margin:9px 0}.muted,small{font-size:13px;color:var(--muted)}.card{background:var(--panel);border:1px solid var(--border);border-radius:12px;padding:22px;margin-bottom:17px}.cols{display:grid;grid-template-columns:1fr 1fr;gap:17px}.chips{display:flex;gap:8px;flex-wrap:wrap;margin-top:10px}.chip{background:#edf5f0;padding:6px 11px;border-radius:6px;font-size:13px}.chip.ok{color:#137a43}.chip.error{color:#b53737}label{display:block;margin:13px 0 6px;color:#26362c;font-size:13px}input,select,button{font:inherit}input:not([type=color]):not([type=range]):not([type=checkbox]):not([type=file]),select{display:block;width:100%;max-width:340px;background:var(--field);border:1px solid #b8c7be;color:#18241c;border-radius:7px;padding:8px 10px;min-height:39px}input[type=number]{max-width:130px!important}#ip{max-width:205px}#ssid,#pass{max-width:300px}#wifiList{max-width:310px}input[type=color]{width:60px;height:37px;border:1px solid #96a79b;border-radius:7px;cursor:pointer;background:var(--field);padding:2px}input[type=range]{width:100%;accent-color:var(--green)}.fields{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:4px 18px}.colorrow{display:grid;grid-template-columns:minmax(110px,1fr) 60px 80px;gap:8px;align-items:center;margin:8px 0}.colorrow label{margin:0}.colorcode{font:12px monospace;color:var(--muted)}button{border:1px solid var(--green);border-radius:7px;background:var(--green);color:#fff;font-weight:700;padding:9px 14px;cursor:pointer}button.alt{background:#edf5f0;color:#245536;border-color:#bfd4c6}button.ghost{background:transparent;color:#356344;border-color:#a9c7b2}button.active{border-color:var(--green);color:#147a42;background:#e3f6e9}button:disabled{opacity:.5}[hidden]{display:none!important}footer a{color:inherit;text-decoration:underline}.langs{display:flex;gap:5px;flex-wrap:wrap}.langs button{padding:6px 9px;font-size:13px}.bar{display:grid;grid-template-columns:repeat(12,minmax(0,1fr));gap:7px;margin:18px 0 9px}.pair{display:grid;grid-template-columns:1fr 1fr;gap:3px}.led{height:28px;border-radius:4px;background:#e3ebe5;transition:background .12s}.pct{font-size:34px;font-weight:750}.controls{display:flex;flex-wrap:wrap;gap:8px;margin-top:14px}.notice{min-height:20px;color:#16864a;font-size:13px}.switch{display:flex;align-items:center;gap:9px;margin:12px 0}.switch label{margin:0}.switch input{width:18px;height:18px;accent-color:var(--green)}.switches{display:flex;gap:20px;flex-wrap:wrap}.checks{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:0 10px;border:1px solid var(--border);border-radius:7px;margin:15px 0;padding:8px 12px}.checks legend{font-size:13px;color:var(--muted);padding:0 5px}.checks .switch{margin:5px 0}hr{border:0;border-top:1px solid var(--border);margin:19px 0}.spaced{margin-top:15px}.scanrow{display:flex;gap:8px;align-items:end}.scanrow select{flex:1}.warning{color:#8e5b19}footer{text-align:center;color:var(--muted);font-size:13px;margin-top:24px}input[type=file]{display:block;max-width:100%;margin:12px 0}@media(max-width:750px){.cols{grid-template-columns:1fr}main{padding:12px}.card{padding:16px}.fields{grid-template-columns:1fr}.bar{gap:3px}.led{height:22px}.colorrow{grid-template-columns:minmax(105px,1fr) 60px 75px}}
.app-header{display:grid;grid-template-columns:1fr auto 1fr;align-items:center}.app-header .brand-title{text-align:center}.app-header .langs{justify-self:end}.app-header .network-open{justify-self:start}.led-card{width:100%}.led-head,.led-row{display:grid;grid-template-columns:minmax(130px,1.2fr) 78px 110px 90px 125px minmax(160px,1.2fr);gap:10px;align-items:center}.led-head{color:var(--muted);font-size:12px;font-weight:700;padding:7px 10px}.led-row{border-top:1px solid var(--border);padding:10px}.led-row:first-child{border-top:0}.led-row label{margin:0;font-weight:600}.led-row input[type=number],.led-row select{max-width:100%!important}.led-row .switch{margin:0}.led-row .field-label{display:none}.led-card .global-fields{display:flex;gap:18px;flex-wrap:wrap;margin-bottom:12px}.led-card .global-fields>div{width:160px}.led-card .global-fields input,.led-card .global-fields select{max-width:160px!important}dialog{width:min(560px,calc(100% - 24px));max-height:calc(100vh - 36px);overflow:auto;border:1px solid var(--border);border-radius:13px;padding:22px;background:#fff;color:var(--text);box-shadow:0 20px 60px #15291b44}dialog::backdrop{background:#14261a99}.dialog-head{display:flex;align-items:center;justify-content:space-between;gap:10px}.toasts{position:fixed;right:16px;bottom:16px;display:grid;gap:9px;z-index:100;max-width:min(380px,calc(100vw - 32px))}.toast{background:#fff;border:1px solid var(--border);border-left:4px solid var(--green);box-shadow:0 7px 22px #172d1d2b;border-radius:8px;padding:11px 14px;font-size:13px}.toast.error{border-left-color:#dd5555}.toast button{float:right;padding:0 3px;margin-left:12px;border:0;background:none;color:var(--muted)}@media(max-width:850px){.app-header{grid-template-columns:1fr auto}.app-header .brand-title{grid-column:1/-1;grid-row:1}.app-header .network-open{grid-row:2}.app-header .langs{grid-row:2}.led-head{display:none}.led-row{grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.led-row .row-title{grid-column:1/-1}.led-row .field-label{display:block;font-size:12px;color:var(--muted);font-weight:400;margin:0 0 3px}.led-row>div{min-width:0}}@media(max-width:500px){.led-row{grid-template-columns:1fr 1fr}.led-card .global-fields>div{width:calc(50% - 9px)}.langs button{padding:5px}.toasts{right:10px;bottom:10px}}
.header-actions{display:flex;gap:7px;justify-self:start;flex-wrap:wrap}.app-header .header-actions{justify-self:start}.app-header{background:#fff;border-bottom:1px solid #e4e4e4;min-height:62px}.app-header .menu-button{background:transparent;color:#292929;border:0;border-bottom:2px solid transparent;border-radius:0;box-shadow:none;padding:20px 8px 17px;font-size:13px;font-weight:650;min-height:0}.app-header .menu-button:hover,.app-header .menu-button:focus-visible{background:transparent;color:var(--green);border-bottom-color:var(--green)}.app-header .langs .menu-button.active{background:transparent;color:#137a43;border-bottom-color:var(--green);box-shadow:none}.app-header .langs .menu-button{padding:20px 8px 17px}.preview-card{padding-bottom:12px}.preview-card .bar{margin:10px 0 6px}.preview-card hr{margin:12px 0}.preview-card .spaced{margin-top:9px}.preview-card .switch{margin:5px 0}.preview-card .fields label{margin-top:7px}.preview-card #simMessage:empty{display:none}#updateDialog .controls{align-items:center;margin-top:5px}#updateDialog .controls button{white-space:normal}@media(max-width:850px){.app-header .header-actions{grid-row:2;grid-column:1}.app-header .langs{grid-row:2;grid-column:2}}@media(max-width:530px){.app-header{grid-template-columns:1fr}.app-header .brand-title,.app-header .header-actions,.app-header .langs{grid-column:1;justify-self:center}.app-header .brand-title{grid-row:1}.app-header .header-actions{grid-row:2}.app-header .langs{grid-row:3}}.faq-signals{display:grid;gap:8px;margin:14px 0}.faq-signals>div{display:flex;align-items:center;gap:16px;padding:11px 12px;background:#f5f9f6;border:1px solid var(--border);border-radius:8px}.faq-signals strong{font-size:19px;min-width:60px;color:#145d37;letter-spacing:2px}#faqDialog h3{font-size:15px;margin:16px 0 5px}.led-row{border-radius:9px}@media(max-width:650px){main{padding:8px 10px 22px}.card{padding:13px;margin-bottom:10px}.app-header{padding:7px 0 9px;margin-bottom:12px;gap:0}.app-header .menu-button,.app-header .langs .menu-button{padding:7px 6px;font-size:12px}.app-header .header-actions,.app-header .langs{gap:3px}.app-header .brand-title{line-height:1.1}.app-header .brand-title .muted{font-size:11px;margin-top:3px}.preview-card .fields{grid-template-columns:minmax(120px,.85fr) minmax(0,1.15fr);gap:10px}.preview-card .fields>div{min-width:0}.preview-card .fields select{max-width:100%}.preview-card .switches{gap:5px 15px}.preview-card .switch label{font-size:12px}.preview-card .bar{gap:3px}.led-card .led-row{display:grid;grid-template-columns:repeat(6,minmax(0,1fr));gap:7px;padding:10px;margin:8px 0;border:1px solid var(--border);background:#fbfdfb}.led-row .row-title{grid-column:1/-1;padding-bottom:5px;border-bottom:1px solid var(--border);font-size:14px}.led-row>div:nth-child(2),.led-row>div:nth-child(3),.led-row>div:nth-child(4){grid-column:span 2}.led-row>div:nth-child(5){grid-column:span 2}.led-row>div:nth-child(6){grid-column:span 4}.led-row .field-label{display:block;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;font-size:11px;margin-bottom:3px}.led-row input[type=number],.led-row select{width:100%;min-width:0;max-width:none!important;padding:6px;height:36px;min-height:36px;font-size:13px}.led-row input[type=color]{height:36px;width:100%;max-width:66px}.led-row>div:nth-child(4){display:flex;align-items:center;justify-content:center;gap:7px;padding-top:15px}.led-row>div:nth-child(4) .field-label{margin:0;white-space:normal}.led-row>div:nth-child(5) .field-label,.led-row>div:nth-child(6) .field-label{white-space:normal;min-height:26px;line-height:1.15;display:flex;align-items:end}.led-row>div:nth-child(4) input{width:19px;height:19px;flex:none}.led-card .global-fields{margin:5px 0}.led-card .global-fields>div{width:100%;max-width:200px}.led-card hr{margin:12px 0}.led-card .notice:empty{display:none}.led-card .muted{margin:8px 0}.pct{font-size:29px}.led{height:22px}dialog{padding:16px}#updateDialog .controls{gap:7px}}@media(max-width:370px){.app-header .menu-button,.app-header .langs .menu-button{padding:6px 4px;font-size:11px}.led-row{gap:5px}.led-row .field-label{font-size:10px}.preview-card .fields{grid-template-columns:minmax(104px,.8fr) minmax(0,1.2fr)}}.app-header #openUpdate.update-ready,.app-header #openUpdate.update-ready:hover,.app-header #openUpdate.update-ready:focus-visible{background:#bf1e2e;color:#fff;border-color:#bf1e2e;box-shadow:0 1px 5px #92182735}.led-row .row-title{display:flex;align-items:center;justify-content:space-between;gap:5px;flex-wrap:wrap}.led-row .enabled-toggle{display:inline-flex;align-items:center;gap:5px;font-size:11px;font-weight:500;white-space:nowrap;color:#326246}.led-row .enabled-toggle input{width:17px;height:17px;margin:0;accent-color:var(--green)}@media(max-width:650px){.led-card .led-row .row-title{flex-wrap:nowrap}.led-row .enabled-toggle{font-size:12px}.led-row .enabled-toggle input{width:19px;height:19px}}.led-head,.led-row{grid-template-columns:minmax(120px,1fr) minmax(170px,1.5fr) 90px 90px 110px minmax(150px,1.2fr)}.palette-controls{display:grid;gap:6px}.palette-count{display:flex;align-items:center;gap:5px}.palette-count label{font-size:11px;font-weight:400}.led-row .palette-count select{width:55px;min-height:30px;padding:3px 6px;font-size:12px}.palette-swatches{display:flex;gap:4px;flex-wrap:wrap}.led-row .palette-swatches input[type=color]{width:30px;height:32px;min-width:0;padding:1px}.bar{grid-template-columns:repeat(auto-fit,minmax(24px,1fr));gap:4px}.pair{min-width:0}.pair .led{min-width:0}@media(max-width:1000px){.led-head{display:none}.led-row{grid-template-columns:repeat(6,minmax(0,1fr))}.led-row .row-title{grid-column:1/-1}.led-row>div:nth-child(2){grid-column:span 4}.led-row>div:nth-child(3){grid-column:span 2}.led-row>div:nth-child(4){grid-column:span 1}.led-row>div:nth-child(5){grid-column:span 2}.led-row>div:nth-child(6){grid-column:span 3}.led-row .field-label{display:block;font-size:11px;color:var(--muted)}.led-row>div:nth-child(4){padding-top:0;display:block}.led-row .palette-swatches input[type=color]{width:32px;height:32px}.led-row .palette-count select{height:30px;min-height:30px}.led-row>div:nth-child(5) .field-label,.led-row>div:nth-child(6) .field-label{min-height:0}.preview-card .bar{gap:3px}.led-card .global-fields>div{max-width:220px;width:220px}}.led-head>span{overflow-wrap:anywhere}.led-row>div:nth-child(4) .field-label{white-space:normal;text-overflow:clip;overflow:visible}@media(max-width:1000px){.led-card .led-row{grid-template-columns:repeat(8,minmax(0,1fr))}.led-row>div:nth-child(2){grid-column:span 6}.led-row>div:nth-child(3){grid-column:span 2}.led-row>div:nth-child(4){grid-column:span 2}.led-row>div:nth-child(5){grid-column:span 2}.led-row>div:nth-child(6){grid-column:span 4}}

.app-header{position:sticky;top:0;z-index:40;margin-bottom:12px;transition:transform .2s ease;box-shadow:0 2px 10px #14261a13}.preview-card{position:sticky;top:var(--header-stick-height,78px);z-index:30;box-shadow:0 7px 20px #172d1d16}.preview-card h2{margin-bottom:12px}.preview-controls{display:flex;gap:10px 22px;align-items:center;flex-wrap:wrap}.preview-controls .switch:last-child{margin-left:auto}.preview-details{display:grid;grid-template-columns:1fr 1fr 1.45fr;gap:16px;align-items:end}.preview-details label{margin:0 0 6px}.preview-status .chips{margin:0 0 9px}.preview-status .headbar{justify-content:flex-start;gap:14px}.preview-test-state select{max-width:none;width:100%}.preview-progress{min-width:0}.preview-progress input{margin-bottom:6px}.preview-card .bar{margin:13px 0 0}.preview-card .notice:empty{display:none}.preview-card #file{max-width:150px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.mobile-languages{display:none!important}.language-options{display:grid;gap:10px;margin-top:16px}.language-options button{text-align:left}
.palette-controls{min-width:0}.palette-toolbar{display:flex;align-items:flex-start;gap:7px;min-width:0}.palette-swatches{display:grid;grid-template-columns:repeat(6,30px);gap:5px;align-content:start}.led-row .palette-swatches input[type=color]{width:30px;height:32px}.palette-actions{display:flex;gap:4px;flex-shrink:0}.palette-actions button{width:29px;height:32px;padding:0;font-size:21px;line-height:1}.led-row .palette-actions button:disabled{opacity:.35}.led-head,.led-row{grid-template-columns:minmax(110px,1fr) minmax(275px,1.8fr) 84px 70px 103px minmax(135px,1fr)}
@media(max-width:850px){.app-header .langs{display:none}.app-header .mobile-languages{display:inline-flex!important}.app-header{grid-template-columns:1fr auto}.app-header .brand-title{grid-row:1;grid-column:2;justify-self:end;text-align:right}.app-header .header-actions{grid-row:1;grid-column:1}.app-header .menu-button{padding:12px 6px}.preview-card{top:0}.app-header.scroll-hidden{transform:translateY(calc(-100% - 2px))}.app-header:not(.scroll-hidden)+.preview-card{top:var(--header-stick-height,68px)}.preview-card.preview-compact .preview-controls,.preview-card.preview-compact .preview-test-state,.preview-card.preview-compact .preview-progress,.preview-card.preview-compact #simMessage{display:none}.preview-card.preview-compact .preview-details{display:block}.preview-card.preview-compact .preview-status{display:flex;align-items:center;justify-content:space-between;gap:6px}.preview-card.preview-compact .preview-status .chips{margin:0}.preview-card.preview-compact .bar{margin-top:6px}.preview-card.preview-compact h2{font-size:14px;margin:0 0 4px}.preview-card.preview-compact .pct{font-size:22px}.preview-card.preview-compact #ledcount{display:none}.preview-details{grid-template-columns:1fr 1fr;gap:8px}.preview-progress{grid-column:1/-1}.preview-controls{gap:1px 15px}.preview-controls .switch:last-child{margin-left:0}}
@media(max-width:650px){.app-header{display:flex;align-items:center;justify-content:space-between;gap:4px;padding:0 2px 3px;margin-bottom:8px}.app-header .brand-title{display:none}.app-header .header-actions{display:flex;justify-content:space-between;flex:1;gap:1px;flex-wrap:nowrap}.app-header .menu-button{padding:10px 3px;font-size:11px;white-space:nowrap}.preview-card{padding:10px 12px;max-height:min(55vh,410px);overflow:auto}.preview-card h2{font-size:15px;margin-bottom:5px}.preview-controls .switch label{font-size:11px}.preview-card .bar{gap:2px;overflow:hidden}.preview-card .led{height:15px}.preview-card .pct{font-size:25px}.led-card .led-row{grid-template-columns:repeat(8,minmax(0,1fr))}.led-row>div:nth-child(2){grid-column:1/-1}.led-row>div:nth-child(3){grid-column:span 2}.led-row>div:nth-child(4){grid-column:span 2}.led-row>div:nth-child(5){grid-column:span 2}.led-row>div:nth-child(6){grid-column:span 2}.palette-swatches{grid-template-columns:repeat(6,29px)}.led-row .palette-swatches input[type=color]{width:29px}}
@media(max-width:390px){.palette-swatches{grid-template-columns:repeat(6,27px);gap:4px}.led-row .palette-swatches input[type=color]{width:27px}.palette-actions button{width:26px}.app-header .menu-button{font-size:10px}}

.discovery-progress{display:flex;align-items:center;gap:12px;margin:8px 0}.discovery-progress[hidden]{display:none}.discovery-progress progress{flex:1;min-width:0;height:12px;accent-color:#198347}.discovery-progress span{font-variant-numeric:tabular-nums;font-weight:700;min-width:3em;text-align:right}.connection-dashboard{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin:14px 0 2px}.connection-panel{background:#f7faf8;border:1px solid var(--border);border-radius:9px;padding:12px;min-width:0}.connection-panel h3{font-size:14px;margin:0 0 8px}.connection-panel h3:not(:first-child){margin-top:16px}.connection-panel dl{display:grid;grid-template-columns:minmax(105px,1fr) minmax(0,1.1fr);gap:5px 8px;margin:0;font-size:12px}.connection-panel dt{color:var(--muted)}.connection-panel dd{margin:0;overflow-wrap:anywhere;font-weight:600}.connection-panel .link-ok{color:#137a43}.connection-panel .link-off{color:#a64040}#connectionInfoMessage:empty{display:none}@media(max-width:530px){.connection-dashboard{grid-template-columns:1fr}.connection-panel{padding:11px}.connection-panel dl{grid-template-columns:minmax(105px,1fr) minmax(0,1.25fr)}}
#linkFriendly{color:#147a42;text-decoration:underline;overflow-wrap:anywhere}#linkFriendlyState{display:block;margin-top:3px;font-size:11px}#webHost{max-width:300px}.saved-network{display:flex;align-items:center;justify-content:space-between;gap:12px;border:1px solid var(--border);border-radius:8px;padding:9px 11px;margin:7px 0}.saved-network strong{overflow-wrap:anywhere}.saved-network small{display:block}.saved-network .controls{margin:0;flex-shrink:0}.saved-network .controls button{font-size:12px;padding:6px 9px}@media(max-width:520px){.saved-network{align-items:flex-start;flex-direction:column}.saved-network .controls{width:100%}}
.led-head,.led-row{grid-template-columns:minmax(120px,1fr) minmax(170px,1.5fr) minmax(110px,1fr) minmax(130px,1fr) minmax(160px,1.2fr)}
.brightness-control details,.pulse-control details{border:1px solid var(--border);border-radius:8px;background:#fff;padding:7px 9px}
.brightness-control summary,.pulse-control summary{display:flex;justify-content:space-between;align-items:center;gap:6px;cursor:pointer;min-height:30px;list-style:none}
.brightness-control summary::-webkit-details-marker,.pulse-control summary::-webkit-details-marker{display:none}
.brightness-control summary strong,.pulse-control summary strong{font-size:12px;color:var(--green);white-space:nowrap}
.brightness-control input[type=range],.pulse-control input[type=range]{width:100%;max-width:none;margin:8px 0;accent-color:var(--green)}
.pulse-control .pulse-enabled{display:flex;align-items:center;gap:7px;padding:8px 0;font-weight:500}
.pulse-control .pulse-enabled input[type=checkbox]{width:18px;height:18px;accent-color:var(--green)}
.pulse-control .range-line{display:flex;align-items:center;gap:8px}
.pulse-control output{min-width:53px;text-align:right;font-size:12px;font-variant-numeric:tabular-nums}
.effect-control select{display:block;width:100%;min-width:0}
.effect-name{display:none;overflow-wrap:anywhere;font-size:12px;color:var(--green);margin-bottom:5px}
@media(max-width:850px){
 dialog:not(#languageDialog){position:fixed;inset:0;margin:0;width:100vw;max-width:none;height:100vh;height:100dvh;max-height:none;border:0;border-radius:0;box-shadow:none;padding:0 16px 24px;box-sizing:border-box;overscroll-behavior:contain}
 dialog:not(#languageDialog)::backdrop{background:#fff}
 dialog:not(#languageDialog) .dialog-head{position:sticky;top:0;z-index:5;background:#fff;border-bottom:1px solid var(--border);padding:10px 0;margin-bottom:12px}
 dialog:not(#languageDialog) .dialog-head button{min-height:44px}
 body:has(dialog:not(#languageDialog)[open]){overflow:hidden}
 #networkDialog .connection-dashboard{grid-template-columns:1fr}
 .led-card .led-row{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:9px}
 .led-card .led-row>div:nth-child(1),.led-card .led-row>div:nth-child(2),.led-card .led-row>div:nth-child(5){grid-column:1/-1}
 .led-card .led-row>div:nth-child(3),.led-card .led-row>div:nth-child(4){grid-column:span 1}
 .led-card .led-row .pulse-control{display:block;padding:0}
 .led-card .led-row .pulse-control input[type=range]{width:100%;height:auto;min-height:20px}
 .led-card .led-row .pulse-control input[type=checkbox]{width:18px;height:18px;min-height:0}
 .led-card .led-row .brightness-control input[type=range]{width:100%;height:auto;min-height:20px}
 .led-row .effect-control .field-label{display:block;white-space:normal;min-height:0}
 .effect-name{display:block}
 .led-row .effect-control select{width:100%;max-width:none!important;min-width:0;min-height:44px;height:auto;font-size:14px}
 .preview-controls .switch:last-child{margin-left:0}
}
@media(max-width:380px){
 .preview-controls{gap:3px 8px}
 .preview-controls .switch label{font-size:10px}
 .led-card .led-row{grid-template-columns:1fr}
 .led-card .led-row>div:nth-child(3),.led-card .led-row>div:nth-child(4){grid-column:1/-1}
}
/* Mobile layout: keep the preview close to the settings and give every control room to breathe. */
@media (max-width: 850px) {
  body {font-size: 14px; line-height: 1.45}
  main {max-width: 700px; padding: 0 12px 28px}
  .card {padding: 16px; margin-bottom: 12px; border-radius: 14px}
  h2 {font-size: 16px; line-height: 1.3; margin-bottom: 12px}
  .muted, small {line-height: 1.5}
  button, input, select {touch-action: manipulation}
  button:focus-visible, input:focus-visible, select:focus-visible, summary:focus-visible {
    outline: 3px solid #176c41; outline-offset: 2px;
  }

  .app-header {display: block; padding: 8px 0; margin-bottom: 10px; min-height: 0;
    border-bottom: 0; box-shadow: 0 3px 12px #172d1d15}
  .app-header .header-actions {display: grid; grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 6px; width: 100%; flex-wrap: nowrap}
  .app-header .brand-title {display: none}
  .app-header .menu-button {display: flex; align-items: center; justify-content: center;
    width: 100%; min-width: 0; min-height: 44px; padding: 6px 7px;
    font-size: 12px; line-height: 1.2; white-space: normal; text-align: center;
    color: #244332; background: #f7faf8; border: 1px solid #d6e5da;
    border-radius: 9px; box-shadow: none}
  .app-header .menu-button:hover, .app-header .menu-button:focus-visible {
    background: #ebf6ee; border-color: #92bea1;
  }
  .app-header #openLang {font-size: 21px}
  .app-header #openUpdate.update-ready {color: #fff; background: #b93237; border-color: #b93237}

  .preview-card {top: 0; padding: 14px; max-height: min(48dvh, 380px); overflow: auto;
    overscroll-behavior: contain; box-shadow: 0 6px 18px #172d1d18}
  .preview-card h2 {margin-bottom: 8px}
  .preview-controls {display: grid; grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 6px 10px; align-items: start}
  .preview-controls .switch {min-width: 0; min-height: 38px; margin: 0;
    align-items: center; gap: 6px}
  .preview-controls .switch:last-child {grid-column: 1 / -1; margin-left: 0}
  .preview-controls .switch input {width: 20px; height: 20px; flex: 0 0 20px}
  .preview-controls .switch label {font-size: 12px; line-height: 1.3; overflow-wrap: anywhere}
  .preview-details {grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 8px 12px;
    align-items: end}
  .preview-status {grid-column: 1 / -1}
  .preview-progress {grid-column: auto}
  .preview-details select {width: 100%; max-width: none; min-height: 44px}
  .preview-details label {font-size: 12px; font-weight: 600}
  .preview-card .bar {margin: 10px 0 0; gap: 3px}
  .preview-card .led {height: 19px}
  .preview-card .pct {font-size: 27px}
  .preview-card.preview-compact {max-height: 128px; overflow: hidden}
  .preview-card.preview-compact .preview-status {min-width: 0}
  .preview-card.preview-compact .chips {min-width: 0}
  .preview-card.preview-compact #displaystate {overflow: hidden; text-overflow: ellipsis;
    white-space: nowrap; max-width: 48vw}

  .led-card .led-row {display: grid; grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 11px; align-items: start; padding: 14px; margin: 10px 0;
    border: 1px solid #dbe6de; border-radius: 12px; background: #fff;
    box-shadow: 0 2px 7px #172d1d0b}
  .led-card .led-row > div {min-width: 0}
  .led-card .led-row > .row-title, .led-card .led-row > .palette-controls,
  .led-card .led-row > .effect-control {grid-column: 1 / -1}
  .led-card .led-row > .brightness-control,
  .led-card .led-row > .pulse-control {grid-column: span 1}
  .led-card .led-row > .brightness-control:has(details[open]),
  .led-card .led-row > .pulse-control:has(details[open]) {grid-column: 1 / -1}
  .led-card .led-row .row-title {display: flex; align-items: center;
    justify-content: space-between; gap: 10px; flex-wrap: wrap;
    padding: 0 0 9px; border-bottom: 1px solid #e4ebe6}
  .led-card .led-row .row-title > label:first-child {font-size: 15px;
    line-height: 1.3; font-weight: 750}
  .led-card .led-row .enabled-toggle {min-height: 36px; padding: 5px 9px;
    font-size: 12px; background: #eef7f1; border-radius: 8px}
  .led-card .led-row .enabled-toggle input {width: 20px; height: 20px}
  .led-card .palette-toolbar {display: grid; grid-template-columns: minmax(0, 1fr) auto;
    align-items: center; gap: 9px}
  .led-card .palette-swatches {display: grid; grid-template-columns: repeat(6, minmax(0, 1fr));
    gap: 5px; width: 100%}
  .led-card .palette-swatches input[type=color] {width: 100%; min-width: 0;
    height: 38px; border-radius: 8px; cursor: pointer}
  .led-card .palette-actions {display: flex; gap: 5px}
  .led-card .palette-actions button {width: 38px; height: 38px; font-size: 23px}
  .led-card .brightness-control details, .led-card .pulse-control details {
    padding: 0; overflow: hidden; border-color: #d6e4d9; border-radius: 9px;
    background: #f8fbf9}
  .led-card .brightness-control summary, .led-card .pulse-control summary {
    min-height: 58px; padding: 8px 10px; flex-wrap: wrap;
    align-content: center; gap: 2px 8px}
  .led-card .brightness-control summary span, .led-card .pulse-control summary span {
    font-size: 12px; line-height: 1.25; color: #31473a}
  .led-card .brightness-control summary strong, .led-card .pulse-control summary strong {
    font-size: 13px; color: #126a3c; margin-left: auto}
  .led-card .brightness-control details[open], .led-card .pulse-control details[open] {
    background: #fff; border-color: #a9d1b4}
  .led-card .led-row .brightness-control input[type=range] {width: calc(100% - 24px);
    margin: 10px 12px 14px; min-height: 28px}
  .led-card .pulse-enabled {min-height: 44px; padding: 7px 12px;
    border-top: 1px solid #e8eee9; font-size: 13px}
  .led-card .pulse-control .range-line {padding: 0 12px 10px; gap: 10px}
  .led-card .pulse-control .range-line input[type=range] {min-height: 28px; margin: 0}
  .led-card .pulse-control output {font-size: 13px; min-width: 58px}
  .led-row .effect-control .field-label {font-size: 12px; font-weight: 650;
    color: #31473a; margin: 0 0 3px}
  .led-row .effect-name {display: block; font-size: 12px; line-height: 1.35;
    color: #126a3c; font-weight: 650; margin-bottom: 5px}
  .led-row .effect-control select {width: 100%; max-width: none !important;
    min-height: 46px; padding: 9px 12px; font-size: 14px}
  .led-card .global-fields > div {width: min(100%, 220px); max-width: 220px}
  .led-card .global-fields input {min-height: 44px}
  .led-card .muted {font-size: 12px; line-height: 1.5; margin: 9px 0}
  .led-card .controls button {min-height: 44px}

  dialog:not(#languageDialog) {padding: 0 16px 24px; line-height: 1.45}
  dialog:not(#languageDialog) > :not(.dialog-head) {max-width: 680px; margin-left: auto; margin-right: auto}
  dialog:not(#languageDialog) .dialog-head {margin: 0 -16px 16px; padding: 12px 16px;
    box-shadow: 0 3px 10px #172d1d12}
  dialog:not(#languageDialog) .dialog-head h2 {margin: 0; font-size: 18px}
  dialog:not(#languageDialog) h3 {line-height: 1.35}
  dialog:not(#languageDialog) form input:not([type=checkbox]):not([type=range]):not([type=color]),
  dialog:not(#languageDialog) form select {min-height: 46px; max-width: 100%}
  dialog:not(#languageDialog) .controls button, dialog:not(#languageDialog) form button {
    min-height: 44px; white-space: normal; line-height: 1.25}
  dialog:not(#languageDialog) .connection-panel {padding: 14px}
  dialog:not(#languageDialog) .connection-panel dl {gap: 8px 10px; line-height: 1.4}
  dialog:not(#languageDialog) .saved-network .controls button {min-height: 42px}
  #languageDialog {width: min(420px, calc(100% - 24px)); padding: 18px;
    border-radius: 14px}
  #languageDialog .language-options button {min-height: 46px}
}

@media (max-width: 390px) {
  .led-card .palette-toolbar {grid-template-columns: 1fr}
  .led-card .palette-actions {justify-content: flex-end}
  .preview-details {grid-template-columns: 1fr}
  .preview-test-state, .preview-progress {grid-column: 1}
  .preview-card .bar {gap: 2px}
}

@media (prefers-reduced-motion: reduce) {
  .app-header, .led, .preview-card {transition: none}
}

.mobile-preview-summary {display: none}

@media (max-width: 850px) {
  .app-header {padding: 4px 0 6px}
  .app-header .header-actions {display: flex; align-items: center;
    justify-content: space-between; gap: 2px; flex-wrap: nowrap}
  .app-header .menu-button {display: inline-flex; width: auto; flex: 0 1 auto;
    min-width: 0; min-height: 42px; padding: 5px 2px;
    font-size: clamp(10px, 2.8vw, 13px); font-weight: 680; line-height: 1.2;
    color: #234632; background: transparent; border: 0;
    border-bottom: 2px solid transparent; border-radius: 0;
    white-space: nowrap; box-shadow: none}
  .app-header .menu-button:hover, .app-header .menu-button:focus-visible {
    color: #116739; background: transparent; border-bottom-color: #18864d}
  .app-header #openLang {font-size: 19px; flex: 0 0 auto; padding: 5px 3px}
  .app-header #openUpdate.update-ready,
  .app-header #openUpdate.update-ready:hover,
  .app-header #openUpdate.update-ready:focus-visible {
    color: #b72e39; background: transparent; border: 0;
    border-bottom: 2px solid #b72e39; box-shadow: none}

  .mobile-preview-summary {display: flex; flex-wrap: wrap; align-items: baseline;
    gap: 2px 7px; padding: 8px 10px; margin: 7px 0;
    font-size: 12px; line-height: 1.35; background: #f2f8f4;
    border: 1px solid #dfebe2; border-radius: 8px}
  .mobile-preview-summary > :first-child {font-weight: 650; color: #344a3b}
  .mobile-preview-summary strong {color: #116739; font-size: 13px}
  #mobileState {color: #344a3b}
  #mobileLedCount {font-weight: 700; color: #116739; white-space: nowrap}
  .preview-card .preview-status, .preview-card .preview-progress {display: none}
  .preview-card .preview-details {display: block}
  .preview-card .preview-test-state {max-width: 100%}
  .preview-card.preview-compact {max-height: 110px}
  .preview-card.preview-compact .mobile-preview-summary {margin: 3px 0; padding: 3px 7px}

  .led-card .led-row > .brightness-control:has(details[open]),
  .led-card .led-row > .pulse-control:has(details[open]) {grid-column: span 1}
  .led-card .pulse-control .range-line {display: grid; grid-template-columns: minmax(0, 1fr);
    gap: 0; padding: 0 10px 7px}
  .led-card .pulse-control .range-line input[type=range] {width: 100%; min-width: 0}
  .led-card .pulse-control output {width: 100%; text-align: right; margin: 0}
  .led-card .pulse-enabled {padding: 6px 10px}
  .led-row .effect-control .field-label {display: inline; margin: 0 6px 0 0}
  .led-row .effect-name {display: inline; margin: 0; line-height: 1.4}
  .led-row .effect-control select {margin-top: 6px}
}

@media (max-width: 370px) {
  .app-header .menu-button {font-size: 10px; padding-inline: 1px}
  .app-header #openLang {font-size: 18px}
  .led-card .brightness-control summary, .led-card .pulse-control summary {
    padding: 7px; min-height: 60px}
}

/* Keep both collapsed controls in one row, including phones below 380 CSS pixels. */
@media (max-width: 850px) {
  .led-card .led-row > div.brightness-control,
  .led-card .led-row > div.pulse-control {grid-column: span 1}
  .led-card .led-row > div.brightness-control:has(details[open]),
  .led-card .led-row > div.pulse-control:has(details[open]) {grid-column: span 1}
}

@media (min-width: 1040px) {
  .led-head, .led-row {
    grid-template-columns: minmax(145px, 1.05fr) minmax(284px, 1.85fr)
      minmax(130px, .95fr) minmax(130px, .95fr) minmax(200px, 1.25fr);
    gap: 12px;
  }
}
@media (min-width: 1001px) and (max-width: 1039px) {
  .led-row .palette-toolbar {flex-wrap: wrap}
  .led-row .palette-actions {margin-left: auto}
}
@media (min-width: 851px) {
  .led-row .row-title > label:first-child {font-size: 15px}
}

</style></head><body><main>
<header class="app-header"><div class="header-actions"><button type="button" class="menu-button" id="openNetwork" data-i="networkSettings">Network</button><button type="button" class="menu-button" id="openPrinter" data-i="printerSettings">Printer</button><button type="button" class="menu-button" id="openUpdate" data-i="updateMenu">Update</button><button type="button" class="menu-button" id="openFAQ" data-i="faqMenu">FAQ</button><button type="button" class="menu-button mobile-languages" id="openLang" data-aria-i="languages" aria-label="Languages">🇬🇧</button></div><div class="brand-title"><h1 data-i="appName">LayerGlow</h1><div class="muted">by FatGiantCat</div></div><nav class="langs" aria-label="Language"><button type="button" class="menu-button" id="en" onclick="setLang('en')">🇬🇧 EN</button><button type="button" class="menu-button" id="pl" onclick="setLang('pl')">🇵🇱 PL</button><button type="button" class="menu-button" id="es" onclick="setLang('es')">🇪🇸 ES</button><button type="button" class="menu-button" id="de" onclick="setLang('de')">🇩🇪 DE</button></nav></header>
<section class="card preview-card" id="previewCard"><h2 data-i="preview">LED preview and test</h2>
<div class="preview-controls"><div class="switch"><input type="checkbox" id="sim" onchange="simToggle()"><label for="sim" data-i="testMode">Test mode</label></div><div class="switch"><input type="checkbox" id="simLoop" onchange="sendSimulation()"><label for="simLoop" data-i="loopTest">Repeat test after 100%</label></div><div class="switch"><input type="checkbox" id="autoProgress" onchange="toggleAutoProgress()"><label for="autoProgress" data-i="autoProgress">Play progress automatically</label></div></div>
<div class="mobile-preview-summary" aria-live="polite"><span data-i="testProgress">Test progress</span><strong id="mobilePct">0%</strong><span id="mobileLedCount">0/24 LED</span><span id="mobileState">Idle</span></div>
<div class="preview-details"><div class="preview-status"><div class="chips"><span id="displaystate" class="chip">…</span><span id="file" class="chip"></span></div><div class="headbar"><span class="pct" id="percent">0%</span><span class="muted" id="ledcount">0 / 12</span></div></div><div class="preview-test-state"><label for="testStatus" data-i="testStatus">Test state</label><select id="testStatus" onchange="testStatusChanged()"><option value="printing" data-i="printing">Printing</option><option value="heating" data-i="heating">Heating</option><option value="paused" data-i="paused">Paused</option><option value="complete" data-i="complete">Completed</option><option value="error" data-i="error">Error</option><option value="standby" data-i="standby">Idle</option></select></div><div class="preview-progress"><label for="manualProgress"><span data-i="testProgress">Test progress</span>: <span id="manualPct">0%</span></label><input type="range" min="0" max="100" value="0" id="manualProgress" oninput="document.getElementById('manualPct').textContent=this.value+'%';sendSimulation()"></div></div>
<div class="bar" id="bar"></div><p id="simMessage" class="notice"></p></section>
<section class="card led-card"><h2 data-i="lighting">Colours and LED settings</h2><form id="lightForm"><div class="led-head"><span data-i="function">Function</span><span data-i="colour">Colour</span><span data-i="brightness">Brightness</span><span data-i="pulseSetting">Pulse</span><span data-i="effect">Effect</span></div><div id="ledRows"></div><hr><div class="global-fields"><div><label for="ledCount" data-i="ledCount">Physical LEDs</label><input name="ledCount" id="ledCount" type="number" min="5" max="300" step="1" value="24" required inputmode="numeric"></div></div><p class="muted" data-i="stripCountHint">Total connected LEDs: 5–300. Two 12-LED strips in one chain = 24. Longer strips need suitable power and wiring.</p><p class="muted" data-i="colourEffectHint">Select at least two colours for palette animations. Rainbow uses its own spectrum. These effects run without Pulse; Cycle controls their speed.</p><p class="muted" data-i="paletteHint">Choose 1–12 colours per stage. Colours form equal zones along the strip; printing reveals them with progress.</p><p class="muted" data-i="powerHint">Check LED power wiring before increasing brightness.</p><div class="controls"><button type="button" class="ghost" id="resetColors" data-i="resetLights">Restore default LED settings</button><button type="button" class="alt" id="backupLights" data-i="backupLights">Save my LED backup</button><button type="button" class="alt" id="restoreLights" data-i="restoreLights" disabled>Restore my LED backup</button></div><p id="lightMessage" class="notice"></p></form></section>

<footer><a href="https://github.com/TomaszK00/LayerGlow" target="_blank" rel="noopener noreferrer"><span data-i="appName">LayerGlow</span> · v2.34 · by FatGiantCat · GitHub</a></footer>
<dialog id="networkDialog" aria-labelledby="networkTitle"><div class="dialog-head"><h2 id="networkTitle" data-i="networkSettings">Network</h2><button type="button" class="ghost" id="closeNetwork" data-i="close">Close</button></div><div class="connection-dashboard" aria-live="polite"><div class="connection-panel"><h3 data-i="homeConnection">Home Wi-Fi / LayerGlow</h3><dl><dt data-i="connectionState">Status</dt><dd id="linkWifi">—</dd><dt data-i="wifiName">Wi-Fi name (SSID)</dt><dd id="linkSSID">—</dd><dt data-i="deviceIP">LayerGlow IP</dt><dd id="linkLocalIP">—</dd><dt data-i="friendlyAddress">Easy address</dt><dd><a id="linkFriendly" href="#">http://layerglow.local/</a><small id="linkFriendlyState"></small></dd><dt data-i="gatewayIP">Router IP</dt><dd id="linkGateway">—</dd><dt data-i="subnetMask">Subnet mask</dt><dd id="linkSubnet">—</dd><dt data-i="dnsServer">DNS</dt><dd id="linkDNS">—</dd><dt data-i="wifiSignal">Signal</dt><dd id="linkRSSI">—</dd><dt data-i="wifiChannel">Wi-Fi channel</dt><dd id="linkChannel">—</dd><dt data-i="deviceMAC">LayerGlow MAC</dt><dd id="linkMAC">—</dd></dl></div><div class="connection-panel"><h3 data-i="setupConnection">Setup AP</h3><dl><dt data-i="connectionState">Status</dt><dd id="linkAP">—</dd><dt data-i="wifiName">Wi-Fi name (SSID)</dt><dd id="linkAPSSID">—</dd><dt data-i="deviceIP">LayerGlow IP</dt><dd id="linkAPIP">—</dd><dt data-i="apClients">Connected devices</dt><dd id="linkClients">—</dd><dt data-i="browserAddress">Current panel address</dt><dd id="linkBrowser">—</dd></dl></div></div><p class="muted" id="connectionInfoMessage"></p><form id="wifiForm"><h3 data-i="wifiSection">Wi-Fi</h3><label for="wifiList" data-i="scanWifi">Available 2.4 GHz Wi-Fi</label><div class="scanrow"><select id="wifiList" onchange="if(this.value)document.getElementById('ssid').value=this.value"><option data-i="scanFirst">Scan first</option></select><button type="button" class="alt" id="scanWifiButton" onclick="scanWifi()" data-i="scan">Scan</button></div><label for="ssid" data-i="wifiName">Wi-Fi name (SSID)</label><input id="ssid" name="ssid" maxlength="32"><label for="webHost" data-i="deviceName">Device name (.local)</label><input id="webHost" name="host" maxlength="38" spellcheck="false" autocomplete="off" placeholder="layerglow"><p class="muted" data-i="nameHint">For example, layerglow opens at http://layerglow.local/. Use letters, digits and hyphens. If .local does not work on your network, use the LayerGlow IP shown above.</p><label for="pass" data-i="wifiPass">Wi-Fi password (blank keeps saved)</label><input id="pass" name="pass" type="password" autocomplete="new-password"><div class="controls"><button data-i="saveNetwork" type="submit">Save &amp; Connect Wi-Fi</button></div><p class="muted" data-i="wifiChangeHint">To add a network, enter or select its name, enter the password and save. Use Connect below for a saved network. If none works, join LayerGlow-Setup and open 192.168.4.1.</p><p id="wifiMessage" class="notice"></p></form><section aria-labelledby="savedWifiTitle"><h3 id="savedWifiTitle" data-i="savedWifiTitle">Saved Wi-Fi networks</h3><p class="muted" data-i="savedWifiHint">Up to 8 networks. The device automatically tries saved networks when you change location. Passwords are never shown here.</p><div id="savedWifiList"></div><p id="savedWifiMessage" class="notice" aria-live="polite"></p></section></dialog><dialog id="printerDialog" aria-labelledby="printerTitle"><div class="dialog-head"><h2 id="printerTitle" data-i="printerSettings">Printer</h2><button type="button" class="ghost" id="closePrinter" data-i="close">Close</button></div><div class="connection-panel printer-connection" aria-live="polite"><h3 data-i="printerConnection">Printer</h3><dl><dt data-i="connectionState">Status</dt><dd id="linkPrinter">—</dd><dt data-i="printerType">Printer type</dt><dd id="linkPrinterType">—</dd><dt data-i="printerAddress">Printer IP : port</dt><dd id="linkPrinterAddress">—</dd><dt data-i="connectionReason">Details</dt><dd id="linkPrinterError">—</dd></dl><dl><dt data-i="printerLiveState">Printer state</dt><dd id="printerReadState">—</dd><dt data-i="printerLiveProgress">Print progress</dt><dd id="printerReadProgress">—</dd><dt data-i="printerLiveFile">Print file</dt><dd id="printerReadFile">—</dd><dt data-i="printerLiveAge">Last read</dt><dd id="printerReadAge">—</dd></dl></div><p class="muted" data-i="printerStatusHint">The status above comes from the printer, even while LED test mode is on.</p><hr><form id="printerForm"><h3 data-i="printerSection">Printer</h3><div class="controls"><button type="button" class="alt" id="findPrinters" data-i="findPrinters">Find printers</button><button type="button" class="ghost" id="disconnectPrinter" data-i="disconnectPrinter">Disconnect printer</button></div><label for="foundPrinters" data-i="foundPrinters">Printers on this network</label><select id="foundPrinters"><option value="" data-i="discoveryIdle">Press Search to find printers</option></select><div class="discovery-progress" id="discoveryProgress" hidden><progress id="discoveryBar" max="100" value="0" aria-label="Printer discovery progress"></progress><span id="discoveryPercent">0%</span></div><p class="muted" id="discoveryMessage" aria-live="polite"></p><p class="muted" data-i="discoveryHint">Selecting a printer fills in known details. Enter its LAN code or API key if required, then save the printer.</p><label for="printerType" data-i="printerType">Printer type</label><select id="printerType" name="printerType" onchange="printerTypeChanged()"><option value="moonraker" data-i="crealityK2">Creality / Klipper (Moonraker)</option><option value="bambu" data-i="bambuA1">Bambu Lab (LAN / MQTT)</option><option value="octoprint" data-i="octoType">OctoPrint (many brands)</option><option value="prusalink" data-i="prusaType">PrusaLink (Prusa)</option></select><label for="ip" data-i="printerIP">Printer IP address</label><input id="ip" name="ip" required><div id="portFields"><label for="port" data-i="printerPort">Printer API port</label><input id="port" name="port" type="number" min="1" max="65535" required></div><div id="bambuFields" style="display:none"><label for="bambuSerial" data-i="bambuSerial">Bambu serial number</label><input id="bambuSerial" name="bambuSerial" maxlength="40" autocomplete="off"><label for="bambuCode" data-i="bambuCode">LAN access code (blank keeps saved)</label><input id="bambuCode" name="bambuCode" type="password" maxlength="64" autocomplete="new-password"><p class="muted" data-i="bambuHint">Enable LAN Only and Developer Mode. Enter IP, serial number and LAN access code. Status is read locally.</p></div><div id="apiKeyFields" style="display:none"><label for="apiKey" data-i="apiKey">API key (blank keeps saved)</label><input id="apiKey" name="apiKey" type="password" maxlength="128" autocomplete="new-password"><p class="muted" data-i="apiHint">Enter the local API key for the selected printer service.</p></div><div class="controls"><button data-i="savePrinter" type="submit">Save printer</button></div><p id="printerMessage" class="notice"></p></form></dialog><dialog id="updateDialog" aria-labelledby="updateTitle"><div class="dialog-head"><h2 id="updateTitle" data-i="updateTitle">Firmware update · OTA</h2><button type="button" class="ghost" id="closeUpdate" data-i="close">Close</button></div><p class="muted" data-i="updateHint">Select the compiled LayerGlow .bin file. Update takes about 2 minutes. Keep power connected and do not close this page.</p><form id="fwform"><input required id="bin" type="file" accept=".bin,application/octet-stream"><div class="controls"><button type="submit" data-i="upload">Upload and restart</button><button type="button" class="alt" id="checkGitHub" data-i="checkUpdates">Check GitHub for updates</button><button type="button" id="installGitHub" data-i="installUpdate" hidden>Install update</button></div></form><p id="githubMessage" class="muted" aria-live="polite"></p><p id="fwMessage" class="notice"></p><p class="muted warning" data-i="security">The panel and OTA have no login. Use only on a trusted local network; do not expose it to the Internet.</p></dialog><dialog id="faqDialog" aria-labelledby="faqTitle"><div class="dialog-head"><h2 id="faqTitle" data-i="faqMenu">FAQ</h2><button type="button" class="ghost" id="closeFAQ" data-i="close">Close</button></div><h3 data-i="blueLedTitle">Blue LED on LayerGlow</h3><p class="muted" data-i="blueLedIntro">The short flashes repeat, followed by a longer pause:</p><div class="faq-signals"><div><strong>· —</strong><span data-i="blueLedNoWifi">One flash: no Wi-Fi connection</span></div><div><strong>·· —</strong><span data-i="blueLedWifi">Two flashes: connected to Wi-Fi</span></div><div><strong>— — ·</strong><span data-i="blueLedUpdate">Two long flashes, pause, one short flash: newer firmware on GitHub</span></div></div><p class="muted" data-i="faqUpdateHint">The panel checks GitHub while open or manually in Update.</p><p class="muted" data-i="simHint">The test also controls connected LEDs.</p></dialog><dialog id="languageDialog" aria-labelledby="languagesTitle"><div class="dialog-head"><h2 id="languagesTitle" data-i="languages">Languages</h2><button type="button" class="ghost" id="closeLang" data-i="close">Close</button></div><div class="language-options"><button type="button" class="alt" onclick="setLang('en');document.getElementById('languageDialog').close()">🇬🇧 English</button><button type="button" class="alt" onclick="setLang('pl');document.getElementById('languageDialog').close()">🇵🇱 Polski</button><button type="button" class="alt" onclick="setLang('es');document.getElementById('languageDialog').close()">🇪🇸 Español</button><button type="button" class="alt" onclick="setLang('de');document.getElementById('languageDialog').close()">🇩🇪 Deutsch</button></div></dialog></main><div class="toasts" id="toasts" aria-live="polite"></div><script>
const T={
en:{pulseSetting:'Pulse',pulseEnabled:'Enabled',pulseOn:'On',networkSettings:'Network',printerSettings:'Printer',disconnectPrinter:'Disconnect printer',printerDisabled:'Printer disconnected',printerLiveState:'Printer state',printerLiveProgress:'Print progress',printerLiveFile:'Print file',printerLiveAge:'Last read',printerStatusHint:'Status comes from the printer, even while LED test mode is on.',secondsAgo:'s ago',rescanPrinters:'Search again',selectThenSave:'Press Save printer below to connect to the selected printer.',savedWifiTitle:'Saved Wi-Fi networks',savedWifiHint:'Up to 8 networks. The device automatically tries saved networks when you change location. Passwords are never shown here.',savedWifiNone:'No saved networks yet',savedWifiConnected:'Connected',savedWifiTrying:'Selected',savedWifiConnect:'Connect',savedWifiRemove:'Remove',savedWifiConfirm:'Remove this saved Wi-Fi network?',savedWifiRemoved:'Network removed',savedWifiFull:'Up to 8 saved networks are allowed.',savedWifiInvalid:'Invalid saved network',deviceName:'Device name (.local)',friendlyAddress:'Easy address',nameHint:'For example, layerglow opens at http://layerglow.local/. Use letters, digits and hyphens. If .local does not work on your network, use the LayerGlow IP shown above.',namePending:'Waiting for mDNS / Wi-Fi',nameReady:'Available on home Wi-Fi',nameInvalid:'Use 1–32 letters, digits or hyphens; no spaces or leading/trailing hyphen.',openNewAddress:'Open the new address',homeConnection:'Home Wi-Fi / LayerGlow',setupConnection:'Setup AP',printerConnection:'Printer connection',connectionState:'Status',deviceIP:'LayerGlow IP',gatewayIP:'Router IP',subnetMask:'Subnet mask',dnsServer:'DNS server',wifiSignal:'Signal',wifiChannel:'Wi-Fi channel',deviceMAC:'LayerGlow MAC',apClients:'Connected devices',browserAddress:'Current panel address',printerAddress:'Printer IP : port',connectionReason:'Details',connectionOn:'Connected',connectionOff:'Disconnected',connectionActive:'Active',connectionInactive:'Inactive',connectionTest:'Test mode',connectionNoError:'No error',connectionUnavailable:'Connection details unavailable',connectionWaiting:'Waiting for printer response',languages:'Languages',addColour:'Add colour',removeColour:'Remove colour',findPrinters:'Find printers',foundPrinters:'Printers on this network',discoveryIdle:'Press Search to find printers',discoveryHint:'Select a printer to fill in its details. Enter its LAN code or API key if required, then save.',discoverySearching:'Searching local network… this can take a minute or two',discoveryNone:'No compatible printer found. You can still enter its address manually.',discoveryUnavailable:'Connect the LayerGlow to your home Wi-Fi to find printers.',discoveryError:'Printer search failed. Enter the address manually.',discoverySelect:'Select a printer',discoveryReady:'Found printers: ',discoveryCredentials:'Enter the LAN code or API key on the printer before saving.',appName:"LayerGlow",colourFlow:"Colour flow",colourFade:"Colour fade",rainbowFlow:"Rainbow flow",colourMarch:"Moving colour bands",colourEffectHint:"Choose at least two colours for palette animations. Rainbow uses its own spectrum. These effects run without Pulse; Cycle controls their speed.",colourCount:"Colours",stripCountHint:"Total connected LEDs: 5–300. Two 12-LED strips in one chain = 24. Longer strips need suitable power and wiring.",paletteHint:"Choose 1–12 colours per stage. Colours form equal zones along the strip; printing reveals them with progress.",wifiSection:'Wi-Fi',saveNetwork:'Save & Connect Wi-Fi',printerSection:'Printer',savePrinter:'Save printer',printerSaved:'Printer settings saved',printerPort:'Printer API port',apiKey:'API key (blank keeps saved)',apiHint:'Enter the local API key for the selected printer service.',octoType:'OctoPrint (many brands)',prusaType:'PrusaLink (Prusa)',bambuDenied:"Bambu rejected the LAN access code",printerType:"Printer type",crealityK2:"Creality / Klipper (Moonraker)",bambuA1:"Bambu Lab (LAN / MQTT)",bambuSerial:"Printer serial number",bambuCode:"LAN access code (blank keeps saved)",bambuHint:"On the A1 enable LAN Only and Developer Mode. Enter its IP, serial number and LAN access code. Status is read locally.",invalidBambu:"Invalid Bambu serial or access code",bambuOffline:"Bambu connection unavailable",bambuWait:"Waiting for Bambu status",scanNone:"No 2.4 GHz networks found. You can enter the SSID manually.",backupLights:"Save my LED backup",restoreLights:"Restore my LED backup",backupSaved:"LED backup saved in the device",backupRestored:"LED backup restored",comet:'Comet',wipe:'Moving fill',alternating:'Alternating pairs',randomPairs:'Random pairs',randomHint:'Pairs light up and turn off at random intervals.',ledOn:'On',faqMenu:'FAQ',blueLedTitle:'Blue LED on LayerGlow',blueLedIntro:'The signal repeats with a pause between cycles:',blueLedNoWifi:'One flash: no Wi-Fi connection',blueLedWifi:'Two flashes: connected to Wi-Fi',blueLedUpdate:'Two long flashes, pause, one short flash: newer firmware on GitHub',faqUpdateHint:'The panel checks GitHub when opened and every 30 minutes while it is open. You can also check manually in Update.',githubMissing:'No OTA .bin file found in the Software folder on GitHub.',githubNetwork:'Could not reach GitHub. Check Internet access.',githubFormat:'GitHub returned an invalid update listing.',updateMenu:'Update',networkSaved:'Network settings saved',networkRestart:'Saved. Switching to the new Wi-Fi. If it fails, connect to LayerGlow-Setup.',invalidMoonraker:'Invalid Moonraker URL',missingFields:'Missing printer fields',invalidNetwork:'Invalid IP, port or SSID',invalidLedCount:'Invalid LED count',invalidLedRow:'Invalid LED settings',noSpace:'Not enough OTA space. Flash once via USB with the min_spiffs partition layout.',led24:'24 LEDs · 12 pairs',led12:'12 LEDs · 6 pairs',moonPort:'Moonraker port',setupOffline:'No printer connection (setup AP)',checkUpdates:'Check GitHub for updates',installUpdate:'Install update',checkingUpdates:'Checking GitHub…',noUpdate:'Firmware is up to date',newVersion:'New version available:',githubUnavailable:'Could not check GitHub. Check Internet access and the Software/update.json file.',downloadFirmware:'Downloading firmware from GitHub…',invalidFirmware:'Firmware download does not match the update manifest.',githubConfirm:'Install version {version}? This can take about 2 minutes. Keep power connected and this page open.',centreOut:'Centre → edges → centre',edgesIn:'Edges → centre → edges',wave:'Flowing wave',restarting:'Uploading and restarting…',waiting:'Waiting for LayerGlow to return…',retry:'Could not confirm the restart. Check the device and reload the page.',deviceOffline:'Connection to LayerGlow lost',function:'Function',colour:'Colour',pulsing:'Pulse?',effect:'Effect',allBar:'Whole bar',lastPair:'Last pair',scanner:'Scanner (Knight Rider)',close:'Close',preview:'LED preview and test',testMode:'Test mode',testStatus:'Test state',testProgress:'Test progress',printing:'Printing',heating:'Heating',paused:'Paused',complete:'Completed',error:'Error',standby:'Idle',run:'▶ Run test',stop:'■ Stop',reset:'Reset',loopTest:'Repeat test after 100%',simHint:'The test also controls connected LEDs.',networkSettings:'Network',scanWifi:'Available 2.4 GHz Wi-Fi',scanFirst:'Scan first',scan:'Scan',wifiName:'Wi-Fi name (SSID)',wifiPass:'Wi-Fi password (blank keeps saved)',printerIP:'Printer IP address',lighting:'Colours and LED settings',ledCount:'Physical LEDs',brightness:'Brightness',stepTime:'Test step (ms)',pulseMode:'Pulsing mode',pulseOff:'Off',pulseAll:'All lit LEDs',pulseLast:'Last lit pair during printing',pulsePeriod:'Cycle / speed (ms)',powerHint:'Check LED power wiring before increasing brightness.',resetColors:'Restore original colours',saving:'Saving…',wifiChangeHint:'To add a network, enter or select its name, enter the password and save. Use Connect below for a saved network. If none works, join LayerGlow-Setup and open 192.168.4.1.',pulseStages:'Pulse in these states (off by default)',pulseLastPair:'Pulse only the last lit pair while printing',autoProgress:'Play progress automatically',resetLights:'Restore default LED settings',saveLights:'Save LED settings',updateTitle:'Firmware update · OTA',updateHint:'Select the compiled LayerGlow .bin file. Update takes about 2 minutes. Keep power connected and do not close this page.',upload:'Upload and restart',security:'The panel and OTA have no login. Use only on a trusted local network; do not expose it to the Internet.',credits:'Designed by',progressColor:'Print progress',colour_printing:'Printing',colour_heating:'Heating',colour_paused:'Paused',colour_complete:'Finished',colour_error:'Error',colour_standby:'Idle',saved:'Settings saved',scanError:'Scan failed',updateDone:'Firmware uploaded; device restarting…',updateFailed:'Upload failed',offline:'Printer offline',connected:'Printer connected',setup:'Setup access point',home:'Local Wi-Fi',test:'TEST',uploadConfirm:'Update can take about 2 minutes. Keep power connected and do not close this page. Upload firmware now?',chooseBin:'Select a .bin file',scanning:'Scanning…'},
pl:{pulseSetting:'Puls',pulseEnabled:'Włączone',pulseOn:'Włączony',networkSettings:'Sieć',printerSettings:'Drukarka',disconnectPrinter:'Rozłącz drukarkę',printerDisabled:'Drukarka rozłączona',printerLiveState:'Stan drukarki',printerLiveProgress:'Postęp druku',printerLiveFile:'Plik wydruku',printerLiveAge:'Ostatni odczyt',printerStatusHint:'Stan pochodzi z drukarki także wtedy, gdy działa test LED.',secondsAgo:'s temu',rescanPrinters:'Szukaj ponownie',selectThenSave:'Aby połączyć wybraną drukarkę, użyj przycisku Zapisz drukarkę poniżej.',savedWifiTitle:'Zapisane sieci Wi-Fi',savedWifiHint:'Maksymalnie 8 sieci. Po zmianie lokalizacji urządzenie samo próbuje połączyć się z zapamiętanymi sieciami. Hasła nie są tutaj pokazywane.',savedWifiNone:'Nie ma zapisanych sieci',savedWifiConnected:'Połączono',savedWifiTrying:'Wybrana',savedWifiConnect:'Połącz',savedWifiRemove:'Usuń',savedWifiConfirm:'Usunąć tę zapisaną sieć Wi-Fi?',savedWifiRemoved:'Sieć usunięta',savedWifiFull:'Można zapisać najwyżej 8 sieci.',savedWifiInvalid:'Nieprawidłowa zapisana sieć',deviceName:'Nazwa urządzenia (.local)',friendlyAddress:'Prosty adres',nameHint:'Na przykład layerglow otwiera się przez http://layerglow.local/. Użyj liter, cyfr i myślników. Jeśli .local nie działa w Twojej sieci, skorzystaj z adresu IP LayerGlow powyżej.',namePending:'Czekam na mDNS / Wi-Fi',nameReady:'Dostępny w domowej sieci Wi-Fi',nameInvalid:'Użyj 1–32 liter, cyfr lub myślników; bez spacji i myślnika na początku lub końcu.',openNewAddress:'Otwórz nowy adres',homeConnection:'Domowe Wi-Fi / LayerGlow',setupConnection:'Sieć awaryjna AP',printerConnection:'Połączenie z drukarką',connectionState:'Stan',deviceIP:'Adres IP LayerGlow',gatewayIP:'Adres IP routera',subnetMask:'Maska podsieci',dnsServer:'Serwer DNS',wifiSignal:'Sygnał',wifiChannel:'Kanał Wi-Fi',deviceMAC:'Adres MAC LayerGlow',apClients:'Połączone urządzenia',browserAddress:'Obecny adres panelu',printerAddress:'Adres IP : port drukarki',connectionReason:'Szczegóły',connectionOn:'Połączono',connectionOff:'Rozłączono',connectionActive:'Aktywna',connectionInactive:'Wyłączona',connectionTest:'Tryb testowy',connectionNoError:'Brak błędu',connectionUnavailable:'Dane połączenia niedostępne',connectionWaiting:'Czekam na odpowiedź drukarki',languages:'Języki',addColour:'Dodaj kolor',removeColour:'Usuń kolor',findPrinters:'Znajdź drukarki',foundPrinters:'Drukarki w tej sieci',discoveryIdle:'Wyszukiwanie rozpocznie się po otwarciu okna',discoveryHint:'Wybierz drukarkę, aby uzupełnić dane. Wpisz kod LAN lub klucz API, jeśli jest wymagany, i zapisz.',discoverySearching:'Szukam w sieci lokalnej… to może potrwać minutę lub dwie',discoveryNone:'Nie znaleziono zgodnej drukarki. Nadal możesz wpisać adres ręcznie.',discoveryUnavailable:'Aby wykryć drukarki, połącz LayerGlow z domowym Wi-Fi.',discoveryError:'Wyszukiwanie nie powiodło się. Wpisz adres ręcznie.',discoverySelect:'Wybierz drukarkę',discoveryReady:'Znalezione drukarki: ',discoveryCredentials:'Przed zapisaniem wpisz kod LAN lub klucz API drukarki.',appName:"LayerGlow",colourFlow:"Pływające kolory",colourFade:"Przenikanie kolorów",rainbowFlow:"Płynąca tęcza",colourMarch:"Wędrujące strefy kolorów",colourEffectHint:"Do animacji palety wybierz co najmniej dwa kolory. Tęcza używa własnego widma. Te efekty działają bez Pulsowania; Cykl ustawia ich szybkość.",colourCount:"Kolory",stripCountHint:"Łączna liczba podłączonych LED: 5–300. Dwa paski po 12 LED w jednym łańcuchu = 24. Dłuższe paski wymagają odpowiedniego zasilania i przewodów.",paletteHint:"Wybierz 1–12 kolorów dla każdego etapu. Kolory tworzą równe strefy na pasku, odsłaniane wraz z postępem drukowania.",wifiSection:'Sieć Wi-Fi',saveNetwork:'Zapisz i połącz Wi-Fi',printerSection:'Drukarka',savePrinter:'Zapisz drukarkę',printerSaved:'Zapisano ustawienia drukarki',printerPort:'Port API drukarki',apiKey:'Klucz API (puste pole zachowa zapisany)',apiHint:'Wpisz lokalny klucz API wybranej usługi drukarki.',octoType:'OctoPrint (różne marki)',prusaType:'PrusaLink (Prusa)',bambuDenied:"Bambu odrzuciła kod dostępu LAN",printerType:"Typ drukarki",crealityK2:"Creality / Klipper (Moonraker)",bambuA1:"Bambu Lab (LAN / MQTT)",bambuSerial:"Numer seryjny drukarki",bambuCode:"Kod dostępu LAN (puste pole zachowa zapisany)",bambuHint:"Na A1 włącz LAN Only i Developer Mode. Wpisz adres IP, numer seryjny i kod dostępu LAN. Status jest pobierany lokalnie.",invalidBambu:"Nieprawidłowy numer seryjny lub kod Bambu",bambuOffline:"Brak połączenia z Bambu",bambuWait:"Oczekiwanie na status Bambu",scanNone:"Nie znaleziono sieci 2,4 GHz. Możesz wpisać nazwę sieci ręcznie.",backupLights:"Zapisz moją kopię ustawień LED",restoreLights:"Przywróć moją kopię ustawień LED",backupSaved:"Zapisano kopię ustawień LED w urządzeniu",backupRestored:"Przywrócono kopię ustawień LED",comet:'Kometa',wipe:'Wędrujące wypełnienie',alternating:'Pary naprzemiennie',randomPairs:'Losowe pary',randomHint:'Pary zapalają się i gasną niezależnie w losowych, krótkich odstępach.',ledOn:'Świeci',faqMenu:'FAQ',blueLedTitle:'Niebieska dioda LayerGlow',blueLedIntro:'Sygnał powtarza się z przerwą między cyklami:',blueLedNoWifi:'Jeden błysk: brak połączenia z Wi-Fi',blueLedWifi:'Dwa błyski: połączono z Wi-Fi',blueLedUpdate:'Dwa długie błyski, przerwa, jeden krótki: nowa wersja na GitHubie',faqUpdateHint:'Panel sprawdza GitHub po otwarciu i co 30 minut, kiedy jest otwarty. Możesz też sprawdzić ręcznie w Aktualizacjach.',githubMissing:'W katalogu Software na GitHubie nie znaleziono pliku OTA .bin.',githubNetwork:'Nie udało się połączyć z GitHubem. Sprawdź dostęp do Internetu.',githubFormat:'GitHub zwrócił nieprawidłową listę aktualizacji.',updateMenu:'Aktualizacja',networkSaved:'Zapisano ustawienia sieci',networkRestart:'Zapisano. Łączenie z nową siecią Wi-Fi. W razie problemu użyj LayerGlow-Setup.',invalidMoonraker:'Nieprawidłowy adres Moonraker',missingFields:'Brakuje danych drukarki',invalidNetwork:'Nieprawidłowy adres IP, port lub SSID',invalidLedCount:'Nieprawidłowa liczba diod LED',invalidLedRow:'Nieprawidłowe ustawienia LED',noSpace:'Za mało miejsca na OTA. Wgraj raz przez USB z układem partycji min_spiffs.',led24:'24 diody LED · 12 par',led12:'12 diod LED · 6 par',moonPort:'Port Moonraker',setupOffline:'Brak połączenia z drukarką (sieć konfiguracyjna)',checkUpdates:'Sprawdź aktualizacje na GitHubie',installUpdate:'Zainstaluj aktualizację',checkingUpdates:'Sprawdzam GitHub…',noUpdate:'Oprogramowanie jest aktualne',newVersion:'Dostępna nowa wersja:',githubUnavailable:'Nie udało się sprawdzić GitHuba. Sprawdź Internet i plik Software/update.json.',downloadFirmware:'Pobieram firmware z GitHuba…',invalidFirmware:'Pobrany firmware nie zgadza się z manifestem aktualizacji.',githubConfirm:'Zainstalować wersję {version}? Może to potrwać około 2 minut. Nie odłączaj zasilania ani nie zamykaj strony.',centreOut:'Środek → boki → środek',edgesIn:'Boki → środek → boki',wave:'Płynąca fala',restarting:'Wgrywanie i uruchamianie ponownie…',waiting:'Czekam na ponowne połączenie z LayerGlow…',retry:'Nie mogę potwierdzić restartu. Sprawdź urządzenie i odśwież stronę.',deviceOffline:'Utracono połączenie z LayerGlow',function:'Funkcja',colour:'Kolor',pulsing:'Pulsowanie?',effect:'Efekt',allBar:'Cały pasek',lastPair:'Ostatnia para',scanner:'Wędrujące światło (Knight Rider)',close:'Zamknij',preview:'Podgląd i test LED',testMode:'Tryb testowy',testStatus:'Stan testowy',testProgress:'Postęp testu',printing:'Drukowanie',heating:'Nagrzewanie',paused:'Pauza',complete:'Zakończono',error:'Błąd',standby:'Bezczynność',run:'▶ Uruchom test',stop:'■ Zatrzymaj',reset:'Resetuj',loopTest:'Powtarzaj test po 100%',simHint:'Test steruje także podłączonym paskiem LED.',networkSettings:'Sieć',scanWifi:'Dostępne Wi-Fi 2,4 GHz',scanFirst:'Najpierw skanuj',scan:'Skanuj',wifiName:'Nazwa Wi-Fi (SSID)',wifiPass:'Hasło Wi-Fi (puste zachowuje zapisane)',printerIP:'Adres IP drukarki',lighting:'Kolory i ustawienia LED',ledCount:'Fizyczne diody LED',brightness:'Jasność',stepTime:'Krok testu (ms)',pulseMode:'Tryb pulsowania',pulseOff:'Wyłączone',pulseAll:'Wszystkie zapalone diody',pulseLast:'Ostatnia zapalona para podczas druku',pulsePeriod:'Cykl / szybkość (ms)',powerHint:'Sprawdź zasilanie LED przed zwiększeniem jasności.',resetColors:'Przywróć oryginalne kolory',saving:'Zapisywanie…',wifiChangeHint:'Aby dodać sieć, wybierz ją lub wpisz nazwę, podaj hasło i zapisz. Przycisk Połącz poniżej pozwala ręcznie wybrać zapisaną sieć. Jeśli żadna nie działa, połącz się z LayerGlow-Setup i otwórz 192.168.4.1.',pulseStages:'Pulsuj w tych stanach (domyślnie wyłączone)',pulseLastPair:'Pulsuj tylko ostatnią zapaloną parę podczas druku',autoProgress:'Automatycznie odtwarzaj postęp',resetLights:'Przywróć domyślne ustawienia LED',saveLights:'Zapisz ustawienia LED',updateTitle:'Aktualizacja oprogramowania · OTA',updateHint:'Wybierz plik .bin dla LayerGlow. Aktualizacja trwa około 2 minut. Nie odłączaj zasilania ani nie zamykaj tej strony.',upload:'Wgraj i uruchom ponownie',security:'Panel i OTA nie mają logowania. Używaj tylko w zaufanej sieci lokalnej; nie udostępniaj w Internecie.',credits:'Projekt:',progressColor:'Postęp druku',colour_printing:'Drukowanie',colour_heating:'Nagrzewanie',colour_paused:'Pauza',colour_complete:'Zakończono',colour_error:'Błąd',colour_standby:'Bezczynność',saved:'Zapisano ustawienia',scanError:'Błąd skanowania',updateDone:'Wgrano firmware; urządzenie uruchamia się ponownie…',updateFailed:'Błąd aktualizacji',offline:'Brak połączenia z drukarką',connected:'Połączono z drukarką',setup:'Sieć konfiguracyjna',home:'Domowe Wi-Fi',test:'TEST',uploadConfirm:'Aktualizacja może potrwać około 2 minut. Nie odłączaj zasilania ani nie zamykaj tej strony. Wgrać firmware?',chooseBin:'Wybierz plik .bin',scanning:'Skanowanie…'},
es:{pulseSetting:'Pulso',pulseEnabled:'Activado',pulseOn:'Activado',networkSettings:'Red',printerSettings:'Impresora',disconnectPrinter:'Desconectar impresora',printerDisabled:'Impresora desconectada',printerLiveState:'Estado de la impresora',printerLiveProgress:'Progreso de impresión',printerLiveFile:'Archivo de impresión',printerLiveAge:'Última lectura',printerStatusHint:'El estado procede de la impresora incluso durante la prueba LED.',secondsAgo:'s atrás',rescanPrinters:'Buscar de nuevo',selectThenSave:'Pulsa Guardar impresora para conectar la impresora seleccionada.',savedWifiTitle:'Redes Wi-Fi guardadas',savedWifiHint:'Hasta 8 redes. El dispositivo prueba automáticamente las redes guardadas al cambiar de ubicación. Las contraseñas no aparecen aquí.',savedWifiNone:'No hay redes guardadas',savedWifiConnected:'Conectada',savedWifiTrying:'Seleccionada',savedWifiConnect:'Conectar',savedWifiRemove:'Eliminar',savedWifiConfirm:'¿Eliminar esta red Wi-Fi guardada?',savedWifiRemoved:'Red eliminada',savedWifiFull:'Se pueden guardar hasta 8 redes.',savedWifiInvalid:'Red guardada no válida',deviceName:'Nombre del dispositivo (.local)',friendlyAddress:'Dirección fácil',nameHint:'Por ejemplo, layerglow abre http://layerglow.local/. Usa letras, números y guiones. Si .local no funciona en tu red, usa la IP del LayerGlow indicada arriba.',namePending:'Esperando mDNS / Wi-Fi',nameReady:'Disponible en la Wi-Fi doméstica',nameInvalid:'Usa 1–32 letras, números o guiones; sin espacios ni guión al principio o final.',openNewAddress:'Abrir la nueva dirección',homeConnection:'Wi-Fi doméstica / LayerGlow',setupConnection:'Red AP de configuración',printerConnection:'Conexión con la impresora',connectionState:'Estado',deviceIP:'IP del LayerGlow',gatewayIP:'IP del router',subnetMask:'Máscara de subred',dnsServer:'Servidor DNS',wifiSignal:'Señal',wifiChannel:'Canal Wi-Fi',deviceMAC:'MAC del LayerGlow',apClients:'Dispositivos conectados',browserAddress:'Dirección actual del panel',printerAddress:'IP : puerto de la impresora',connectionReason:'Detalles',connectionOn:'Conectado',connectionOff:'Desconectado',connectionActive:'Activo',connectionInactive:'Inactivo',connectionTest:'Modo prueba',connectionNoError:'Sin errores',connectionUnavailable:'Datos de conexión no disponibles',connectionWaiting:'Esperando respuesta de la impresora',languages:'Idiomas',addColour:'Añadir color',removeColour:'Quitar color',findPrinters:'Buscar impresoras',foundPrinters:'Impresoras en esta red',discoveryIdle:'La búsqueda comienza al abrir esta ventana',discoveryHint:'Selecciona una impresora para completar los datos. Introduce el código LAN o la clave API si hace falta y guarda.',discoverySearching:'Buscando en la red local… puede tardar uno o dos minutos',discoveryNone:'No se encontró una impresora compatible. Puedes introducir la dirección manualmente.',discoveryUnavailable:'Conecta el LayerGlow a la Wi-Fi doméstica para buscar impresoras.',discoveryError:'Falló la búsqueda. Introduce la dirección manualmente.',discoverySelect:'Selecciona una impresora',discoveryReady:'Impresoras encontradas: ',discoveryCredentials:'Introduce el código LAN o la clave API antes de guardar.',appName:"LayerGlow",colourFlow:"Flujo de colores",colourFade:"Transición de colores",rainbowFlow:"Arcoíris en movimiento",colourMarch:"Bandas de color móviles",colourEffectHint:"Elige al menos dos colores para animar la paleta. El arcoíris usa su propio espectro. No requieren pulsación; Ciclo controla la velocidad.",colourCount:"Colores",stripCountHint:"Total de LED conectados: 5–300. Dos tiras de 12 LED en una cadena = 24. Las tiras largas requieren alimentación y cableado adecuados.",paletteHint:"Elige 1–12 colores por etapa. Forman zonas iguales en la tira, que se iluminan con el progreso de impresión.",wifiSection:'Wi-Fi',saveNetwork:'Guardar y conectar Wi-Fi',printerSection:'Impresora',savePrinter:'Guardar impresora',printerSaved:'Ajustes de impresora guardados',printerPort:'Puerto API de impresora',apiKey:'Clave API (vacío conserva la guardada)',apiHint:'Introduce la clave API local del servicio de impresión.',octoType:'OctoPrint (varias marcas)',prusaType:'PrusaLink (Prusa)',bambuDenied:"Bambu rechazó el código de acceso LAN",printerType:"Tipo de impresora",crealityK2:"Creality / Klipper (Moonraker)",bambuA1:"Bambu Lab (LAN / MQTT)",bambuSerial:"Número de serie de A1",bambuCode:"Código de acceso LAN (vacío conserva el guardado)",bambuHint:"En la A1 activa LAN Only y Developer Mode. Introduce IP, número de serie y código LAN. El estado se obtiene localmente.",invalidBambu:"Número de serie o código Bambu incorrecto",bambuOffline:"Sin conexión con Bambu",bambuWait:"Esperando el estado de Bambu",scanNone:"No se encontraron redes de 2,4 GHz. Puedes escribir el SSID manualmente.",backupLights:"Guardar mi copia de LED",restoreLights:"Restaurar mi copia de LED",backupSaved:"Copia de LED guardada en el dispositivo",backupRestored:"Copia de LED restaurada",comet:'Cometa',wipe:'Relleno en movimiento',alternating:'Pares alternos',randomPairs:'Pares aleatorios',randomHint:'Las parejas se encienden y apagan en intervalos cortos y aleatorios.',ledOn:'Encendido',faqMenu:'Preguntas',blueLedTitle:'LED azul del LayerGlow',blueLedIntro:'La señal se repite con una pausa entre ciclos:',blueLedNoWifi:'Un destello: sin conexión Wi-Fi',blueLedWifi:'Dos destellos: conectado a Wi-Fi',blueLedUpdate:'Dos destellos largos, pausa, uno corto: firmware nuevo en GitHub',faqUpdateHint:'El panel consulta GitHub al abrirse y cada 30 minutos mientras está abierto. También puedes buscar manualmente en Actualizar.',githubMissing:'No se encontró ningún OTA .bin en Software de GitHub.',githubNetwork:'No se pudo conectar a GitHub. Comprueba Internet.',githubFormat:'GitHub devolvió una lista de actualizaciones no válida.',updateMenu:'Actualizar',networkSaved:'Ajustes de red guardados',networkRestart:'Guardado. Conectando a la nueva red Wi-Fi. Si falla, usa LayerGlow-Setup.',invalidMoonraker:'URL de Moonraker no válida',missingFields:'Faltan datos de la impresora',invalidNetwork:'IP, puerto o SSID no válidos',invalidLedCount:'Número de LED no válido',invalidLedRow:'Ajustes LED no válidos',noSpace:'No hay espacio OTA suficiente. Graba una vez por USB con la partición min_spiffs.',led24:'24 LED · 12 pares',led12:'12 LED · 6 pares',moonPort:'Puerto Moonraker',setupOffline:'Sin conexión con la impresora (red de configuración)',checkUpdates:'Buscar actualizaciones en GitHub',installUpdate:'Instalar actualización',checkingUpdates:'Consultando GitHub…',noUpdate:'El firmware está actualizado',newVersion:'Nueva versión disponible:',githubUnavailable:'No se pudo consultar GitHub. Comprueba Internet y Software/update.json.',downloadFirmware:'Descargando firmware de GitHub…',invalidFirmware:'El firmware descargado no coincide con el manifiesto.',githubConfirm:'¿Instalar la versión {version}? Puede tardar unos 2 minutos. Mantén la alimentación y la página abierta.',centreOut:'Centro → extremos → centro',edgesIn:'Extremos → centro → extremos',wave:'Ola en movimiento',restarting:'Subiendo y reiniciando…',waiting:'Esperando a que vuelva el LayerGlow…',retry:'No se pudo confirmar el reinicio. Comprueba el dispositivo y recarga la página.',deviceOffline:'Se perdió la conexión con LayerGlow',function:'Función',colour:'Color',pulsing:'¿Pulsar?',effect:'Efecto',allBar:'Toda la barra',lastPair:'Último par',scanner:'Luz móvil (Knight Rider)',close:'Cerrar',preview:'Vista previa y prueba LED',testMode:'Modo de prueba',testStatus:'Estado de prueba',testProgress:'Progreso de prueba',printing:'Imprimiendo',heating:'Calentando',paused:'En pausa',complete:'Finalizado',error:'Error',standby:'En espera',run:'▶ Iniciar prueba',stop:'■ Detener',reset:'Restablecer',loopTest:'Repetir al llegar al 100%',simHint:'La prueba también controla los LED conectados.',networkSettings:'Red',scanWifi:'Redes Wi-Fi de 2,4 GHz',scanFirst:'Escanear primero',scan:'Escanear',wifiName:'Nombre Wi-Fi (SSID)',wifiPass:'Contraseña Wi-Fi (vacío conserva la guardada)',printerIP:'Dirección IP de la impresora',lighting:'Colores y ajustes LED',ledCount:'LED físicos',brightness:'Brillo',stepTime:'Paso de prueba (ms)',pulseMode:'Modo de pulsación',pulseOff:'Desactivado',pulseAll:'Todos los LED encendidos',pulseLast:'Último par encendido al imprimir',pulsePeriod:'Ciclo / velocidad (ms)',powerHint:'Comprueba la alimentación antes de aumentar el brillo.',resetColors:'Restaurar colores originales',saving:'Guardando…',wifiChangeHint:'Para añadir una red, selecciona o introduce su nombre y contraseña y guarda. Usa Conectar para elegir una red guardada. Si ninguna funciona, usa LayerGlow-Setup en 192.168.4.1.',pulseStages:'Pulsar en estos estados (desactivado por defecto)',pulseLastPair:'Pulsar solo el último par encendido al imprimir',autoProgress:'Reproducir el progreso automáticamente',resetLights:'Restaurar ajustes LED predeterminados',saveLights:'Guardar ajustes LED',updateTitle:'Actualizar firmware · OTA',updateHint:'Selecciona el .bin para LayerGlow. La actualización tarda unos 2 minutos. Mantén la alimentación y no cierres esta página.',upload:'Subir y reiniciar',security:'El panel y OTA no tienen contraseña. Úsalos solo en una red local de confianza; no los expongas a Internet.',credits:'Diseñado por',progressColor:'Progreso de impresión',colour_printing:'Imprimiendo',colour_heating:'Calentando',colour_paused:'En pausa',colour_complete:'Finalizado',colour_error:'Error',colour_standby:'En espera',saved:'Ajustes guardados',scanError:'Error al escanear',updateDone:'Firmware cargado; reiniciando…',updateFailed:'Error al subir',offline:'Impresora desconectada',connected:'Impresora conectada',setup:'Punto de acceso',home:'Wi-Fi local',test:'PRUEBA',uploadConfirm:'La actualización puede tardar unos 2 minutos. Mantén la alimentación y no cierres esta página. ¿Subir firmware?',chooseBin:'Selecciona un archivo .bin',scanning:'Escaneando…'},
de:{pulseSetting:'Pulsieren',pulseEnabled:'Aktiv',pulseOn:'Ein',networkSettings:'Netzwerk',printerSettings:'Drucker',disconnectPrinter:'Drucker trennen',printerDisabled:'Drucker getrennt',printerLiveState:'Druckerstatus',printerLiveProgress:'Druckfortschritt',printerLiveFile:'Druckdatei',printerLiveAge:'Letzter Abruf',printerStatusHint:'Der Status kommt auch im LED-Testmodus vom Drucker.',secondsAgo:'s zuvor',rescanPrinters:'Erneut suchen',selectThenSave:'Unten Drucker speichern wählen, um den ausgewählten Drucker zu verbinden.',savedWifiTitle:'Gespeicherte WLANs',savedWifiHint:'Bis zu 8 Netzwerke. Bei einem Standortwechsel versucht das Gerät automatisch, sich mit gespeicherten Netzwerken zu verbinden. Passwörter werden hier nicht angezeigt.',savedWifiNone:'Noch keine gespeicherten Netzwerke',savedWifiConnected:'Verbunden',savedWifiTrying:'Ausgewählt',savedWifiConnect:'Verbinden',savedWifiRemove:'Entfernen',savedWifiConfirm:'Dieses gespeicherte WLAN entfernen?',savedWifiRemoved:'Netzwerk entfernt',savedWifiFull:'Es können höchstens 8 Netzwerke gespeichert werden.',savedWifiInvalid:'Ungültiges gespeichertes Netzwerk',deviceName:'Gerätename (.local)',friendlyAddress:'Einfacher Adresse',nameHint:'Beispiel: layerglow öffnet http://layerglow.local/. Buchstaben, Zahlen und Bindestriche verwenden. Falls .local im Netz nicht funktioniert, die LayerGlow-IP oben nutzen.',namePending:'Warte auf mDNS / WLAN',nameReady:'Im Heim-WLAN verfügbar',nameInvalid:'1–32 Buchstaben, Zahlen oder Bindestriche; keine Leerzeichen und kein Bindestrich am Anfang/Ende.',openNewAddress:'Neue Adresse öffnen',homeConnection:'Heim-WLAN / LayerGlow',setupConnection:'Einrichtungs-AP',printerConnection:'Druckerverbindung',connectionState:'Status',deviceIP:'LayerGlow-IP',gatewayIP:'Router-IP',subnetMask:'Subnetzmaske',dnsServer:'DNS-Server',wifiSignal:'Signal',wifiChannel:'WLAN-Kanal',deviceMAC:'LayerGlow-MAC',apClients:'Verbundene Geräte',browserAddress:'Aktuelle Panel-Adresse',printerAddress:'Drucker-IP : Port',connectionReason:'Details',connectionOn:'Verbunden',connectionOff:'Getrennt',connectionActive:'Aktiv',connectionInactive:'Inaktiv',connectionTest:'Testmodus',connectionNoError:'Kein Fehler',connectionUnavailable:'Verbindungsdaten nicht verfügbar',connectionWaiting:'Warte auf Antwort des Druckers',languages:'Sprachen',addColour:'Farbe hinzufügen',removeColour:'Farbe entfernen',findPrinters:'Drucker suchen',foundPrinters:'Drucker in diesem Netz',discoveryIdle:'Suchen drücken, um Drucker zu finden',discoveryHint:'Drucker auswählen, um bekannte Daten einzutragen. Falls nötig, LAN-Code oder API-Schlüssel eingeben und speichern.',discoverySearching:'Suche im lokalen Netz… dies kann ein bis zwei Minuten dauern',discoveryNone:'Kein kompatibler Drucker gefunden. Adresse kann manuell eingetragen werden.',discoveryUnavailable:'LayerGlow mit dem Heim-WLAN verbinden, um Drucker zu finden.',discoveryError:'Druckersuche fehlgeschlagen. Adresse manuell eingeben.',discoverySelect:'Drucker auswählen',discoveryReady:'Gefundene Drucker: ',discoveryCredentials:'Vor dem Speichern LAN-Code oder API-Schlüssel eingeben.',appName:"LayerGlow",colourFlow:"Fließende Farben",colourFade:"Farbübergang",rainbowFlow:"Fließender Regenbogen",colourMarch:"Wandernde Farbzonen",colourEffectHint:"Für Palettenanimationen mindestens zwei Farben wählen. Der Regenbogen nutzt sein eigenes Spektrum. Pulsieren ist optional; Zyklus regelt das Tempo.",colourCount:"Farben",stripCountHint:"Gesamtzahl angeschlossener LEDs: 5–300. Zwei Streifen mit je 12 LEDs in einer Kette = 24. Längere Streifen brauchen passende Stromversorgung und Kabel.",paletteHint:"Pro Status 1–12 Farben wählen. Sie bilden gleich große Zonen, die mit dem Druckfortschritt aufleuchten.",wifiSection:'WLAN',saveNetwork:'WLAN speichern und verbinden',printerSection:'Drucker',savePrinter:'Drucker speichern',printerSaved:'Druckereinstellungen gespeichert',printerPort:'Drucker-API-Port',apiKey:'API-Schlüssel (leer behält gespeicherten)',apiHint:'Lokalen API-Schlüssel des Druckerdienstes eingeben.',octoType:'OctoPrint (viele Marken)',prusaType:'PrusaLink (Prusa)',bambuDenied:"Bambu hat den LAN-Zugangscode abgelehnt",printerType:"Druckertyp",crealityK2:"Creality / Klipper (Moonraker)",bambuA1:"Bambu Lab (LAN / MQTT)",bambuSerial:"Drucker-Seriennummer",bambuCode:"LAN-Zugangscode (leer behält gespeicherten Code)",bambuHint:"Auf dem A1 LAN Only und Developer Mode aktivieren. IP, Seriennummer und LAN-Code eingeben. Status wird lokal gelesen.",invalidBambu:"Ungültige Bambu-Seriennummer oder Zugangscode",bambuOffline:"Keine Bambu-Verbindung",bambuWait:"Warte auf Bambu-Status",scanNone:"Keine 2,4-GHz-Netze gefunden. SSID kann manuell eingegeben werden.",backupLights:"Meine LED-Sicherung speichern",restoreLights:"Meine LED-Sicherung laden",backupSaved:"LED-Sicherung im Gerät gespeichert",backupRestored:"LED-Sicherung wiederhergestellt",comet:'Komet',wipe:'Wandernde Füllung',alternating:'Wechselnde Paare',randomPairs:'Zufällige Paare',randomHint:'Die Paare leuchten unabhängig in kurzen, zufälligen Abständen auf und erlöschen.',ledOn:'An',faqMenu:'FAQ',blueLedTitle:'Blaue LED am LayerGlow',blueLedIntro:'Das Signal wiederholt sich mit einer Pause zwischen den Zyklen:',blueLedNoWifi:'Ein Blinkzeichen: keine WLAN-Verbindung',blueLedWifi:'Zwei Blinkzeichen: mit WLAN verbunden',blueLedUpdate:'Zwei lange Blinkzeichen, Pause, ein kurzes: neue Firmware auf GitHub',faqUpdateHint:'Das Panel prüft GitHub beim Öffnen und alle 30 Minuten, solange es geöffnet ist. Manuell geht es unter Update.',githubMissing:'Keine OTA-.bin-Datei im GitHub-Ordner Software gefunden.',githubNetwork:'GitHub nicht erreichbar. Internetverbindung prüfen.',githubFormat:'Ungültige Update-Liste von GitHub.',updateMenu:'Update',networkSaved:'Netzwerkeinstellungen gespeichert',networkRestart:'Gespeichert. Verbindung zum neuen WLAN läuft. Falls nötig, LayerGlow-Setup verwenden.',invalidMoonraker:'Ungültige Moonraker-URL',missingFields:'Druckerdaten fehlen',invalidNetwork:'Ungültige IP, Port oder SSID',invalidLedCount:'Ungültige LED-Anzahl',invalidLedRow:'Ungültige LED-Einstellungen',noSpace:'Zu wenig OTA-Speicher. Einmal per USB mit min_spiffs-Partitionierung flashen.',led24:'24 LEDs · 12 Paare',led12:'12 LEDs · 6 Paare',moonPort:'Moonraker-Port',setupOffline:'Keine Druckerverbindung (Einrichtungsnetz)',checkUpdates:'Auf GitHub nach Updates suchen',installUpdate:'Update installieren',checkingUpdates:'GitHub wird geprüft…',noUpdate:'Firmware ist aktuell',newVersion:'Neue Version verfügbar:',githubUnavailable:'GitHub konnte nicht geprüft werden. Internet und Software/update.json prüfen.',downloadFirmware:'Firmware von GitHub wird geladen…',invalidFirmware:'Geladene Firmware stimmt nicht mit dem Update-Manifest überein.',githubConfirm:'Version {version} installieren? Dies kann etwa 2 Minuten dauern. Stromversorgung und Seite geöffnet lassen.',centreOut:'Mitte → außen → Mitte',edgesIn:'Außen → Mitte → außen',wave:'Fließende Welle',restarting:'Upload und Neustart…',waiting:'Warte auf LayerGlow…',retry:'Neustart nicht bestätigt. Gerät prüfen und Seite neu laden.',deviceOffline:'Verbindung zum LayerGlow verloren',function:'Funktion',colour:'Farbe',pulsing:'Pulsieren?',effect:'Effekt',allBar:'Ganzer Streifen',lastPair:'Letztes Paar',scanner:'Lauflicht (Knight Rider)',close:'Schließen',preview:'LED-Vorschau und Test',testMode:'Testmodus',testStatus:'Teststatus',testProgress:'Testfortschritt',printing:'Druckt',heating:'Heizt',paused:'Pausiert',complete:'Abgeschlossen',error:'Fehler',standby:'Bereit',run:'▶ Test starten',stop:'■ Stoppen',reset:'Zurücksetzen',loopTest:'Test nach 100 % wiederholen',simHint:'Der Test steuert auch die angeschlossenen LEDs.',networkSettings:'Netzwerk',scanWifi:'Verfügbare 2,4-GHz-WLANs',scanFirst:'Zuerst suchen',scan:'Suchen',wifiName:'WLAN-Name (SSID)',wifiPass:'WLAN-Passwort (leer lässt gespeichert)',printerIP:'IP-Adresse des Druckers',lighting:'Farben und LED-Einstellungen',ledCount:'Physische LEDs',brightness:'Helligkeit',stepTime:'Testschritt (ms)',pulseMode:'Pulsiermodus',pulseOff:'Aus',pulseAll:'Alle leuchtenden LEDs',pulseLast:'Letztes leuchtendes Paar beim Drucken',pulsePeriod:'Zyklus / Tempo (ms)',powerHint:'LED-Stromversorgung vor höherer Helligkeit prüfen.',resetColors:'Originalfarben wiederherstellen',saving:'Speichern…',wifiChangeHint:'WLAN hinzufügen: Namen auswählen oder eingeben, Passwort eingeben und speichern. Mit Verbinden unten ein gespeichertes Netzwerk wählen. Wenn keines funktioniert, LayerGlow-Setup verbinden und 192.168.4.1 öffnen.',pulseStages:'In diesen Zuständen pulsieren (standardmäßig aus)',pulseLastPair:'Beim Drucken nur das letzte leuchtende Paar pulsieren lassen',autoProgress:'Fortschritt automatisch abspielen',resetLights:'LED-Standardeinstellungen wiederherstellen',saveLights:'LED-Einstellungen speichern',updateTitle:'Firmware-Update · OTA',updateHint:'.bin-Datei für LayerGlow auswählen. Das Update dauert etwa 2 Minuten. Strom nicht trennen und diese Seite nicht schließen.',upload:'Hochladen und neu starten',security:'Panel und OTA haben kein Login. Nur in einem vertrauenswürdigen lokalen Netz verwenden; nicht im Internet freigeben.',credits:'Entwickelt von',progressColor:'Druckfortschritt',colour_printing:'Druckt',colour_heating:'Heizt',colour_paused:'Pausiert',colour_complete:'Fertig',colour_error:'Fehler',colour_standby:'Bereit',saved:'Einstellungen gespeichert',scanError:'Suche fehlgeschlagen',updateDone:'Firmware hochgeladen; Neustart…',updateFailed:'Upload fehlgeschlagen',offline:'Drucker offline',connected:'Drucker verbunden',setup:'Einrichtungsnetz',home:'Lokales WLAN',test:'TEST',uploadConfirm:'Das Update kann etwa 2 Minuten dauern. Strom nicht trennen und diese Seite nicht schließen. Firmware hochladen?',chooseBin:'.bin-Datei auswählen',scanning:'Suche läuft…'}};
let lang='en',initialized=false,lastStatus={},simBusy=false,simQueued=null,lastNetworkInfo=null,networkInfoBusy=false;
function setLang(l){if(!T[l])l='en';lang=l;document.documentElement.lang=l;document.title=T[l].appName+' by FatGiantCat';document.getElementById('openLang').textContent=({en:'🇬🇧',pl:'🇵🇱',es:'🇪🇸',de:'🇩🇪'})[l];localStorage.setItem('k2lang',l);document.querySelectorAll('[data-i]').forEach(e=>{e.textContent=T[l][e.dataset.i]||T.en[e.dataset.i]||e.textContent});document.querySelectorAll('[data-aria-i]').forEach(e=>e.setAttribute('aria-label',T[l][e.dataset.ariaI]||T.en[e.dataset.ariaI]));document.querySelectorAll('.toast button').forEach(e=>e.setAttribute('aria-label',T[l].close));for(const id of Object.keys(T).filter(x=>['en','pl','es','de'].includes(x)))document.getElementById(id).classList.toggle('active',l===id);paintStatus(lastStatus);if(lastNetworkInfo)renderConnectionStatus(lastNetworkInfo);if(availableUpdate)document.getElementById('githubMessage').textContent=T[l].newVersion+' '+availableUpdate.version;rowIds.forEach(syncRowControls);if(initialized){lastConnectionKey=null;refresh()}}
const GITHUB_MANIFEST='https://raw.githubusercontent.com/TomaszK00/LayerGlow/main/Software/update.json';
const GITHUB_LIST='https://api.github.com/repos/TomaszK00/LayerGlow/contents/Software?ref=main';
let availableUpdate=null,refreshing=false,updatingFirmware=false;
const rowIds=['progress','heating','paused','complete','error','standby'];
const rowLabels=['printing','heating','paused','complete','error','standby'];
const rowContainer=document.getElementById('ledRows');
for(const [i,id] of rowIds.entries()){const row=document.createElement('div');row.className='led-row';const effects='<option value="0" data-i="allBar">Whole bar</option><option value="1" data-i="lastPair">Last pair</option><option value="2" data-i="scanner">Scanner (Knight Rider)</option><option value="3" data-i="centreOut">Centre → edges → centre</option><option value="5" data-i="wave">Flowing wave</option><option value="6" data-i="randomPairs">Random pairs</option><option value="7" data-i="comet">Comet</option><option value="8" data-i="wipe">Moving fill</option><option value="9" data-i="alternating">Alternating pairs</option><option value="10" data-i="colourFlow">Colour flow</option><option value="11" data-i="colourFade">Colour fade</option><option value="12" data-i="rainbowFlow">Rainbow flow</option><option value="13" data-i="colourMarch">Moving colour bands</option>'; row.innerHTML=`<div class="row-title"><label data-i="${rowLabels[i]}" for="c_${id}"></label><label class="enabled-toggle"><input type="checkbox" name="on_${id}" id="on_${id}" checked><span data-i="ledOn">On</span></label></div><div class="palette-controls"><input type="hidden" name="n_${id}" id="n_${id}" value="1"><div class="palette-toolbar"><div class="palette-swatches"><input type="color" name="c_${id}" id="c_${id}" data-aria-i="colour">${Array.from({length:11},(_,idx)=>idx+1).map(n=>`<input type="color" name="c${n}_${id}" id="c${n}_${id}" data-aria-i="colour" hidden>`).join('')}</div><div class="palette-actions"><button type="button" class="alt" id="add_${id}" data-aria-i="addColour" aria-label="Add colour" onclick="changePalette('${id}',1)">+</button><button type="button" class="alt" id="remove_${id}" data-aria-i="removeColour" aria-label="Remove colour" onclick="changePalette('${id}',-1)">−</button></div></div></div><div class="brightness-control"><details><summary><span data-i="brightness">Brightness</span><strong id="bv_${id}">65%</strong></summary><input type="range" name="b_${id}" id="b_${id}" min="0" max="100" step="1" value="65" data-aria-i="brightness"></details></div><div class="pulse-control"><details><summary><span data-i="pulseSetting">Pulse</span><strong id="pv_${id}">Off</strong></summary><label class="pulse-enabled"><input type="checkbox" name="pe_${id}" id="pe_${id}"><span data-i="pulseEnabled">Enabled</span></label><div class="range-line"><input type="range" name="pp_${id}" id="pp_${id}" min="0" max="5000" step="50" value="1400" data-aria-i="pulsePeriod"><output id="pms_${id}" for="pp_${id}">1400 ms</output></div></details></div><div class="effect-control"><span class="field-label" data-i="effect">Effect</span><span class="effect-name" id="effectCurrent_${id}"></span><select name="ps_${id}" id="ps_${id}" data-aria-i="effect">${effects}</select></div>`;rowContainer.append(row)}
function syncRowControls(id){
  const bright=document.getElementById('b_'+id),period=document.getElementById('pp_'+id),pulse=document.getElementById('pe_'+id),effect=document.getElementById('ps_'+id);
  document.getElementById('bv_'+id).textContent=bright.value+'%';
  document.getElementById('pv_'+id).textContent=pulse.checked&&Number(period.value)>0?period.value+' ms':T[lang].pulseOff;
  document.getElementById('pms_'+id).textContent=period.value+' ms';
  document.getElementById('effectCurrent_'+id).textContent=effect.selectedOptions[0]?.textContent||'';
}
function updatePaletteControls(id){const count=Number(document.getElementById('n_'+id).value);for(let n=1;n<12;n++)document.getElementById('c'+n+'_'+id).hidden=n>=count;document.getElementById('add_'+id).disabled=count>=12;document.getElementById('remove_'+id).disabled=count<=1}
function changePalette(id,delta){const field=document.getElementById('n_'+id);field.value=String(Math.max(1,Math.min(12,Number(field.value)+delta)));updatePaletteControls(id);paintStatus(lastStatus);scheduleLightSave()}
function paletteFor(id,r){const count=Number(document.getElementById('n_'+id)?.value||r.colourCount||1);return Array.from({length:count},(_,n)=>document.getElementById((n?'c'+n:'c')+'_'+id)?.value||r.colours?.[n]||r.color);}
function paletteSlot(pixel,total,count){return Math.min(count-1,Math.floor(pixel*count/total));}
function blendColour(a,b,f){const x=parseInt(a.slice(1),16),y=parseInt(b.slice(1),16);return '#'+[16,8,0].map(shift=>Math.floor((((x>>shift)&255)*(65536-f)+((y>>shift)&255)*f+32768)/65536).toString(16).padStart(2,'0')).join('')}
function samplePalette(palette,position,smooth=true){const scaled=position*palette.length,slot=Math.floor(scaled/65536),fraction=scaled%65536;return smooth?blendColour(palette[slot],palette[(slot+1)%palette.length],fraction):palette[slot]}
function effectColour(palette,pixel,total,style,tick){const position=(Math.floor(pixel*65536/total)+65536-tick)%65536;if(style===10)return samplePalette(palette,position);if(style===11)return samplePalette(palette,tick);if(style===12)return samplePalette(['#ff0000','#ffff00','#00ff00','#00ffff','#0000ff','#ff00ff'],position);if(style===13)return samplePalette(palette,position,false);return palette[paletteSlot(pixel,total,palette.length)]}
function applyRows(rows){for(const id of rowIds){const r=rows[id];document.getElementById('c_'+id).value=r.color;document.getElementById('n_'+id).value=r.colourCount||1;for(let n=1;n<12;n++)document.getElementById('c'+n+'_'+id).value=r.colours?.[n]||['#ff9e35','#ffce45','#2fe182','#9b5cff','#ff445c','#2878ff','#ffffff','#ff3366','#3edbd1','#f0df56','#704dff'][n-1];updatePaletteControls(id);document.getElementById('on_'+id).checked=r.enabled!==false;document.getElementById('b_'+id).value=r.brightness;document.getElementById('pe_'+id).checked=r.pulse;document.getElementById('pp_'+id).value=r.period;document.getElementById('ps_'+id).value=r.style;syncRowControls(id)}}
function updateUpdateButton(s=lastStatus){document.getElementById('openUpdate').classList.toggle('update-ready',Boolean(availableUpdate||s?.updateAvailable))}
function toast(message,error=false,sticky=false,key=''){if(!message)return;if(key)document.querySelector(`[data-toast-key="${key}"]`)?.remove();const box=document.createElement('div');box.className='toast'+(error?' error':'');if(key)box.dataset.toastKey=key;if(sticky)box.dataset.sticky='true';const close=document.createElement('button');close.type='button';close.textContent='×';close.setAttribute('aria-label',T[lang].close);close.onclick=()=>box.remove();box.append(close,document.createTextNode(message));document.getElementById('toasts').prepend(box);const removable=[...document.querySelectorAll('.toast:not([data-sticky="true"])')];while(document.querySelectorAll('.toast').length>4&&removable.length)removable.pop().remove();if(!sticky)setTimeout(()=>box.remove(),6500)}

document.getElementById('openNetwork').onclick=()=>{document.getElementById('networkDialog').showModal();refreshNetworkInfo()};
document.getElementById('openPrinter').onclick=()=>{document.getElementById('printerDialog').showModal();refreshNetworkInfo()};
document.getElementById('closePrinter').onclick=()=>document.getElementById('printerDialog').close();
document.getElementById('closeNetwork').onclick=()=>document.getElementById('networkDialog').close();
document.getElementById('openUpdate').onclick=()=>document.getElementById('updateDialog').showModal();
document.getElementById('closeUpdate').onclick=()=>document.getElementById('updateDialog').close();
document.getElementById('openFAQ').onclick=()=>document.getElementById('faqDialog').showModal();
document.getElementById('closeFAQ').onclick=()=>document.getElementById('faqDialog').close();
document.getElementById('openLang').onclick=()=>document.getElementById('languageDialog').showModal();
document.getElementById('closeLang').onclick=()=>document.getElementById('languageDialog').close();
for(const dialog of document.querySelectorAll('dialog')){
  dialog.addEventListener('click',e=>{
    if(window.matchMedia('(max-width:850px)').matches&&dialog.id!=='languageDialog')return;
    const r=dialog.getBoundingClientRect();
    if(e.target===dialog&&(e.clientX<r.left||e.clientX>r.right||e.clientY<r.top||e.clientY>r.bottom))dialog.close();
  });
  dialog.addEventListener('cancel',e=>{if(window.matchMedia('(max-width:850px)').matches&&dialog.id!=='languageDialog')e.preventDefault()});
}
const appHeader=document.querySelector('.app-header'),previewCard=document.getElementById('previewCard');
function updateStickyLayout(){document.documentElement.style.setProperty('--header-stick-height',Math.ceil(appHeader.getBoundingClientRect().height+12)+'px')}
new ResizeObserver(updateStickyLayout).observe(appHeader);updateStickyLayout();
let previousScroll=window.scrollY;window.addEventListener('scroll',()=>{const current=window.scrollY;if(window.innerWidth<=850){if(current>100&&current>previousScroll+4)appHeader.classList.add('scroll-hidden');else if(current<previousScroll-4||current<50)appHeader.classList.remove('scroll-hidden');previewCard.classList.toggle('preview-compact',current>previewCard.offsetTop+70)}else{appHeader.classList.remove('scroll-hidden');previewCard.classList.remove('preview-compact')}previousScroll=current},{passive:true});

function buildBar(total){const bar=document.getElementById('bar');if(Number(bar.dataset.leds)===total)return;bar.replaceChildren();bar.dataset.leds=String(total);bar.style.gridTemplateColumns='repeat('+Math.min(12,Math.ceil(total/2))+',minmax(0,1fr))';for(let i=0;i<total;i+=2){const pair=document.createElement('div');pair.className='pair';for(let j=i;j<Math.min(i+2,total);j++){const led=document.createElement('div');led.className='led';led.dataset.pixel=String(j);pair.append(led)}bar.append(pair)}}

function renderConnectionStatus(info){const t=T[lang],put=(id,value,mode='')=>{const el=document.getElementById(id);el.textContent=value===undefined||value===null||value===''?'—':String(value);el.classList.toggle('link-ok',mode==='ok');el.classList.toggle('link-off',mode==='off')};const wifi=info.wifi||{},ap=info.ap||{},printer=info.printer||{};
put('linkWifi',wifi.connected?t.connectionOn:t.connectionOff,wifi.connected?'ok':'off');put('linkSSID',wifi.ssid);put('linkLocalIP',wifi.ip);const friendly=document.getElementById('linkFriendly');friendly.textContent='http://'+(info.webHost||'layerglow')+'.local/';friendly.href=friendly.textContent;document.getElementById('linkFriendlyState').textContent=info.mdnsReady?t.nameReady:t.namePending;put('linkGateway',wifi.gateway);put('linkSubnet',wifi.subnet);put('linkDNS',wifi.dns);put('linkRSSI',wifi.connected?wifi.rssi+' dBm':'—');put('linkChannel',wifi.connected?wifi.channel:'—');put('linkMAC',wifi.mac);
put('linkAP',ap.active?t.connectionActive:t.connectionInactive,ap.active?'ok':'off');put('linkAPSSID',ap.active?ap.ssid:'—');put('linkAPIP',ap.ip);put('linkClients',ap.active?ap.clients:'—');put('linkBrowser',window.location.host);
put('linkPrinter',printer.connected?t.connectionOn:t.connectionOff,printer.connected?'ok':'off');put('linkPrinterType',({moonraker:t.crealityK2,bambu:t.bambuA1,octoprint:t.octoType,prusalink:t.prusaType})[printer.type]||printer.type);put('linkPrinterAddress',printer.ip?printer.ip+':'+printer.port:'—');put('linkPrinterError',!printer.enabled?t.printerDisabled:printer.error?localError(printer.error):t.connectionNoError);put('printerReadState',printer.enabled?(printer.connected?(t[printer.state]||printer.state):t.connectionWaiting):t.printerDisabled);put('printerReadProgress',printer.connected?Math.round((printer.progress||0)*100)+'%':'—');put('printerReadFile',printer.connected?printer.filename:'—');put('printerReadAge',printer.connected&&printer.lastReadAgeSeconds>=0?printer.lastReadAgeSeconds+' '+t.secondsAgo:'—');document.getElementById('disconnectPrinter').disabled=!printer.enabled;renderSavedNetworks(info)}
function renderSavedNetworks(info){const box=document.getElementById('savedWifiList'),list=info.savedNetworks||[],t=T[lang];box.replaceChildren();if(!list.length){const p=document.createElement('p');p.className='muted';p.textContent=t.savedWifiNone;box.append(p);return}for(const network of list){const row=document.createElement('div');row.className='saved-network';const label=document.createElement('div'),name=document.createElement('strong'),state=document.createElement('small'),controls=document.createElement('div');name.textContent=network.ssid;state.textContent=network.connected?t.savedWifiConnected:network.active?t.savedWifiTrying:'';label.append(name,state);controls.className='controls';const connect=document.createElement('button');connect.type='button';connect.className='alt';connect.textContent=t.savedWifiConnect;connect.disabled=network.connected;connect.onclick=()=>savedWifiAction('select',network.index);const remove=document.createElement('button');remove.type='button';remove.className='ghost';remove.textContent=t.savedWifiRemove;remove.onclick=()=>{if(confirm(t.savedWifiConfirm+' '+network.ssid))savedWifiAction('delete',network.index)};controls.append(connect,remove);row.append(label,controls);box.append(row)}}
async function savedWifiAction(action,index){const message=document.getElementById('savedWifiMessage');try{const response=await fetch('/api/wifi/'+action,{method:'POST',body:new URLSearchParams({index:String(index)})});if(!response.ok)throw Error(await response.text());message.textContent=action==='select'?T[lang].networkRestart:T[lang].savedWifiRemoved;await refreshNetworkInfo()}catch(e){message.textContent=localError(e.message);toast(message.textContent,true)}}
async function refreshNetworkInfo(){if(networkInfoBusy)return;networkInfoBusy=true;try{const info=await api('/api/network/info',{cache:'no-store',signal:AbortSignal.timeout(3500)});lastNetworkInfo=info;renderConnectionStatus(info);document.getElementById('connectionInfoMessage').textContent=''}catch(_){document.getElementById('connectionInfoMessage').textContent=T[lang].connectionUnavailable}finally{networkInfoBusy=false}}
setInterval(()=>{if(document.getElementById('networkDialog').open||document.getElementById('printerDialog').open)refreshNetworkInfo()},3000);
async function api(path,options){const r=await fetch(path,options);if(!r.ok)throw Error(await r.text());return r.json()}
function paintStatus(s){if(!s||!s.rows)return;const state=s.sim?s.simState:(s.connected?s.state:'standby'),id=state==='printing'?'progress':state,prog=s.sim?s.simProgress:s.progress*100,pairs=Math.ceil(s.ledCount/2),r=s.rows[id];if(!r)return;let count=state==='printing'?Math.ceil(prog*pairs/100):(state==='standby'&&!r.pulse&&Number(document.getElementById('ps_'+id)?.value??r.style)<6?0:pairs);const enabled=document.getElementById('on_'+id)?.checked??(r.enabled!==false);if(!enabled)count=0;buildBar(s.ledCount);const palette=paletteFor(id,r),bright=Number(document.getElementById('b_'+id)?.value??r.brightness)/100,pulse=document.getElementById('pe_'+id)?.checked??r.pulse,period=Number(document.getElementById('pp_'+id)?.value??r.period),style=Number(document.getElementById('ps_'+id)?.value??r.style);document.getElementById('percent').textContent=Math.round(prog)+'%';document.getElementById('ledcount').textContent=Math.min(count*2,s.ledCount)+' / '+s.ledCount+' LED';document.getElementById('displaystate').textContent=(s.sim?T[lang].test+' · ':'')+(T[lang][state]||state);document.getElementById('mobilePct').textContent=Math.round(prog)+'%';document.getElementById('mobileState').textContent=(s.sim?T[lang].test+' · ':'')+(T[lang][state]||state);document.getElementById('mobileLedCount').textContent=Math.min(count*2,s.ledCount)+'/'+s.ledCount+' LED';document.getElementById('file').textContent=s.filename||'';document.getElementById('file').style.display=s.filename?'':'none';const randomNow=Date.now(),randomKey=id+'/'+count+'/'+style;if(randomPreview.key!==randomKey){randomPreview.key=randomKey;randomPreview.on=[];randomPreview.next=[]}const randomBase=Math.max(55,Math.min(220,Math.round(period/12)));const cycle=Math.max(200,period||1400),phase=(randomNow%cycle)/cycle,wave=(1-Math.cos(2*Math.PI*phase))/2,scale=.25+.75*wave,travel=phase*2*Math.max(0,count-1),head=travel<=count-1?travel:2*(count-1)-travel,centre=(count-1)/2,radius=Math.max(.5,centre),outward=1-Math.abs(2*phase-1),front=outward*radius;[...document.querySelectorAll('.pair')].forEach((p,i)=>{if(style===6&&i<count){if(randomPreview.next[i]===undefined)randomPreview.next[i]=randomNow+Math.random()*randomBase;if(randomNow>=randomPreview.next[i]){randomPreview.on[i]=!randomPreview.on[i];randomPreview.next[i]=randomNow+randomBase*(.5+Math.random()*1.5)}}[...p.children].forEach(led=>{const color=effectColour(palette,Number(led.dataset.pixel),s.ledCount,style,Math.floor((randomNow%cycle)*65536/cycle));let lit=i<count&&(style!==6||!!randomPreview.on[i]);let level=1;if(pulse&&lit&&period>0){if(style===2)level=Math.max(.08,1-Math.abs(i-head)/2.2);else if(style===3)level=Math.max(.08,1-Math.abs(Math.abs(i-centre)-front)/1.3);else if(style===5)level=.16+.84*(.5+.5*Math.cos(2*Math.PI*(i/Math.max(1,count-1)-phase)));else if(style===0||style>=10||(style===1&&i===count-1))level=scale}if(lit&&style===7){const trail=((phase*count-i)%count+count)%count;level=Math.max(0,1-trail/3.2);lit=level>.02}else if(lit&&style===8){const step=Math.floor(phase*2*count);lit=step<count?i<=step:i>step-count}else if(lit&&style===9){lit=i%2===Math.floor(phase*2)}led.style.background=lit?color:'#e3ebe5';led.style.opacity=lit?Math.max(.05,bright*level):1;led.style.boxShadow=lit?'0 0 7px '+color+'40':'none'})})}

let discoveredPrinters=[],discoveryBusy=false;
function requiredPrinterCredentials(){const type=document.getElementById('printerType').value,ip=document.getElementById('ip').value.trim();document.getElementById('bambuCode').required=type==='bambu'&&(!lastStatus?.hasBambuCode||ip!==savedPrinterIPs.bambu||document.getElementById('bambuSerial').value.trim().toUpperCase()!==(lastStatus?.bambuSerial||''));document.getElementById('apiKey').required=(type==='octoprint'&&(!lastStatus?.hasOctoKey||ip!==savedPrinterIPs.octoprint))||(type==='prusalink'&&(!lastStatus?.hasPrusaKey||ip!==savedPrinterIPs.prusalink))}
async function discoverPrinters(){if(discoveryBusy)return;discoveryBusy=true;const button=document.getElementById('findPrinters'),list=document.getElementById('foundPrinters'),msg=document.getElementById('discoveryMessage'),bar=document.getElementById('discoveryBar'),pct=document.getElementById('discoveryPercent');document.getElementById('discoveryProgress').hidden=false;bar.value=0;pct.textContent='0%';button.disabled=true;msg.textContent=T[lang].discoverySearching;try{let result=await api('/api/printers/discovery',{method:'POST',signal:AbortSignal.timeout(4000)});for(let i=0;result.searching&&i<180;i++){const done=result.phase===1&&result.total?Math.min(99,Math.floor(100*result.checked/result.total)):0;bar.value=done;pct.textContent=done+'%';msg.textContent=T[lang].discoverySearching;await new Promise(resolve=>setTimeout(resolve,800));result=await api('/api/printers/discovery',{signal:AbortSignal.timeout(4000)})}if(result.searching)throw Error('timeout');bar.value=100;pct.textContent='100%';discoveredPrinters=result.printers||[];list.replaceChildren(new Option(discoveredPrinters.length?T[lang].discoverySelect:T[lang].discoveryNone,''));discoveredPrinters.forEach((p,i)=>list.add(new Option((p.name||p.type)+' · '+p.ip+(p.port&&p.type!=='bambu'?':'+p.port:'')+' ('+p.type+')',String(i))));msg.textContent=discoveredPrinters.length?T[lang].discoveryReady+discoveredPrinters.length:T[lang].discoveryNone}catch(err){msg.textContent=err.message.includes('Wi-Fi')?T[lang].discoveryUnavailable:T[lang].discoveryError}finally{button.disabled=false;discoveryBusy=false;button.textContent=T[lang].rescanPrinters}}
document.getElementById('findPrinters').onclick=discoverPrinters;
document.getElementById('foundPrinters').onchange=e=>{const p=discoveredPrinters[Number(e.target.value)];if(!e.target.value||!p)return;document.getElementById('printerType').value=p.type;printerTypeChanged();document.getElementById('ip').value=p.ip;if(p.type!=='bambu')document.getElementById('port').value=p.port;if(p.type==='bambu')document.getElementById('bambuSerial').value=p.serial||'';requiredPrinterCredentials();document.getElementById('discoveryMessage').textContent=p.type==='bambu'||p.type==='octoprint'||p.type==='prusalink'?T[lang].discoveryCredentials:T[lang].discoveryHint+' '+T[lang].selectThenSave};
document.getElementById('ip').addEventListener('input',requiredPrinterCredentials);document.getElementById('bambuSerial').addEventListener('input',requiredPrinterCredentials);
const printerIPs={moonraker:'',bambu:'',octoprint:'',prusalink:''},savedPrinterIPs={moonraker:'',bambu:'',octoprint:'',prusalink:''},printerPorts={moonraker:7125,octoprint:80,prusalink:80};let printerTypeShown='moonraker';
function printerTypeChanged(initial=false){const type=document.getElementById('printerType').value;if(!initial){printerIPs[printerTypeShown]=document.getElementById('ip').value;if(printerTypeShown!=='bambu')printerPorts[printerTypeShown]=Number(document.getElementById('port').value)||80}printerTypeShown=type;document.getElementById('ip').value=printerIPs[type]||'';document.getElementById('portFields').style.display=type==='bambu'?'none':'';document.getElementById('port').required=type!=='bambu';document.getElementById('port').value=printerPorts[type]||80;document.getElementById('bambuFields').style.display=type==='bambu'?'':'none';document.getElementById('apiKeyFields').style.display=(type==='octoprint'||type==='prusalink')?'':'none';document.getElementById('bambuSerial').required=type==='bambu';document.getElementById('bambuCode').required=type==='bambu'&&!(lastStatus&&lastStatus.hasBambuCode);document.getElementById('apiKey').required=(type==='octoprint'&&!lastStatus?.hasOctoKey)||(type==='prusalink'&&!lastStatus?.hasPrusaKey);document.getElementById('apiKey').value='';requiredPrinterCredentials()}
const randomPreview={key:"",on:[],next:[]};
let lastConnectionKey=null;
async function refresh(){if(refreshing||updatingFirmware)return;refreshing=true;try{const s=await api('/api/status',{cache:'no-store',signal:AbortSignal.timeout(3500)});lastStatus=s;updateUpdateButton(s);if(!initialized){for(const [id,v] of Object.entries({ssid:s.ssid,ip:s.ip,port:s.port,ledCount:s.ledCount}))document.getElementById(id).value=v;applyRows(s.rows);document.getElementById('webHost').value=s.webHost||'layerglow';printerIPs.moonraker=s.moonIP||'';printerIPs.bambu=s.bambuIP||'';printerIPs.octoprint=s.octoIP||'';printerIPs.prusalink=s.prusaIP||'';Object.assign(savedPrinterIPs,printerIPs);printerPorts.moonraker=s.moonPort||7125;printerPorts.octoprint=s.octoPort||80;printerPorts.prusalink=s.prusaPort||80;printerTypeShown=s.printerType||'moonraker';document.getElementById('printerType').value=printerTypeShown;document.getElementById('bambuSerial').value=s.bambuSerial||'';printerTypeChanged(true);document.getElementById('testStatus').value=s.simState;document.getElementById('simLoop').checked=s.simLoop;initialized=true}requiredPrinterCredentials();document.getElementById('manualProgress').value=s.simProgress;document.getElementById('manualPct').textContent=s.simProgress+'%';document.getElementById('sim').checked=s.sim;document.getElementById('autoProgress').checked=s.simRunning;paintStatus(s);document.getElementById('restoreLights').disabled=!s.lightingBackup;const key=[s.ap,s.connected,s.connected?'':s.error].join('|');if(key!==lastConnectionKey){lastConnectionKey=key;toast(s.connected?T[lang].connected:(s.ap?T[lang].setupOffline:T[lang].offline+(s.error&&s.error!=='No printer connection (setup AP)'?' · '+localError(s.error):'')),!s.connected,!s.connected,'connection')}}catch(e){if(lastConnectionKey!=='device-offline'){lastConnectionKey='device-offline';toast(T[lang].deviceOffline,true,true,'connection')}}finally{refreshing=false}}

function localError(message){if(message==='Invalid device name')return T[lang].nameInvalid;if(message==='Saved Wi-Fi list is full')return T[lang].savedWifiFull;if(message==='Invalid saved network')return T[lang].savedWifiInvalid;if(message==='No printer connection (setup AP)')return T[lang].setupOffline;if(message==='Printer disconnected')return T[lang].printerDisabled;if(message==='Waiting for first poll')return T[lang].connectionWaiting;if(message==='Invalid Bambu serial or access code')return T[lang].invalidBambu;if(message==='Bambu access denied')return T[lang].bambuDenied;if(message==='Bambu MQTT offline'||message==='Bambu settings missing')return T[lang].bambuOffline;if(message==='Bambu status timeout')return T[lang].bambuWait;if(message==='Invalid Moonraker URL')return T[lang].invalidMoonraker;if(message==='Missing printer fields')return T[lang].missingFields;if(message==='Invalid IP, port or SSID')return T[lang].invalidNetwork;if(message==='Invalid LED count')return T[lang].invalidLedCount;if(message.startsWith('Invalid LED row:'))return T[lang].invalidLedRow;if(message.includes('Not Enough Space'))return T[lang].noSpace;if(message.startsWith('JSON:'))return 'JSON: '+message.slice(5);return message}
async function postForm(path,form){const r=await fetch(path,{method:'POST',body:new URLSearchParams(new FormData(form))});if(!r.ok)throw Error(await r.text());return r.text()}
document.getElementById('wifiForm').onsubmit=async e=>{e.preventDefault();try{const host=document.getElementById('webHost').value.trim().toLowerCase().replace(/\.local\.?$/,'');document.getElementById('webHost').value=host;const msg=await postForm('/api/wifi',e.target),label=msg.startsWith('Switching')?T[lang].networkRestart:T[lang].networkSaved;const output=document.getElementById('wifiMessage');output.replaceChildren(document.createTextNode(label+' · '));const link=document.createElement('a');link.href='http://'+host+'.local/';link.textContent=T[lang].openNewAddress+' ('+host+'.local)';output.append(link);document.getElementById('pass').value='';refreshNetworkInfo();toast(label)}catch(err){document.getElementById('wifiMessage').textContent=localError(err.message);toast(localError(err.message),true)}};
document.getElementById('disconnectPrinter').onclick=async()=>{try{await fetch('/api/printer/disconnect',{method:'POST'}).then(r=>{if(!r.ok)throw Error('HTTP '+r.status)});await refresh();await refreshNetworkInfo();document.getElementById('printerMessage').textContent=T[lang].printerDisabled;toast(T[lang].printerDisabled)}catch(e){toast(localError(e.message),true)}};
document.getElementById('printerForm').onsubmit=async e=>{e.preventDefault();try{await postForm('/api/printer',e.target);const type=document.getElementById('printerType').value;printerIPs[type]=document.getElementById('ip').value;savedPrinterIPs[type]=printerIPs[type];if(type!=='bambu')printerPorts[type]=Number(document.getElementById('port').value);document.getElementById('bambuCode').value='';document.getElementById('apiKey').value='';printerTypeChanged(true);await refresh();refreshNetworkInfo();document.getElementById('printerMessage').textContent=T[lang].printerSaved;toast(T[lang].printerSaved)}catch(err){document.getElementById('printerMessage').textContent=localError(err.message);toast(localError(err.message),true)}};
let lightTimer,lightSaveChain=Promise.resolve(),resettingColors=false;
const lightForm=document.getElementById('lightForm'),lightMessage=document.getElementById('lightMessage');
function scheduleLightSave(){if(resettingColors)return;clearTimeout(lightTimer);lightTimer=setTimeout(saveLightSettings,450)}
function saveLightSettings(){if(resettingColors||!lightForm.checkValidity())return;clearTimeout(lightTimer);lightMessage.textContent=T[lang].saving;lightSaveChain=lightSaveChain.catch(()=>{}).then(async()=>{if(resettingColors)return;try{await postForm('/api/lighting',lightForm);lightMessage.textContent=T[lang].saved;toast(T[lang].saved);await refresh()}catch(err){lightMessage.textContent=localError(err.message);toast(localError(err.message),true)}});return lightSaveChain}
lightForm.addEventListener('input',e=>{
  const id=e.target.id.split('_').slice(1).join('_');
  if(rowIds.includes(id)&&['pp','b','pe','ps'].includes(e.target.id.split('_')[0])){
    if(e.target.id.startsWith('pp_'))document.getElementById('pe_'+id).checked=Number(e.target.value)>0;
    if(e.target.id.startsWith('pe_')&&e.target.checked&&Number(document.getElementById('pp_'+id).value)===0)document.getElementById('pp_'+id).value=1400;
    syncRowControls(id);
  }
  if(e.target.type!=='color')scheduleLightSave();
});
lightForm.addEventListener('change',e=>{
  const id=e.target.id.split('_').slice(1).join('_');
  if(rowIds.includes(id))syncRowControls(id);
  scheduleLightSave();
  if(window.matchMedia('(max-width:850px)').matches&&e.target.matches('.brightness-control input[type=range], .pulse-control input[type=range]')){
    const details=e.target.closest('details');
    clearTimeout(details.closeTimer);
    details.closeTimer=setTimeout(()=>{details.open=false},800);
  }
});
lightForm.onsubmit=e=>{e.preventDefault();saveLightSettings()};
document.getElementById('resetColors').onclick=async()=>{clearTimeout(lightTimer);resettingColors=true;const button=document.getElementById('resetColors');button.disabled=true;try{await lightSaveChain.catch(()=>{});const settings=await api('/api/lighting/defaults',{method:'POST'});document.getElementById('ledCount').value=settings.ledCount;applyRows(settings.rows);lastStatus={...lastStatus,...settings};paintStatus(lastStatus);lightMessage.textContent=T[lang].saved;toast(T[lang].saved)}catch(err){lightMessage.textContent=localError(err.message);toast(localError(err.message),true)}finally{button.disabled=false;resettingColors=false}};

document.getElementById('backupLights').onclick=async()=>{const button=document.getElementById('backupLights');button.disabled=true;clearTimeout(lightTimer);try{if(!lightForm.checkValidity()){lightForm.reportValidity();return}await lightSaveChain.catch(()=>{});await postForm('/api/lighting',lightForm);const result=await api('/api/lighting/backup',{method:'POST'});document.getElementById('restoreLights').disabled=!result.lightingBackup;lightMessage.textContent=T[lang].backupSaved;toast(T[lang].backupSaved)}catch(err){lightMessage.textContent=localError(err.message);toast(localError(err.message),true)}finally{button.disabled=false}};
document.getElementById('restoreLights').onclick=async()=>{clearTimeout(lightTimer);resettingColors=true;const button=document.getElementById('restoreLights');button.disabled=true;try{await lightSaveChain.catch(()=>{});const settings=await api('/api/lighting/restore',{method:'POST'});document.getElementById('ledCount').value=settings.ledCount;applyRows(settings.rows);lastStatus={...lastStatus,...settings};paintStatus(lastStatus);lightMessage.textContent=T[lang].backupRestored;toast(T[lang].backupRestored)}catch(err){lightMessage.textContent=localError(err.message);toast(localError(err.message),true)}finally{resettingColors=false;button.disabled=!(lastStatus&&lastStatus.lightingBackup)}};

async function scanWifi(){const list=document.getElementById('wifiList'),button=document.getElementById('scanWifiButton');button.disabled=true;list.replaceChildren(new Option(T[lang].scanning,''));try{let result;for(let i=0;i<24;i++){result=await api('/api/wifi/scan');if(!result.scanning)break;await new Promise(resolve=>setTimeout(resolve,650))}if(result.scanning)throw Error('scan timeout');list.replaceChildren(new Option(result.networks.length?'—':T[lang].scanNone,''));result.networks.forEach(n=>list.add(new Option(n.ssid+' ('+n.rssi+' dBm) '+(n.secure?'🔒':''),n.ssid)))}catch(e){list.replaceChildren(new Option(T[lang].scanError,''));toast(T[lang].scanError,true)}finally{button.disabled=false}}
async function sendSimulation(extra={}){simQueued={...(simQueued||{}),...extra};if(simBusy)return;simBusy=true;try{while(simQueued){const action=simQueued;simQueued=null;const p=new URLSearchParams({enabled:document.getElementById('sim').checked?'1':'0',status:document.getElementById('testStatus').value,progress:document.getElementById('manualProgress').value,loop:document.getElementById('simLoop').checked?'1':'0',...action});await api('/api/simulation',{method:'POST',body:p});await refresh()}}catch(e){document.getElementById('simMessage').textContent=e.message}finally{simBusy=false}}
function simToggle(){if(!document.getElementById('sim').checked)document.getElementById('autoProgress').checked=false;sendSimulation({run:'0'})}function testStatusChanged(){if(document.getElementById('testStatus').value!=='printing'){document.getElementById('autoProgress').checked=false;sendSimulation({run:'0'})}else sendSimulation()}function toggleAutoProgress(){const play=document.getElementById('autoProgress').checked;if(play){document.getElementById('sim').checked=true;document.getElementById('testStatus').value='printing';document.getElementById('manualProgress').value=0}sendSimulation({run:play?'1':'0',...(play?{progress:'0'}:{})})}
const sleep=ms=>new Promise(resolve=>setTimeout(resolve,ms));
async function waitForRestart(confirmed,startVersion){let consecutive=0;const began=Date.now();await sleep(1200);while(Date.now()-began<180000){try{const r=await fetch('/api/status?probe='+Date.now(),{cache:'no-store',signal:AbortSignal.timeout(5000)});if(!r.ok)throw Error('HTTP '+r.status);const status=await r.json();if(status.version!==startVersion||(confirmed&&Date.now()-began>6000)){if(++consecutive>=2){location.replace('/?updated='+Date.now());return}}else consecutive=0}catch(_){consecutive=0}await sleep(1800)}throw Error(T[lang].retry)}
// SHA-256 works on the local HTTP panel too, where SubtleCrypto may be unavailable.
function sha256(bytes){
  const primes=[];for(let n=2;primes.length<64;n++){let prime=true;for(let d=2;d*d<=n;d++)if(n%d===0){prime=false;break}if(prime)primes.push(n)}
  const frac=x=>Math.floor((x-Math.floor(x))*4294967296)>>>0;
  const k=primes.map(x=>frac(Math.cbrt(x))),h=primes.slice(0,8).map(x=>frac(Math.sqrt(x)));
  const size=bytes.length,pad=((size+9+63)>>6)<<6,data=new Uint8Array(pad);data.set(bytes);data[size]=128;
  const bitLen=size*8;for(let i=0;i<8;i++)data[pad-1-i]=Math.floor(bitLen/2**(i*8))&255;
  const rr=(v,n)=>(v>>>n)|(v<<(32-n)),w=new Uint32Array(64);
  for(let offset=0;offset<pad;offset+=64){
    for(let i=0;i<16;i++){let j=offset+i*4;w[i]=(data[j]<<24)|(data[j+1]<<16)|(data[j+2]<<8)|data[j+3]}
    for(let i=16;i<64;i++){let a=w[i-15],b=w[i-2];w[i]=(w[i-16]+(rr(a,7)^rr(a,18)^(a>>>3))+w[i-7]+(rr(b,17)^rr(b,19)^(b>>>10)))|0}
    let [a,b,c,d,e,f,g,q]=h;
    for(let i=0;i<64;i++){let t1=(q+(rr(e,6)^rr(e,11)^rr(e,25))+((e&f)^(~e&g))+k[i]+w[i])|0,t2=((rr(a,2)^rr(a,13)^rr(a,22))+((a&b)^(a&c)^(b&c)))|0;q=g;g=f;f=e;e=(d+t1)|0;d=c;c=b;b=a;a=(t1+t2)|0}
    [a,b,c,d,e,f,g,q].forEach((v,i)=>h[i]=(h[i]+v)|0)
  }
  return h.map(x=>(x>>>0).toString(16).padStart(8,'0')).join('')
}
const versionParts=v=>v.split('.').map(Number);
function newer(a,b){const x=versionParts(a),y=versionParts(b);for(let i=0;i<3;i++){if((x[i]||0)>(y[i]||0))return true;if((x[i]||0)<(y[i]||0))return false}return false}
function sha1Blob(bytes){
  const prefix=new TextEncoder().encode('blob '+bytes.length+'\0'),raw=new Uint8Array(prefix.length+bytes.length);
  raw.set(prefix);raw.set(bytes,prefix.length);
  const length=raw.length,padded=((length+9+63)>>6)<<6,data=new Uint8Array(padded);data.set(raw);data[length]=128;
  const bitLength=length*8;for(let i=0;i<8;i++)data[padded-1-i]=Math.floor(bitLength/2**(i*8))&255;
  const rol=(v,n)=>(v<<n)|(v>>>(32-n)),h=[0x67452301,0xefcdab89,0x98badcfe,0x10325476,0xc3d2e1f0],w=new Uint32Array(80);
  for(let offset=0;offset<padded;offset+=64){
    for(let i=0;i<16;i++){const j=offset+i*4;w[i]=(data[j]<<24)|(data[j+1]<<16)|(data[j+2]<<8)|data[j+3]}
    for(let i=16;i<80;i++)w[i]=rol(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
    let [a,b,c,d,e]=h;
    for(let i=0;i<80;i++){
      const f=i<20?(b&c)|(~b&d):i<40?b^c^d:i<60?(b&c)|(b&d)|(c&d):b^c^d;
      const k=i<20?0x5a827999:i<40?0x6ed9eba1:i<60?0x8f1bbcdc:0xca62c1d6;
      const t=(rol(a,5)+f+e+k+w[i])|0;e=d;d=c;c=rol(b,30);b=a;a=t
    }
    [a,b,c,d,e].forEach((v,i)=>h[i]=(h[i]+v)|0)
  }
  return h.map(x=>(x>>>0).toString(16).padStart(8,'0')).join('')
}
async function fetchUpdateInfo(){
  let response;
  try{response=await fetch(GITHUB_MANIFEST+'?t='+Date.now(),{cache:'no-store'})}catch(_){}
  if(response?.ok){
    const m=await response.json();
    if(!/^\d+\.\d+(?:\.\d+)?$/.test(m.version)||!/^(?:LayerGlow|(?:K2_)?Status_Light)_v[0-9_]+_OTA\.bin$/.test(m.file)||!Number.isInteger(m.size)||m.size<100000||m.size>1966080||!(/^[a-f0-9]{64}$/i.test(m.sha256)))throw Error(T[lang].githubFormat);
    return m
  }
  const listResponse=await fetch(GITHUB_LIST+'&t='+Date.now(),{cache:'no-store'});
  if(!listResponse.ok)throw Error('HTTP '+listResponse.status+' · '+T[lang].githubNetwork);
  const entries=await listResponse.json();
  if(!Array.isArray(entries))throw Error(T[lang].githubFormat);
  const candidates=entries.filter(x=>x.type==='file'&&/^(?:LayerGlow|(?:K2_)?Status_Light)_v\d+(?:_\d+){1,2}_OTA\.bin$/.test(x.name)&&Number.isInteger(x.size)&&x.size>100000&&x.size<=1966080&&/^[a-f0-9]{40}$/i.test(x.sha||''))
    .map(x=>({version:x.name.match(/^(?:LayerGlow|(?:K2_)?Status_Light)_v(.+)_OTA\.bin$/)[1].replaceAll('_','.'),file:x.name,size:x.size,sha:x.sha.toLowerCase()}));
  if(!candidates.length)throw Error(T[lang].githubMissing);
  candidates.sort((a,b)=>newer(a.version,b.version)?-1:newer(b.version,a.version)?1:0);
  return candidates[0]
}
async function checkGitHub(silent=false){
  if(updatingFirmware||silent&&lastStatus?.ap)return;
  const msg=document.getElementById('githubMessage'),button=document.getElementById('checkGitHub'),install=document.getElementById('installGitHub');
  if(!silent)msg.textContent=T[lang].checkingUpdates;
  button.disabled=true;
  try{
    const m=await fetchUpdateInfo(),previous=availableUpdate?.version;
    availableUpdate=newer(m.version,lastStatus.version||'0.0')?m:null;updateUpdateButton();
    install.hidden=!availableUpdate;
    msg.textContent=availableUpdate?T[lang].newVersion+' '+m.version:T[lang].noUpdate;
    fetch('/api/update/available',{method:'POST',body:new URLSearchParams({available:availableUpdate?'1':'0'})}).catch(()=>{});
    if(availableUpdate&&previous!==m.version)toast(msg.textContent);
  }catch(e){availableUpdate=null;updateUpdateButton();install.hidden=true;msg.textContent=e instanceof TypeError?T[lang].githubNetwork:e.message||T[lang].githubUnavailable;if(!silent)toast(msg.textContent,true)}
  finally{button.disabled=false}
}
async function uploadFirmware(file,button){
  button.disabled=true;updatingFirmware=true;
  const msg=document.getElementById('fwMessage'),began=Date.now();
  const ticker=setInterval(()=>{if(!msg.dataset.uploading)msg.textContent=T[lang].waiting+' '+Math.floor((Date.now()-began)/1000)+' s'},1000);
  try{
    const head=new Uint8Array(await file.slice(0,1).arrayBuffer());
    if(head[0]!==0xe9)throw Error(T[lang].invalidFirmware);
    const capacity=await api('/api/ota/capacity');
    if(!capacity.appSlotBytes||file.size>capacity.appSlotBytes)throw Error(T[lang].noSpace+' ('+file.size+' / '+capacity.appSlotBytes+' bytes)');
    const f=new FormData();f.append('firmware',file,file.name);msg.dataset.uploading='1';msg.textContent=T[lang].restarting+' 0%';
    let confirmed=false;
    try{await new Promise((resolve,reject)=>{
      const xhr=new XMLHttpRequest();xhr.open('POST','/api/update');
      xhr.upload.onprogress=e=>{if(e.lengthComputable)msg.textContent=T[lang].restarting+' '+Math.round(e.loaded*100/e.total)+'%'};
      xhr.onload=()=>xhr.status===200?resolve():reject(Error(xhr.responseText||'HTTP '+xhr.status));
      xhr.onerror=()=>reject(new TypeError('Device restarting'));xhr.send(f);
    });confirmed=true}catch(err){if(!(err instanceof TypeError))throw err}
    delete msg.dataset.uploading;
    await waitForRestart(confirmed,lastStatus.version);
  }catch(err){msg.textContent=T[lang].updateFailed+': '+localError(err.message);toast(msg.textContent,true)}
  finally{delete msg.dataset.uploading;clearInterval(ticker);button.disabled=false;updatingFirmware=false}
}
document.getElementById('checkGitHub').onclick=()=>checkGitHub();
document.getElementById('installGitHub').onclick=async e=>{
  const m=availableUpdate;if(!m||!confirm(T[lang].githubConfirm.replace('{version}',m.version)))return;
  const button=e.target,msg=document.getElementById('githubMessage');button.disabled=true;
  try{
    msg.textContent=T[lang].downloadFirmware;
    const url='https://raw.githubusercontent.com/TomaszK00/LayerGlow/main/Software/'+m.file;
    const response=await fetch(url+'?t='+Date.now(),{cache:'no-store'});
    if(!response.ok)throw Error('HTTP '+response.status);
    const bytes=new Uint8Array(await response.arrayBuffer());
    if(bytes.length!==m.size||bytes[0]!==0xe9||(m.sha256?sha256(bytes)!==m.sha256.toLowerCase():sha1Blob(bytes)!==m.sha))throw Error(T[lang].invalidFirmware);
    await uploadFirmware(new File([bytes],m.file,{type:'application/octet-stream'}),button);
  }catch(err){msg.textContent=err.message;toast(msg.textContent,true)}
  finally{button.disabled=false}
};
document.getElementById('fwform').onsubmit=async e=>{
  e.preventDefault();const file=document.getElementById('bin').files[0];
  if(!file||!file.name.toLowerCase().endsWith('.bin')){document.getElementById('fwMessage').textContent=T[lang].chooseBin;return}
  if(confirm(T[lang].uploadConfirm))await uploadFirmware(file,e.target.querySelector('button'));
};

setLang(T[localStorage.getItem('k2lang')]?localStorage.getItem('k2lang'):'en');refresh().then(()=>setTimeout(()=>checkGitHub(true),2500));setInterval(refresh,2000);setInterval(()=>checkGitHub(true),30*60*1000);setInterval(()=>paintStatus(lastStatus),120);
</script></body></html>
)HTML";

String normalizeColor(String c, const String& fallback) {
  if(c.length()!=7 || c[0]!='#')return fallback;
  for(int i=1;i<7;i++)if(!isxdigit((unsigned char)c[i]))return fallback;
  c.toLowerCase();return c;
}
uint32_t color24(const String& h) {return strtoul(h.c_str()+1,nullptr,16);}
int strictCount(const String& value,int low,int high){
  if(value.isEmpty()||value.length()>6)return -1;
  for(char c:value)if(c<'0'||c>'9')return -1;
  int n=value.toInt();return n>=low&&n<=high?n:-1;
}
void loadPalette(Preferences& storage,int i){
  rowColorCount[i]=constrain((int)storage.getUChar((String("pn")+i).c_str(),1),1,12);
  for(int j=0;j<11;j++)rowExtraColors[i][j]=normalizeColor(storage.getString((String("pc")+i+j).c_str(),DEFAULT_EXTRA_COLORS[j]),DEFAULT_EXTRA_COLORS[j]);
}
bool savePalette(Preferences& storage,int i){
  bool ok=storage.putUChar((String("pn")+i).c_str(),rowColorCount[i])>0;
  for(int j=0;j<11;j++)ok=(storage.putString((String("pc")+i+j).c_str(),rowExtraColors[i][j])>0)&&ok;
  return ok;
}
int paletteSlot(int pixel,int total,int count){return min(count-1,pixel*count/total);}
uint32_t pixelColour(int row,int pixel){int slot=paletteSlot(pixel,activeLedCount,rowColorCount[row]);return color24(slot==0?rowColors[row]:rowExtraColors[row][slot-1]);}
uint32_t blendColour(uint32_t a,uint32_t b,uint32_t fraction){
  uint32_t colour=0;
  for(int shift=16;shift>=0;shift-=8){uint32_t channel=(((a>>shift)&255)*(65536-fraction)+((b>>shift)&255)*fraction+32768)/65536;colour|=channel<<shift;}
  return colour;
}
uint32_t samplePalette(int row,uint32_t position,bool smooth){
  uint32_t scaled=position*rowColorCount[row];int slot=scaled/65536;int next=(slot+1)%rowColorCount[row];
  uint32_t a=color24(slot==0?rowColors[row]:rowExtraColors[row][slot-1]);
  if(!smooth)return a;
  uint32_t b=color24(next==0?rowColors[row]:rowExtraColors[row][next-1]);
  return blendColour(a,b,scaled%65536);
}
uint32_t effectColour(int row,int pixel,int style,uint32_t tick){
  uint32_t position=((uint32_t)pixel*65536/activeLedCount+65536-tick)%65536;
  if(style==10)return samplePalette(row,position,true);
  if(style==11)return samplePalette(row,tick,true);
  if(style==12){const uint32_t rainbow[]={0xff0000,0xffff00,0x00ff00,0x00ffff,0x0000ff,0xff00ff};uint32_t scaled=position*6;int slot=scaled/65536;return blendColour(rainbow[slot],rainbow[(slot+1)%6],scaled%65536);}
  if(style==13)return samplePalette(row,position,false);
  return pixelColour(row,pixel);
}
void loadSettings(){
  prefs.begin("k2led",true);
  webHost=prefs.getString("host","layerglow");
  if(prefs.isKey("wcount")){
    wifiProfileCount=min((int)prefs.getUChar("wcount",0),(int)MAX_WIFI_PROFILES);
    activeWifi=wifiProfileCount?min((int)prefs.getUChar("wactive",0),(int)wifiProfileCount-1):0;
    for(int i=0;i<wifiProfileCount;i++){
      wifiProfiles[i].name=prefs.getString((String("ws")+i).c_str(),"");
      wifiProfiles[i].password=prefs.getString((String("wp")+i).c_str(),"");
    }
  }else{
    String oldSSID=prefs.getString("ssid","");
    if(oldSSID.length()){
      wifiProfiles[0].name=oldSSID;wifiProfiles[0].password=prefs.getString("pass","");
      wifiProfileCount=1;
    }
    migrateLegacyWifi=true;
  }
  if(wifiProfileCount){ssid=wifiProfiles[activeWifi].name;wifiPassword=wifiProfiles[activeWifi].password;}
  printerIP=prefs.getString("ip","192.168.1.9");printerPort=prefs.getUShort("port",7125);
  printerType=prefs.getString("ptype","moonraker");printerEnabled=prefs.getBool("penabled",true);
  if(printerType!="bambu"&&printerType!="octoprint"&&printerType!="prusalink")printerType="moonraker";
  moonIP=prefs.getString("moonip",printerType=="moonraker"?printerIP:"192.168.1.9");
  bambuIP=prefs.getString("bambuip",printerType=="bambu"?printerIP:"");
  bambuSerial=prefs.getString("bserial","");
  bambuAccessCode=prefs.getString("bcode","");
  octoIP=prefs.getString("octoip",printerType=="octoprint"?printerIP:"");prusaIP=prefs.getString("prusaip",printerType=="prusalink"?printerIP:"");
  octoKey=prefs.getString("octokey","");prusaKey=prefs.getString("prusakey","");
  moonPort=prefs.getUShort("moonport",printerPort);octoPort=prefs.getUShort("octoport",80);prusaPort=prefs.getUShort("prusaport",80);
  printerIP=printerType=="bambu"?bambuIP:printerType=="octoprint"?octoIP:printerType=="prusalink"?prusaIP:moonIP;
  printerPort=printerType=="octoprint"?octoPort:printerType=="prusalink"?prusaPort:moonPort;
  intervalMs=constrain(prefs.getInt("intvl",500),100,10000);
  activeLedCount=constrain(prefs.getInt("ledcnt",24),5,MAX_LED_COUNT);
  int oldBright=constrain(prefs.getInt("bright",65),0,100);
  int oldPeriod=constrain(prefs.getInt("pulsems",1400),200,10000);
  uint8_t oldMask=prefs.isKey("pmask")?prefs.getUChar("pmask",0):0;
  bool oldLast=prefs.getBool("plast",false);
  if(!prefs.isKey("pmask")){String mode=prefs.getString("pulsemode","off");oldMask=mode=="all"?0x1f:(mode=="last"?1:0);oldLast=mode=="last";}
  for(int i=0;i<6;i++){
    String key=String(i);
    String oldColor=i==0?prefs.getString("color",DEFAULT_ROW_COLORS[0]):prefs.getString((String("c")+i).c_str(),DEFAULT_ROW_COLORS[i]);
    rowColors[i]=normalizeColor(prefs.getString((String("rc")+key).c_str(),oldColor),DEFAULT_ROW_COLORS[i]);loadPalette(prefs,i);
    rowBrightness[i]=constrain(prefs.getInt((String("rb")+key).c_str(),oldBright),0,100);
    rowPulse[i]=prefs.getBool((String("re")+key).c_str(),(oldMask&(1<<i))!=0);
    rowEnabled[i]=prefs.getBool((String("ri")+key).c_str(),true);
    rowStyle[i]=constrain((int)prefs.getUChar((String("rs")+key).c_str(),(i==0&&oldLast)?1:0),0,13);if(rowStyle[i]==4)rowStyle[i]=3;
    rowPeriod[i]=constrain(prefs.getInt((String("rp")+key).c_str(),oldPeriod),0,5000);
  }
  prefs.end();
}
void saveWifiSettings(){
  if(!prefs.begin("k2led",false))return;
  prefs.putString("host",webHost);
  for(int i=0;i<MAX_WIFI_PROFILES;i++){
    String nameKey=String("ws")+i,passwordKey=String("wp")+i;
    if(i<wifiProfileCount){prefs.putString(nameKey.c_str(),wifiProfiles[i].name);prefs.putString(passwordKey.c_str(),wifiProfiles[i].password);}
    else{prefs.remove(nameKey.c_str());prefs.remove(passwordKey.c_str());}
  }
  prefs.putUChar("wactive",activeWifi);prefs.putUChar("wcount",wifiProfileCount);
  prefs.remove("ssid");prefs.remove("pass");prefs.end();
}
void savePrinterSettings(){prefs.begin("k2led",false);prefs.putString("ip",printerIP);prefs.putUShort("port",printerPort);prefs.putString("ptype",printerType);prefs.putBool("penabled",printerEnabled);prefs.putString("moonip",moonIP);prefs.putString("bambuip",bambuIP);prefs.putString("octoip",octoIP);prefs.putString("prusaip",prusaIP);prefs.putUShort("moonport",moonPort);prefs.putUShort("octoport",octoPort);prefs.putUShort("prusaport",prusaPort);prefs.putString("bserial",bambuSerial);prefs.putString("bcode",bambuAccessCode);prefs.putString("octokey",octoKey);prefs.putString("prusakey",prusaKey);prefs.end();}
void saveLighting(){
  prefs.begin("k2led",false);
  if(prefs.getInt("intvl",-1)!=intervalMs)prefs.putInt("intvl",intervalMs);
  if(prefs.getInt("ledcnt",-1)!=activeLedCount)prefs.putInt("ledcnt",activeLedCount);
  for(int i=0;i<6;i++){
    String key=String(i);
    auto saveString=[&](const String& field,const String& value){if(prefs.getString(field.c_str(),"")!=value)prefs.putString(field.c_str(),value);};
    auto saveInt=[&](const String& field,int value){if(prefs.getInt(field.c_str(),-1)!=value)prefs.putInt(field.c_str(),value);};
    saveString("rc"+key,rowColors[i]);
    String paletteCount="pn"+key;
    if(prefs.getUChar(paletteCount.c_str(),0)!=rowColorCount[i])prefs.putUChar(paletteCount.c_str(),rowColorCount[i]);
    for(int j=0;j<11;j++)saveString("pc"+key+j,rowExtraColors[i][j]);
    saveInt("rb"+key,rowBrightness[i]);
    String pulse="re"+key,enabled="ri"+key;
    if(!prefs.isKey(pulse.c_str())||prefs.getBool(pulse.c_str(),!rowPulse[i])!=rowPulse[i])prefs.putBool(pulse.c_str(),rowPulse[i]);
    if(!prefs.isKey(enabled.c_str())||prefs.getBool(enabled.c_str(),!rowEnabled[i])!=rowEnabled[i])prefs.putBool(enabled.c_str(),rowEnabled[i]);
    String style="rs"+key;
    if(prefs.getUChar(style.c_str(),255)!=rowStyle[i])prefs.putUChar(style.c_str(),rowStyle[i]);
    saveInt("rp"+key,rowPeriod[i]);
  }
  prefs.end();
}
void updateLEDs();
bool saveLightingBackup(){
  Preferences backup;
  if(!backup.begin("k2ledbak",false))return false;
  backup.putBool("valid",false);
  bool ok=backup.putInt("intvl",intervalMs)>0 && backup.putInt("ledcnt",activeLedCount)>0;
  for(int i=0;i<6;i++){
    String key=String(i);
    ok=(backup.putString((String("rc")+key).c_str(),rowColors[i])>0)&&ok;ok=savePalette(backup,i)&&ok;
    ok=(backup.putInt((String("rb")+key).c_str(),rowBrightness[i])>0)&&ok;
    ok=(backup.putBool((String("re")+key).c_str(),rowPulse[i])>0)&&ok;
    ok=(backup.putBool((String("ri")+key).c_str(),rowEnabled[i])>0)&&ok;
    ok=(backup.putUChar((String("rs")+key).c_str(),rowStyle[i])>0)&&ok;
    ok=(backup.putInt((String("rp")+key).c_str(),rowPeriod[i])>0)&&ok;
  }
  lightingBackupAvailable=ok && backup.putBool("valid",true)>0;
  backup.end();
  return lightingBackupAvailable;
}
bool restoreLightingBackup(){
  Preferences backup;
  if(!backup.begin("k2ledbak",true))return false;
  if(!backup.getBool("valid",false)){backup.end();lightingBackupAvailable=false;return false;}
  String colors[6];uint8_t brightness[6],styles[6];bool pulse[6],enabled[6];uint16_t periods[6];
  int count=constrain(backup.getInt("ledcnt",24),5,MAX_LED_COUNT);
  int interval=constrain(backup.getInt("intvl",500),100,10000);
  for(int i=0;i<6;i++){
    String key=String(i);
    colors[i]=normalizeColor(backup.getString((String("rc")+key).c_str(),DEFAULT_ROW_COLORS[i]),DEFAULT_ROW_COLORS[i]);loadPalette(backup,i);
    brightness[i]=constrain(backup.getInt((String("rb")+key).c_str(),65),0,100);
    pulse[i]=backup.getBool((String("re")+key).c_str(),false);
    enabled[i]=backup.getBool((String("ri")+key).c_str(),true);
    styles[i]=constrain((int)backup.getUChar((String("rs")+key).c_str(),0),0,13);
    if(styles[i]==4)styles[i]=3;
    periods[i]=constrain(backup.getInt((String("rp")+key).c_str(),1400),0,5000);
  }
  backup.end();
  activeLedCount=count;intervalMs=interval;
  for(int i=0;i<6;i++){rowColors[i]=colors[i];rowBrightness[i]=brightness[i];rowPulse[i]=pulse[i];rowEnabled[i]=enabled[i];rowStyle[i]=styles[i];rowPeriod[i]=periods[i];}
  saveLighting();updateLEDs();
  return true;
}
void addRows(JsonDocument& d){for(int i=0;i<6;i++){const char* name=i==0?"progress":stateNames[i];d["rows"][name]["color"]=rowColors[i];d["rows"][name]["colourCount"]=rowColorCount[i];JsonArray palette=d["rows"][name]["colours"].to<JsonArray>();palette.add(rowColors[i]);for(int j=0;j<11;j++)palette.add(rowExtraColors[i][j]);d["rows"][name]["brightness"]=rowBrightness[i];d["rows"][name]["pulse"]=rowPulse[i];d["rows"][name]["enabled"]=rowEnabled[i];d["rows"][name]["period"]=rowPeriod[i];d["rows"][name]["style"]=rowStyle[i];}}

int statusIndex(String st){if(st=="printing")return 0;if(st=="heating")return 1;if(st=="paused")return 2;if(st=="complete")return 3;if(st=="error")return 4;return 5;}
String normalState(String s){s.toLowerCase();if(s=="complete"||s=="completed")return "complete";if(s=="printing")return "printing";if(s=="paused")return "paused";if(s=="error"||s=="cancelled")return "error";if(s=="heating")return "heating";return "standby";}
void updateLEDs(){
  if(!LED_ENABLED)return;
  if(strip.numPixels()!=activeLedCount){strip.clear();strip.show();strip.updateLength(activeLedCount);}
  String st=simEnabled?String(stateNames[simStatus]):normalState(printerState);
  if(!simEnabled&&!printerConnected)st="standby";
  int idx=statusIndex(st),pairs=(activeLedCount+1)/2;
  int count=st=="printing"?(int)ceilf((simEnabled?simProgress:printerProgress*100.0f)*pairs/100.0f):(st=="standby"&&!rowPulse[idx]&&rowStyle[idx]<6?0:pairs);
  count=constrain(count,0,pairs);if(!rowEnabled[idx])count=0;
  uint32_t effectivePeriod=rowPeriod[idx]?rowPeriod[idx]:1400;
  uint32_t cycleMs=millis()%effectivePeriod,colourTick=cycleMs*65536UL/effectivePeriod;
  float cycle=cycleMs/(float)effectivePeriod;
  float scale=0.625f-0.375f*cosf(6.2831853f*cycle);
  float travel=cycle*2.0f*fmaxf(0,count-1);float head=travel<=count-1?travel:2.0f*(count-1)-travel;float centre=(count-1)/2.0f;float radius=fmaxf(0.5f,centre);float outward=1.0f-fabsf(2.0f*cycle-1.0f);float front=outward*radius;
  static bool randomPairOn[MAX_LED_PAIRS]={};
  static uint32_t randomPairNext[MAX_LED_PAIRS]={};
  static int randomState=-1,randomLength=0;
  uint32_t randomNow=millis();
  bool randomEffect=rowStyle[idx]==6;
  if(!randomEffect)randomState=-1;
  else if(randomState!=idx||randomLength!=activeLedCount){
    randomState=idx;randomLength=activeLedCount;
    for(int pair=0;pair<MAX_LED_PAIRS;pair++){
      randomPairOn[pair]=false;
      randomPairNext[pair]=randomNow+random(20,180);
    }
  }
  if(randomEffect){
    uint32_t base=constrain((int)rowPeriod[idx]/12,55,220);
    for(int pair=0;pair<pairs;pair++){
      if((int32_t)(randomNow-randomPairNext[pair])>=0){
        randomPairOn[pair]=!randomPairOn[pair];
        randomPairNext[pair]=randomNow+random(base/2,base*2+1);
      }
    }
  }
  strip.setBrightness(map(rowBrightness[idx],0,100,0,255));
  for(int i=0;i<activeLedCount;i++){
    uint32_t rgb=effectColour(idx,i,rowStyle[idx],colourTick);uint8_t r=(rgb>>16)&255,g=(rgb>>8)&255,b=rgb&255;
    int pair=i/2;bool lit=i<activeLedCount&&pair<count&&(!randomEffect||randomPairOn[pair]);float level=1.0f;
    if(lit&&rowPulse[idx]&&rowPeriod[idx]>0){
      if(rowStyle[idx]==2)level=fmaxf(0.08f,1.0f-fabsf(pair-head)/2.2f);
      else if(rowStyle[idx]==3)level=fmaxf(0.08f,1.0f-fabsf(fabsf(pair-centre)-front)/1.3f);
      else if(rowStyle[idx]==5)level=0.16f+0.84f*(0.5f+0.5f*cosf(6.2831853f*(pair/fmaxf(1,count-1)-cycle)));
      else if(rowStyle[idx]==0||rowStyle[idx]>=10||(rowStyle[idx]==1&&pair==count-1))level=scale;
    }
    if(lit&&rowStyle[idx]==7){
      float trail=fmodf(cycle*count-pair+count,count);
      level=fmaxf(0.0f,1.0f-trail/3.2f);
      lit=level>0.02f;
    }else if(lit&&rowStyle[idx]==8){
      int step=(int)(cycle*2*count);
      lit=step<count?pair<=step:pair>step-count;
    }else if(lit&&rowStyle[idx]==9){
      lit=(pair%2)==(int)(cycle*2);
    }
    strip.setPixelColor(i,lit?strip.Color((uint8_t)(r*level),(uint8_t)(g*level),(uint8_t)(b*level),0):strip.Color(0,0,0,0));
  }
  strip.show();
}

bool startRecoveryAP(){
  if(apMode)return true;
  lastApAttempt=millis();
  WiFi.mode(WIFI_AP_STA);
  if(!WiFi.softAP(AP_NAME,AP_PASSWORD)){
    apMode=false;
    Serial.println("Setup AP failed; retrying");
    return false;
  }
  apMode=true;
  Serial.println("Setup AP: LayerGlow-Setup · http://"+WiFi.softAPIP().toString());
  return true;
}
bool validWebHost(const String& name){
  if(name.length()<1||name.length()>32||name[0]=='-'||name[name.length()-1]=='-')return false;
  for(char c:name)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'))return false;
  return true;
}
void maintainMdns(uint32_t now){
  extern volatile bool discoveryRunning;
  if(WiFi.status()!=WL_CONNECTED||discoveryRunning||otaUploadInProgress)return;
  if(mdnsStarted&&!mdnsNeedsRestart&&mdnsHostActive==webHost)return;
  if(mdnsChangeAt&&(int32_t)(now-mdnsChangeAt)<0)return;
  if(lastMdnsAttempt&&now-lastMdnsAttempt<15000)return;
  lastMdnsAttempt=now;
  if(mdnsStarted){MDNS.end();mdnsStarted=false;}
  mdnsStarted=MDNS.begin(webHost);
  if(mdnsStarted){mdnsHostActive=webHost;mdnsNeedsRestart=false;mdnsChangeAt=0;MDNS.addService("http","tcp",80);Serial.println("Panel: http://"+webHost+".local/");}
  else Serial.println("mDNS unavailable; local IP and setup AP remain accessible");
}
void connectWifiProfile(uint8_t index,uint32_t now){
  if(index>=wifiProfileCount)return;
  activeWifi=index;ssid=wifiProfiles[index].name;wifiPassword=wifiProfiles[index].password;
  WiFi.disconnect(false);
  WiFi.begin(ssid.c_str(),wifiPassword.c_str());
  wifiAttemptStarted=now;lastWifiAttempt=now;wifiConnectedSince=0;
  Serial.println("Trying saved Wi-Fi: "+ssid);
}
void startNetwork(){
  // DHCP hostname must be set before startRecoveryAP() enables Wi-Fi.
  // The router name follows the configurable .local host on each boot.
  if(!WiFi.setHostname(webHost.equalsIgnoreCase("layerglow") ? "LayerGlow" : webHost.c_str()))
    Serial.println("Could not set Wi-Fi hostname");
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  // Bring up the rescue AP immediately; do not block the web server for 10 seconds.
  startRecoveryAP();
  if(wifiProfileCount)connectWifiProfile(activeWifi,millis());
}
void maintainNetwork(uint32_t now){
  if(otaUploadInProgress)return;
  if(wifiScanActive){
    if(now-wifiScanStarted<16000)return;
    WiFi.scanDelete();wifiScanActive=false;
  }
  if(wifiChangePending&&(int32_t)(now-wifiChangeAt)>=0){
    wifiChangePending=false;
    startRecoveryAP();
    if(wifiProfileCount)connectWifiProfile(activeWifi,now);
    else{WiFi.disconnect(false);ssid="";wifiPassword="";wifiAttemptStarted=0;wifiConnectedSince=0;}
    wifiLostSince=now;
    return;
  }
  if(wifiChangePending)return;
  if(WiFi.status()==WL_CONNECTED){
    if(!wifiConnectedSince){
      wifiConnectedSince=now;
      configTime(0,0,"pool.ntp.org","time.cloudflare.com");
      mdnsNeedsRestart=true;
      if(wifiProfileCount){
        if(prefs.begin("k2led",false)){if(prefs.getUChar("wactive",255)!=activeWifi)prefs.putUChar("wactive",activeWifi);prefs.end();}
      }
      Serial.println("Wi-Fi connected: http://"+WiFi.localIP().toString());
    }
    wifiLostSince=0;
    maintainMdns(now);
    // Never remove an AP while a phone is using it for recovery.
    if(apMode&&now-wifiConnectedSince>=60000&&WiFi.softAPgetStationNum()==0){
      if(WiFi.softAPdisconnect(true)){apMode=false;Serial.println("Setup AP closed");}
    }
    return;
  }
  wifiConnectedSince=0;
  mdnsNeedsRestart=true;
  if(!wifiLostSince){wifiLostSince=now;wifiAttemptStarted=now;}
  if(now-wifiLostSince>=8000&&!apMode&&now-lastApAttempt>=8000)startRecoveryAP();
  if(!wifiProfileCount||wifiScanActive)return;
  if(apMode&&WiFi.softAPgetStationNum()>0)return;
  if(wifiProfileCount==1){
    if(now-wifiAttemptStarted>=20000){wifiAttemptStarted=now;WiFi.reconnect();}
    return;
  }
  if(now-wifiAttemptStarted<12000)return;
  // Try each remembered network without shutting down the recovery AP.
  connectWifiProfile((activeWifi+1)%wifiProfileCount,now);
}
void pollPrinter(){
  if(WiFi.status()!=WL_CONNECTED){printerConnected=false;lastError="No printer connection (setup AP)";return;}
  WiFiClient client;HTTPClient http;
  String url="http://"+printerIP+":"+String(printerPort)+"/printer/objects/query?print_stats&virtual_sdcard";
  http.setConnectTimeout(700);http.setTimeout(1000);
  if(!http.begin(client,url)){printerConnected=false;lastError="Invalid Moonraker URL";return;}
  int response=http.GET();if(response!=200){printerConnected=false;lastError="HTTP "+String(response);http.end();return;}
  JsonDocument doc;DeserializationError err=deserializeJson(doc,http.getStream());http.end();
  if(err){printerConnected=false;lastError=String("JSON: ")+err.c_str();return;}
  JsonVariant status=doc["result"]["status"];
  if(status["print_stats"].isNull()||status["virtual_sdcard"].isNull()){printerConnected=false;lastError="Missing printer fields";return;}
  printerState=status["print_stats"]["state"].as<String>();printerFile=status["print_stats"]["filename"].as<String>();
  printerProgress=constrain(status["virtual_sdcard"]["progress"].as<float>(),0.f,1.f);
  printerConnected=true;lastError="";lastPrinterReadAt=millis();
}
void pollHttpPrinter(){
  if(WiFi.status()!=WL_CONNECTED){printerConnected=false;lastError="No printer connection (setup AP)";return;}
  WiFiClient client;HTTPClient http;
  const bool octo=printerType=="octoprint";
  String url="http://"+printerIP+":"+String(printerPort)+(octo?"/api/job":"/api/v1/status");
  http.setConnectTimeout(700);http.setTimeout(1000);
  if(!http.begin(client,url)){printerConnected=false;lastError="Invalid printer URL";return;}
  http.addHeader("X-Api-Key",octo?octoKey:prusaKey);
  int response=http.GET();if(response!=200){printerConnected=false;lastError="HTTP "+String(response);http.end();return;}
  JsonDocument doc;DeserializationError err=deserializeJson(doc,http.getStream());http.end();
  if(err){printerConnected=false;lastError=String("JSON: ")+err.c_str();return;}
  String state=octo?doc["state"].as<String>():doc["printer"]["state"].as<String>();state.toLowerCase();
  if(state.isEmpty()){printerConnected=false;lastError="Missing printer fields";return;}
  if(state=="printing"||state=="pausing"||state=="resuming")printerState="printing";
  else if(state=="paused")printerState="paused";
  else if(state=="finished"||state=="complete")printerState="complete";
  else if(state=="error"||state=="attention"||state=="cancelling"||state=="offline")printerState="error";
  else printerState="standby";
  float percent=octo?doc["progress"]["completion"].as<float>():doc["job"]["progress"].as<float>();
  printerProgress=constrain(percent/100.f,0.f,1.f);
  printerFile=octo?doc["job"]["file"]["name"].as<String>():String("");
  printerConnected=true;lastError="";lastPrinterReadAt=millis();
}
void onBambuReport(char* topic,byte* payload,unsigned int length){
  if(!printerEnabled)return;
  JsonDocument doc;
  if(deserializeJson(doc,payload,length))return;
  JsonVariant p=doc["print"];
  if(p.isNull())return;
  bool gotStatus=false;
  if(!p["gcode_state"].isNull()){
    String state=p["gcode_state"].as<String>();state.toUpperCase();
    if(state=="RUNNING")printerState="printing";
    else if(state=="PREPARE"||state=="SLICING")printerState="heating";
    else if(state=="PAUSE")printerState="paused";
    else if(state=="FINISH")printerState="complete";
    else if(state=="FAILED")printerState="error";
    else if(state=="IDLE")printerState="standby";
    gotStatus=true;
  }
  if(!p["mc_percent"].isNull()){printerProgress=constrain(p["mc_percent"].as<float>()/100.f,0.f,1.f);gotStatus=true;}
  if(!p["subtask_name"].isNull()){printerFile=p["subtask_name"].as<String>();gotStatus=true;}
  if(!gotStatus)return;
  lastBambuReport=millis();printerConnected=true;lastError="";lastPrinterReadAt=lastBambuReport;
  if(!simEnabled)updateLEDs();
}
void maintainBambu(uint32_t now){
  if(!printerEnabled||printerType!="bambu"||WiFi.status()!=WL_CONNECTED||otaUploadInProgress){
    if(bambuMqtt.connected())bambuMqtt.disconnect();
    if(bambuMqtt.getBufferSize()>256)bambuMqtt.setBufferSize(256);
    return;
  }
  if(!bambuMqtt.connected()){
    printerConnected=false;
    if(lastError!="Bambu access denied")lastError="Bambu MQTT offline";
    if(lastBambuAttempt&&now-lastBambuAttempt<30000)return;
    lastBambuAttempt=now;
    if(bambuIP.isEmpty()||bambuSerial.isEmpty()||bambuAccessCode.isEmpty()){lastError="Bambu settings missing";return;}
    if(bambuMqtt.getBufferSize()<32768&&!bambuMqtt.setBufferSize(32768)){lastError="Not enough memory for Bambu MQTT";return;}
    bambuMqtt.setServer(bambuIP.c_str(),8883);
    String clientID="layerglow-"+WiFi.macAddress();clientID.replace(":","");
    if(!bambuMqtt.connect(clientID.c_str(),"bblp",bambuAccessCode.c_str())){
      int result=bambuMqtt.state();
      lastError=(result==4||result==5)?"Bambu access denied":"Bambu MQTT offline";
      return;
    }
    String report="device/"+bambuSerial+"/report";
    if(!bambuMqtt.subscribe(report.c_str())){bambuMqtt.disconnect();return;}
    lastBambuReport=now;
    lastBambuRequest=0;
  }
  if(!bambuMqtt.loop()){printerConnected=false;lastError="Bambu MQTT offline";return;}
  if(now-lastBambuReport>35000){bambuMqtt.disconnect();printerConnected=false;lastError="Bambu status timeout";return;}
  if(!lastBambuRequest||now-lastBambuRequest>=30000){
    String request="device/"+bambuSerial+"/request";
    bambuMqtt.publish(request.c_str(),"{\"pushing\":{\"command\":\"pushall\",\"push_target\":1,\"sequence_id\":\"1\",\"version\":1}}");
    lastBambuRequest=now;
  }
}
struct PrinterCandidate {String type, name, ip, serial; uint16_t port;};
PrinterCandidate discoveryResults[12];
int discoveryCount=0;
volatile bool discoveryRunning=false;
volatile uint16_t discoveryChecked=0, discoveryTotal=0;
volatile uint8_t discoveryPhase=0; // 0 advertising, 1 local subnet, 2 finished
void addDiscovered(const String& type,const String& name,const String& ip,uint16_t port,const String& serial=""){
  if(ip.isEmpty()||ip==WiFi.localIP().toString()||port==0)return;
  for(int i=0;i<discoveryCount;i++)if(discoveryResults[i].type==type&&discoveryResults[i].ip==ip)return;
  if(discoveryCount>=12)return;
  PrinterCandidate& p=discoveryResults[discoveryCount++];p.type=type;p.name=name.length()?name:type;p.ip=ip;p.port=port;p.serial=serial;
}
String discoveryHeader(const String& message,const char* header){
  String lower=message;lower.toLowerCase();String key="\n"+String(header);int pos=lower.indexOf(key);
  if(pos<0)return "";
  pos+=key.length();
  int end=message.indexOf('\n',pos);
  String value=message.substring(pos,end<0?message.length():end);
  value.trim();return value;
}
bool isMoonraker(IPAddress ip,uint16_t port,String* discoveredName=nullptr){
  if(port==0||port==443)return false;
  WiFiClient client;HTTPClient request;request.setConnectTimeout(400);request.setTimeout(600);
  if(!request.begin(client,"http://"+ip.toString()+":"+String(port)+"/printer/info"))return false;
  int status=request.GET();bool yes=false;
  if(status==200&&request.getSize()<4096){JsonDocument doc;yes=!deserializeJson(doc,request.getString())&&!doc["result"]["state"].isNull();if(yes&&discoveredName)*discoveredName=doc["result"]["hostname"].as<String>();}
  request.end();return yes;
}
void printerDiscoveryTask(void*){
  // Runs only on demand. The HTTP server remains responsive while network discovery waits.
  if(WiFi.status()==WL_CONNECTED){
    WiFiUDP udp;
    if(udp.begin(0)){
      const char* search="M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:2021\r\nMAN: \"ssdp:discover\"\r\nMX: 1\r\nST: urn:bambulab-com:device:3dprinter:1\r\n\r\n";
      for(int n=0;n<2;n++){udp.beginPacket(IPAddress(239,255,255,250),2021);udp.write((const uint8_t*)search,strlen(search));udp.endPacket();vTaskDelay(pdMS_TO_TICKS(200));}
      uint32_t until=millis()+1800;
      while((int32_t)(until-millis())>0){
        int bytes=udp.parsePacket();if(bytes>0){char packet[768];int len=udp.read(packet,sizeof(packet)-1);if(len>0){packet[len]=0;String msg(packet);String lower=msg;lower.toLowerCase();if(lower.indexOf("urn:bambulab-com:device:3dprinter:1")>=0){String serial=discoveryHeader(msg,"usn:"),name=discoveryHeader(msg,"devname.bambu.com:");serial.toUpperCase();if(serial.length()>=6&&serial.length()<=40)addDiscovered("bambu",name,udp.remoteIP().toString(),8883,serial);}}}
        vTaskDelay(pdMS_TO_TICKS(35));
      }
      udp.stop();
    }
    int n=MDNS.queryService("octoprint","tcp");
    for(int i=0;i<n&&i<12;i++){IPAddress ip=MDNS.address(i);uint16_t port=MDNS.port(i);if(ip!=IPAddress()&&port!=443)addDiscovered("octoprint",MDNS.instanceName(i),ip.toString(),port);}
    n=MDNS.queryService("http","tcp");
    for(int i=0;i<n&&i<10&&discoveryCount<12;i++){
      IPAddress ip=MDNS.address(i);uint16_t port=MDNS.port(i);String name=MDNS.instanceName(i);
      if(ip==IPAddress()||ip==WiFi.localIP())continue;
      if(isMoonraker(ip,port))addDiscovered("moonraker",name,ip.toString(),port);
      else if(port!=7125&&isMoonraker(ip,7125))addDiscovered("moonraker",name,ip.toString(),7125);
    }
    // Creality K2 often does not advertise Moonraker via mDNS. Probe only
    // the ESP32's own /24, constrained further by the actual subnet mask.
    IPAddress own=WiFi.localIP(),mask=WiFi.subnetMask(),gateway=WiFi.gatewayIP();
    discoveryPhase=1;discoveryTotal=508;
    for(int pass=0;pass<2&&discoveryCount<12&&WiFi.status()==WL_CONNECTED;pass++){
      uint16_t port=pass==0?7125:80;
      
      for(int host=1;host<255&&discoveryCount<12&&WiFi.status()==WL_CONNECTED;host++){
        discoveryChecked++;
        IPAddress target(own[0],own[1],own[2],host);
        bool sameSubnet=true;for(int octet=0;octet<4;octet++)if((target[octet]&mask[octet])!=(own[octet]&mask[octet]))sameSubnet=false;
        if(!sameSubnet||target==own||target==gateway){vTaskDelay(1);continue;}
        WiFiClient probe;
        if(probe.connect(target,port,180)){
          probe.stop();String name;
          if(isMoonraker(target,port,&name))addDiscovered("moonraker",name.length()?name:"Creality / Moonraker",target.toString(),port);
        }
        probe.stop();vTaskDelay(1);
      }
    }
  }
  discoveryPhase=2;discoveryRunning=false;vTaskDelete(nullptr);
}

void jsonReply(int code, JsonDocument& d){String s;serializeJson(d,s);server.send(code,"application/json",s);}
void routes(){
  server.on("/",HTTP_GET,[](){server.sendHeader("Cache-Control","no-store");server.send_P(200,"text/html; charset=utf-8",PAGE);});
  server.on("/api/ota/capacity",HTTP_GET,[](){JsonDocument d;const esp_partition_t* slot=esp_ota_get_next_update_partition(nullptr);d["appSlotBytes"]=slot?slot->size:0;jsonReply(200,d);});
  server.on("/api/network/info",HTTP_GET,[](){
    JsonDocument d;bool online=WiFi.status()==WL_CONNECTED;
    JsonObject w=d["wifi"].to<JsonObject>();w["connected"]=online;w["ssid"]=online?WiFi.SSID():ssid;w["ip"]=online?WiFi.localIP().toString():"";w["gateway"]=online?WiFi.gatewayIP().toString():"";w["subnet"]=online?WiFi.subnetMask().toString():"";w["dns"]=online?WiFi.dnsIP().toString():"";w["rssi"]=online?WiFi.RSSI():0;w["channel"]=online?WiFi.channel():0;w["mac"]=WiFi.macAddress();
    JsonObject a=d["ap"].to<JsonObject>();a["active"]=apMode;a["ssid"]=apMode?AP_NAME:"";a["ip"]=apMode?WiFi.softAPIP().toString():"";a["clients"]=apMode?WiFi.softAPgetStationNum():0;
    JsonObject p=d["printer"].to<JsonObject>();p["connected"]=printerConnected;p["test"]=simEnabled;p["type"]=printerType;p["ip"]=printerIP;p["port"]=printerType=="bambu"?8883:printerPort;p["error"]=lastError;p["enabled"]=printerEnabled;p["state"]=printerState;p["progress"]=printerProgress;p["filename"]=printerFile;p["lastReadAgeSeconds"]=lastPrinterReadAt?((millis()-lastPrinterReadAt)/1000):-1;
    d["webHost"]=webHost;d["mdnsReady"]=online&&mdnsStarted&&mdnsHostActive==webHost&&!mdnsNeedsRestart;
    JsonArray saved=d["savedNetworks"].to<JsonArray>();
    for(int i=0;i<wifiProfileCount;i++){
      JsonObject item=saved.add<JsonObject>();item["index"]=i;item["ssid"]=wifiProfiles[i].name;
      item["active"]=i==activeWifi;item["connected"]=online&&WiFi.SSID()==wifiProfiles[i].name;
    }
    d["maxSavedNetworks"]=MAX_WIFI_PROFILES;
    jsonReply(200,d);
  });
  server.on("/api/health",HTTP_GET,[](){JsonDocument d;d["version"]="2.34";d["uptimeSeconds"]=millis()/1000;d["freeHeap"]=ESP.getFreeHeap();d["minFreeHeap"]=ESP.getMinFreeHeap();d["wifiConnected"]=WiFi.status()==WL_CONNECTED;d["rssi"]=WiFi.status()==WL_CONNECTED?WiFi.RSSI():0;d["apActive"]=apMode;d["apClients"]=apMode?WiFi.softAPgetStationNum():0;d["printerConnected"]=printerConnected;d["otaUpload"]=otaUploadInProgress;jsonReply(200,d);});
  server.on("/api/status",HTTP_GET,[](){JsonDocument d;d["version"]="2.34";d["updateAvailable"]=updateAvailable;d["lightingBackup"]=lightingBackupAvailable;d["printerEnabled"]=printerEnabled;d["connected"]=printerConnected;d["ap"]=apMode;d["state"]=normalState(printerState);d["progress"]=printerProgress;d["filename"]=printerFile;d["error"]=lastError;d["ssid"]=ssid;d["webHost"]=webHost;d["ip"]=printerIP;d["port"]=printerPort;d["printerType"]=printerType;d["moonIP"]=moonIP;d["bambuIP"]=bambuIP;d["octoIP"]=octoIP;d["prusaIP"]=prusaIP;d["moonPort"]=moonPort;d["octoPort"]=octoPort;d["prusaPort"]=prusaPort;d["hasOctoKey"]=!octoKey.isEmpty();d["hasPrusaKey"]=!prusaKey.isEmpty();d["bambuSerial"]=bambuSerial;d["hasBambuCode"]=!bambuAccessCode.isEmpty();d["interval"]=intervalMs;d["ledCount"]=activeLedCount;d["simLoop"]=simLoop;d["sim"]=simEnabled;d["simState"]=stateNames[simStatus];d["simProgress"]=simProgress;d["simRunning"]=simRun;addRows(d);jsonReply(200,d);});
  server.on("/api/printers/discovery",HTTP_POST,[](){
    if(WiFi.status()!=WL_CONNECTED){server.send(503,"text/plain","Wi-Fi not connected");return;}
    if(otaUploadInProgress||wifiScanActive){server.send(409,"text/plain","Network is busy");return;}
    if(!discoveryRunning){discoveryCount=0;discoveryChecked=0;discoveryTotal=0;discoveryPhase=0;discoveryRunning=true;if(xTaskCreate(printerDiscoveryTask,"printer-discovery",8192,nullptr,1,nullptr)!=pdPASS){discoveryRunning=false;server.send(503,"text/plain","Discovery unavailable");return;}}
    JsonDocument d;d["searching"]=true;jsonReply(200,d);
  });
  server.on("/api/printers/discovery",HTTP_GET,[](){JsonDocument d;d["searching"]=discoveryRunning;d["phase"]=discoveryPhase;d["checked"]=discoveryChecked;d["total"]=discoveryTotal;if(!discoveryRunning){JsonArray a=d["printers"].to<JsonArray>();for(int i=0;i<discoveryCount;i++){JsonObject p=a.add<JsonObject>();p["type"]=discoveryResults[i].type;p["name"]=discoveryResults[i].name;p["ip"]=discoveryResults[i].ip;p["port"]=discoveryResults[i].port;p["serial"]=discoveryResults[i].serial;}}jsonReply(200,d);});
  server.on("/api/wifi/scan",HTTP_GET,[](){
    JsonDocument d;
    if(!wifiScanActive){
      if(discoveryRunning){server.send(409,"text/plain","Printer discovery in progress");return;}
      WiFi.scanDelete();
      // Keep the STA and rescue AP alive during an asynchronous scan.
      int started=WiFi.scanNetworks(true,true);
      if(started==-2){
        d["error"]="scan_failed";jsonReply(503,d);return;
      }
      wifiScanActive=true;wifiScanStarted=millis();
    }
    int n=WiFi.scanComplete();
    if(n==-1){d["scanning"]=true;jsonReply(200,d);return;}
    wifiScanActive=false;
    if(n<0){WiFi.scanDelete();d["error"]="scan_failed";jsonReply(503,d);return;}
    JsonArray a=d["networks"].to<JsonArray>();
    for(int i=0;i<n && i<35;i++){
      String name=WiFi.SSID(i);if(name.isEmpty())continue;
      JsonObject net=a.add<JsonObject>();net["ssid"]=name;net["rssi"]=WiFi.RSSI(i);net["secure"]=WiFi.encryptionType(i)!=WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();d["scanning"]=false;jsonReply(200,d);
  });
  server.on("/api/wifi",HTTP_POST,[](){
    String newSSID=server.arg("ssid");newSSID.trim();
    String newHost=server.hasArg("host")?server.arg("host"):webHost;newHost.trim();newHost.toLowerCase();if(newHost.endsWith(".local"))newHost.remove(newHost.length()-6);
    if(newSSID.length()>32){server.send(400,"text/plain","Invalid SSID");return;}
    if(!validWebHost(newHost)){server.send(400,"text/plain","Invalid device name");return;}
    String newPassword=server.arg("pass");
    if(newPassword.length()>64){server.send(400,"text/plain","Wi-Fi password too long");return;}
    int index=-1;for(int i=0;i<wifiProfileCount;i++)if(wifiProfiles[i].name==newSSID){index=i;break;}
    if(newSSID.length()&&index<0&&wifiProfileCount>=MAX_WIFI_PROFILES){server.send(409,"text/plain","Saved Wi-Fi list is full");return;}
    bool hostChanged=webHost!=newHost,changed=false;
    if(newSSID.length()){
      if(index<0)index=wifiProfileCount++;
      String password=newPassword.length()?newPassword:(wifiProfiles[index].name==newSSID?wifiProfiles[index].password:"");
      changed=index!=activeWifi||wifiProfiles[index].name!=newSSID||wifiProfiles[index].password!=password||WiFi.status()!=WL_CONNECTED||WiFi.SSID()!=newSSID;
      wifiProfiles[index].name=newSSID;wifiProfiles[index].password=password;
      activeWifi=index;ssid=newSSID;wifiPassword=password;
    }
    webHost=newHost;
    saveWifiSettings();
    if(hostChanged){mdnsNeedsRestart=true;mdnsChangeAt=millis()+2500;lastMdnsAttempt=0;}
    if(changed){wifiChangePending=true;wifiChangeAt=millis()+1500;}
    server.send(200,"text/plain; charset=utf-8",changed?"Switching Wi-Fi":"Saved");
  });
  server.on("/api/wifi/select",HTTP_POST,[](){
    String value=server.arg("index");
    if(value.length()!=1||value[0]<'0'||value[0]>'7'||value.toInt()>=wifiProfileCount){server.send(400,"text/plain","Invalid saved network");return;}
    activeWifi=value.toInt();ssid=wifiProfiles[activeWifi].name;wifiPassword=wifiProfiles[activeWifi].password;
    saveWifiSettings();wifiChangePending=true;wifiChangeAt=millis()+1500;
    server.send(200,"text/plain; charset=utf-8","Switching Wi-Fi");
  });
  server.on("/api/wifi/delete",HTTP_POST,[](){
    String value=server.arg("index");
    if(value.length()!=1||value[0]<'0'||value[0]>'7'||value.toInt()>=wifiProfileCount){server.send(400,"text/plain","Invalid saved network");return;}
    int index=value.toInt();bool removedActive=index==activeWifi;
    for(int i=index;i+1<wifiProfileCount;i++)wifiProfiles[i]=wifiProfiles[i+1];
    wifiProfiles[--wifiProfileCount]=WifiProfile();
    if(wifiProfileCount){if(index<activeWifi)activeWifi--;else if(removedActive)activeWifi=min(index,(int)wifiProfileCount-1);ssid=wifiProfiles[activeWifi].name;wifiPassword=wifiProfiles[activeWifi].password;}
    else{activeWifi=0;ssid="";wifiPassword="";}
    saveWifiSettings();
    if(removedActive){wifiChangePending=true;wifiChangeAt=millis()+1500;}
    server.send(200,"text/plain; charset=utf-8","Removed");
  });
  server.on("/api/printer",HTTP_POST,[](){
    String newIP=server.arg("ip"),type=server.arg("printerType"),serial=server.arg("bambuSerial"),code=server.arg("bambuCode"),key=server.arg("apiKey");
    newIP.trim();serial.trim();code.trim();key.trim();serial.toUpperCase();
    int port=server.arg("port").toInt();IPAddress parsed;
    if((type!="bambu"&&type!="moonraker"&&type!="octoprint"&&type!="prusalink")||!parsed.fromString(newIP)||((type!="bambu")&&(port<1||port>65535))){server.send(400,"text/plain","Invalid printer IP or port");return;}
    if(type=="bambu"){
      if(serial.length()<6||serial.length()>40||code.length()>64||(code.isEmpty()&&(bambuAccessCode.isEmpty()||serial!=bambuSerial||newIP!=bambuIP))){server.send(400,"text/plain","Invalid Bambu serial or access code");return;}
      for(char c:serial)if(!isalnum((unsigned char)c)){server.send(400,"text/plain","Invalid Bambu serial or access code");return;}
    }
    if((type=="octoprint"&&key.isEmpty()&&(octoKey.isEmpty()||newIP!=octoIP))||(type=="prusalink"&&key.isEmpty()&&(prusaKey.isEmpty()||newIP!=prusaIP))||key.length()>128){server.send(400,"text/plain","API key required");return;}
    bool changed=!printerEnabled||printerType!=type||printerIP!=newIP||printerPort!=port||bambuSerial!=serial||!code.isEmpty()||!key.isEmpty();
    printerEnabled=true;
    printerType=type;printerIP=newIP;
    if(type=="bambu"){bambuIP=newIP;bambuSerial=serial;if(!code.isEmpty())bambuAccessCode=code;}
    else{printerPort=port;if(type=="moonraker"){moonIP=newIP;moonPort=port;}else if(type=="octoprint"){octoIP=newIP;octoPort=port;if(!key.isEmpty())octoKey=key;}else{prusaIP=newIP;prusaPort=port;if(!key.isEmpty())prusaKey=key;}}
    savePrinterSettings();
    if(changed){bambuMqtt.disconnect();printerConnected=false;printerState="standby";printerProgress=0;printerFile="";lastError="Waiting for first poll";lastPrinterReadAt=0;lastBambuAttempt=0;lastPoll=0;}
    server.send(200,"text/plain; charset=utf-8","Saved");
  });
  server.on("/api/printer/disconnect",HTTP_POST,[](){
    printerEnabled=false;bambuMqtt.disconnect();printerConnected=false;
    printerState="standby";printerProgress=0;printerFile="";lastPrinterReadAt=0;
    lastError="Printer disconnected";savePrinterSettings();updateLEDs();
    server.send(200,"text/plain; charset=utf-8","Disconnected");
  });
  server.on("/api/lighting",HTTP_POST,[](){
    int leds=strictCount(server.arg("ledCount"),5,MAX_LED_COUNT);
    if(leds<0){server.send(400,"text/plain","Invalid LED count");return;}
    String colors[6],extras[6][11];uint8_t colourCounts[6],br[6],styles[6];bool enabled[6],lightsOn[6];uint16_t periods[6];
    for(int i=0;i<6;i++){
      String id=i==0?"progress":String(stateNames[i]);
      colors[i]=normalizeColor(server.arg("c_"+id),"");int b=server.arg("b_"+id).toInt(),p=server.arg("pp_"+id).toInt(),style=server.arg("ps_"+id).toInt();
      if(colors[i].isEmpty()||b<0||b>100||p<0||p>5000||style<0||style>13||style==4){server.send(400,"text/plain","Invalid LED row: "+id);return;}
      int n=server.hasArg("n_"+id)?strictCount(server.arg("n_"+id),1,12):1;
      if(n<0){server.send(400,"text/plain","Invalid LED row: "+id);return;}
      colourCounts[i]=n;
      for(int j=0;j<11;j++){
        String field=String("c")+(j+1)+"_"+id;
        extras[i][j]=server.hasArg(field)?normalizeColor(server.arg(field),""):rowExtraColors[i][j];
        if(extras[i][j].isEmpty()){server.send(400,"text/plain","Invalid LED row: "+id);return;}
      }
      br[i]=b;periods[i]=p;styles[i]=style;enabled[i]=server.hasArg("pe_"+id);lightsOn[i]=server.hasArg("on_"+id);
    }
    activeLedCount=leds;
    for(int i=0;i<6;i++){rowColors[i]=colors[i];rowColorCount[i]=colourCounts[i];for(int j=0;j<11;j++)rowExtraColors[i][j]=extras[i][j];rowBrightness[i]=br[i];rowPeriod[i]=periods[i];rowStyle[i]=styles[i];rowPulse[i]=enabled[i];rowEnabled[i]=lightsOn[i];}
    saveLighting();updateLEDs();server.send(200,"text/plain","Saved");
  });
  server.on("/api/lighting/defaults",HTTP_POST,[](){
    activeLedCount=24;intervalMs=500;
    for(int i=0;i<6;i++){rowColors[i]=DEFAULT_ROW_COLORS[i];rowColorCount[i]=1;for(int j=0;j<11;j++)rowExtraColors[i][j]=DEFAULT_EXTRA_COLORS[j];rowBrightness[i]=65;rowPulse[i]=false;rowEnabled[i]=true;rowStyle[i]=0;rowPeriod[i]=1400;}
    saveLighting();updateLEDs();JsonDocument d;d["ok"]=true;d["ledCount"]=activeLedCount;d["interval"]=intervalMs;addRows(d);jsonReply(200,d);
  });
  server.on("/api/lighting/backup",HTTP_POST,[](){if(!saveLightingBackup()){server.send(500,"text/plain","LED backup failed");return;}JsonDocument d;d["ok"]=true;d["lightingBackup"]=true;jsonReply(200,d);});
  server.on("/api/lighting/restore",HTTP_POST,[](){if(!restoreLightingBackup()){server.send(404,"text/plain","No LED backup");return;}JsonDocument d;d["ok"]=true;d["lightingBackup"]=true;d["ledCount"]=activeLedCount;d["interval"]=intervalMs;addRows(d);jsonReply(200,d);});
  server.on("/api/simulation",HTTP_POST,[](){simEnabled=server.arg("enabled")=="1";simStatus=statusIndex(server.arg("status"));simProgress=constrain(server.arg("progress").toInt(),0,100);simLoop=server.arg("loop")=="1";if(server.hasArg("run")){simRun=simEnabled&&server.arg("run")=="1";lastSimulationStep=millis();}if(!simEnabled)simRun=false;updateLEDs();JsonDocument d;d["ok"]=true;jsonReply(200,d);});
  server.on("/api/update/available",HTTP_POST,[](){updateAvailable=server.arg("available")=="1";server.send(200,"text/plain","OK");});
  server.on("/api/update",HTTP_POST,[](){bool ok=!Update.hasError()&&updateError.isEmpty();server.send(ok?200:500,"text/plain",ok?"OK":("Update failed: "+updateError));delay(350);if(ok)ESP.restart();},[](){HTTPUpload& upload=server.upload();if(upload.status==UPLOAD_FILE_START){otaUploadInProgress=true;updateError="";if(!upload.filename.endsWith(".bin")){updateError="Expected .bin";return;}if(!Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH))updateError=Update.errorString();}else if(upload.status==UPLOAD_FILE_WRITE){if(updateError.isEmpty()&&Update.write(upload.buf,upload.currentSize)!=upload.currentSize)updateError=Update.errorString();}else if(upload.status==UPLOAD_FILE_END){if(updateError.isEmpty()&&!Update.end(true))updateError=Update.errorString();otaUploadInProgress=false;}else if(upload.status==UPLOAD_FILE_ABORTED){Update.abort();updateError="Aborted";otaUploadInProgress=false;}});
}
void setup(){pinMode(STATUS_LED_PIN,OUTPUT);digitalWrite(STATUS_LED_PIN,HIGH);Serial.begin(115200);delay(450);loadSettings();bool migrateName=webHost=="k2-led";if(migrateName)webHost="layerglow";if(migrateLegacyWifi||migrateName)saveWifiSettings();webHost.toLowerCase();if(!validWebHost(webHost))webHost="layerglow";{Preferences backup;if(backup.begin("k2ledbak",true)){lightingBackupAvailable=backup.getBool("valid",false);backup.end();}}if(LED_ENABLED){strip.updateLength(activeLedCount);strip.begin();strip.clear();strip.show();}bambuTLS.setInsecure();bambuTLS.setHandshakeTimeout(2);bambuMqtt.setCallback(onBambuReport);bambuMqtt.setKeepAlive(15);bambuMqtt.setSocketTimeout(2);startNetwork();routes();server.begin();Serial.println("LayerGlow v2.34 web server ready");}
void loop(){
  server.handleClient();
  uint32_t now=millis();
  maintainNetwork(now);
  maintainBambu(now);
  // Active LOW: update = two long flashes, pause, one short flash, pause.
  bool ledLit;
  if(WiFi.status()!=WL_CONNECTED){
    ledLit=(now%1200)<90;
  }else if(updateAvailable){
    uint32_t beat=now%2200;
    ledLit=(beat<350)||(beat>=530&&beat<880)||(beat>=1400&&beat<1490);
  }else{
    uint32_t beat=now%1200;
    ledLit=(beat<90)||(beat>=200&&beat<290);
  }
  digitalWrite(STATUS_LED_PIN,ledLit?LOW:HIGH);
  // GitHub TLS could block this single-core web server for seconds.
  // The browser checks GitHub and reports the update flag via /api/update/available.
  if(now-lastPoll>(printerConnected?2500UL:6000UL)){lastPoll=now;if(printerEnabled&&printerType!="bambu"){if(printerType=="moonraker")pollPrinter();else pollHttpPrinter();updateLEDs();}}
  static uint32_t lastRandomRender=0;
  if(now-lastRandomRender>=40){
    String activeState=simEnabled?String(stateNames[simStatus]):(!printerConnected?"standby":normalState(printerState));
    int idx=statusIndex(activeState);
    if(rowEnabled[idx]&&(rowStyle[idx]>=6||(rowPulse[idx]&&rowStyle[idx]>=2))){lastRandomRender=now;updateLEDs();}
  }
  if(simRun&&now-lastSimulationStep>=(uint32_t)intervalMs){
    lastSimulationStep=now;
    if(simProgress>=100){if(simLoop)simProgress=0;else simRun=false;}
    else {simProgress=min(100,simProgress+9);if(simProgress>=100&&!simLoop)simRun=false;}
    updateLEDs();
  }
  if(rowPulse[statusIndex(simEnabled?String(stateNames[simStatus]):(!printerConnected?String("standby"):normalState(printerState)))]&&now-lastPulseFrame>=40){lastPulseFrame=now;updateLEDs();}
  delay(2);
}

