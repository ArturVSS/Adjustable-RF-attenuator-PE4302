//Copyright by VSS 2026
//Not for commercial use  

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <U8g2lib.h>
#include <Preferences.h>

// ============================================================
// ESP32 + SH1106 OLED + enkoder + PE4302 + WWW + NVS WiFi
//
// OLED:
// SDA = GPIO21
// SCL = GPIO22
//
// Enkoder:
// A = GPIO19
// B = GPIO18
//
// Przycisk resetu tłumienia:
// GPIO5
//
// PE4302:
// GPIO25 -> C0.5
// GPIO26 -> C1
// GPIO27 -> C2
// GPIO32 -> C4
// GPIO33 -> C8
// GPIO23 -> C16
//
// P/S -> GND
// LE  -> 3.3V
// ============================================================

Preferences preferences;

// -------------------------
// Domyślne dane WiFi
// -------------------------
String wifiSSID = "SSID";
String wifiPassword = "password";

const bool USE_STATIC_IP = false;

IPAddress local_IP(192, 168, 1, 222);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(1, 1, 1, 1);

// -------------------------
// Web server
// -------------------------
WebServer server(80);

// -------------------------
// OLED SH1106 I2C
// -------------------------
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// -------------------------
// GPIO OLED
// -------------------------
static const uint8_t PIN_I2C_SDA = 21;
static const uint8_t PIN_I2C_SCL = 22;

// -------------------------
// GPIO enkoder / przycisk
// -------------------------
static const uint8_t PIN_ENC_A = 19;
static const uint8_t PIN_ENC_B = 18;
static const uint8_t PIN_BUTTON = 5;

// -------------------------
// GPIO PE4302
// -------------------------
static const uint8_t PIN_C05 = 25;
static const uint8_t PIN_C1  = 26;
static const uint8_t PIN_C2  = 27;
static const uint8_t PIN_C4  = 32;
static const uint8_t PIN_C8  = 33;
static const uint8_t PIN_C16 = 23;

// -------------------------
// Tłumienie:
// 0  = 0.0 dB
// 63 = 31.5 dB
// -------------------------
volatile int attenuationSteps = 0;
int lastDrawnSteps = -1;

int lastClkState = HIGH;

bool buttonState = HIGH;
bool lastButtonState = HIGH;
unsigned long lastButtonDebounce = 0;

String lastIpString = "";
unsigned long lastIpCheckMs = 0;

// ============================================================
// WiFi / NVS
// ============================================================
void loadWifiFromNVS() {
  preferences.begin("wifi", true);
  wifiSSID = preferences.getString("ssid", "Xiaomi2G");
  wifiPassword = preferences.getString("pass", "dupablada");
  preferences.end();
}

void saveWifiToNVS(const String& ssid, const String& pass) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);
  preferences.end();
}

String getIpString() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }

  return "No IP";
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);

  if (USE_STATIC_IP) {
    WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  }

  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttempt < 20000) {
    delay(500);
  }
}

void reconnectWiFi() {
  WiFi.disconnect(true, true);
  delay(500);
  connectWiFi();
}

// ============================================================
// Tłumik PE4302
// ============================================================
String attenuationToString(int steps) {
  steps = constrain(steps, 0, 63);

  int whole = steps / 2;
  bool half = (steps % 2) != 0;

  if (half) {
    return String(whole) + ".5";
  }

  return String(whole) + ".0";
}

int parseAttenuationToSteps(String s) {
  s.trim();
  s.replace(",", ".");

  float v = s.toFloat();

  if (v < 0.0f) v = 0.0f;
  if (v > 31.5f) v = 31.5f;

  int steps = (int)(v * 2.0f + 0.5f);

  return constrain(steps, 0, 63);
}

void setPE4302(int steps) {
  steps = constrain(steps, 0, 63);

  digitalWrite(PIN_C05, (steps & 0x01) ? HIGH : LOW);
  digitalWrite(PIN_C1,  (steps & 0x02) ? HIGH : LOW);
  digitalWrite(PIN_C2,  (steps & 0x04) ? HIGH : LOW);
  digitalWrite(PIN_C4,  (steps & 0x08) ? HIGH : LOW);
  digitalWrite(PIN_C8,  (steps & 0x10) ? HIGH : LOW);
  digitalWrite(PIN_C16, (steps & 0x20) ? HIGH : LOW);
}

void applyAttenuation(int steps) {
  attenuationSteps = constrain(steps, 0, 63);
  setPE4302(attenuationSteps);
}

// ============================================================
// OLED
// ============================================================
void drawScreen(int steps) {
  String txt = attenuationToString(steps);
  String ip = getIpString();

  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_6x10_tf);
  int16_t titleX =
    (128 - u8g2.getStrWidth("Adjustable attenuator")) / 2;
  u8g2.drawStr(titleX, 10, "Adjustable attenuator");

  u8g2.drawFrame(0, 14, 128, 34);

  u8g2.setFont(u8g2_font_7x13B_tf);
  u8g2.drawStr(6, 35, "ATT");

  u8g2.setFont(u8g2_font_logisoso20_tf);
  int16_t valueWidth = u8g2.getStrWidth(txt.c_str());
  int16_t valueX = 92 - valueWidth;

  if (valueX < 30) {
    valueX = 30;
  }

  u8g2.drawStr(valueX, 41, txt.c_str());

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(100, 36, "dBm");

  u8g2.setFont(u8g2_font_5x8_tf);
  String ipLine = "IP: " + ip;

  int16_t ipX =
    (128 - u8g2.getStrWidth(ipLine.c_str())) / 2;

  if (ipX < 0) {
    ipX = 0;
  }

  u8g2.drawStr(ipX, 62, ipLine.c_str());
  u8g2.sendBuffer();
}

// ============================================================
// WWW
// ============================================================
String makeHtmlPage() {
  String html;
  html.reserve(25000);

  html += R"rawliteral(
<!DOCTYPE html>
<html lang="pl">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Adjustable Attenuator</title>

<style>
  html {
    background: #111;
  }

  body {
    font-family: Arial, sans-serif;
    background: #111;
    color: #eee;
    margin: 0;
    padding: 20px;
    text-align: center;
  }

  .wrap {
    max-width: 1300px;
    margin: 0 auto;
  }

  .box {
    background: #1b1b1b;
    border: 1px solid #3c3c3c;
    border-radius: 14px;
    padding: 20px;
    margin-bottom: 16px;
  }

  .title {
    font-size: 30px;
    color: #fff;
    margin-bottom: 18px;
    font-weight: bold;
  }

  .valueRow {
    display: grid;
    grid-template-columns: 1fr auto 1fr;
    align-items: end;
    color: #66ff33;
  }

  .attLabel {
    font-size: 56px;
    font-weight: bold;
    line-height: 1;
    color: #66ff33;
    justify-self: start;
    text-align: left;
  }

  .centerWrap {
    display: flex;
    align-items: end;
    justify-content: center;
  }

  .mainValue {
    font-size: 224px;
    line-height: 1;
    font-weight: bold;
    color: #66ff33;
  }

  .unit {
    font-size: 56px;
    line-height: 1;
    font-weight: bold;
    color: #66ff33;
    justify-self: end;
    text-align: right;
  }

  .ctrlRow {
    display: flex;
    flex-wrap: wrap;
    gap: 12px;
    align-items: center;
    justify-content: center;
  }

  .ctrlInput {
    font-size: 28px;
    padding: 12px 14px;
    width: 180px;
    text-align: center;
    border-radius: 10px;
    border: 1px solid #666;
    background: #0f0f0f;
    color: #66ff33;
  }

  .btn {
    font-size: 22px;
    font-weight: bold;
    padding: 12px 20px;
    border-radius: 10px;
    border: 1px solid #4a4a4a;
    background: #1f1f1f;
    color: #66ff33;
    cursor: pointer;
    min-width: 90px;
  }

  .btn:hover {
    background: #2a2a2a;
  }

  .hint {
    font-size: 15px;
    color: #8f8f8f;
    margin-top: 10px;
  }

  .presetGrid {
    display: grid;
    grid-template-columns: repeat(16, 1fr);
    gap: 6px;
    width: 100%;
    margin: 18px auto 0;
  }

  .presetBtn {
    font-size: 16px;
    font-weight: bold;
    min-height: 40px;
    padding: 8px 2px;
    border-radius: 7px;
    border: 1px solid #4a4a4a;
    background: #1f1f1f;
    color: #66ff33;
    cursor: pointer;
  }

  .presetBtn:hover {
    background: #2a2a2a;
  }

  .netWrap {
    display: flex;
    flex-direction: column;
    gap: 18px;
  }

  .netLine {
    display: flex;
    flex-wrap: wrap;
    gap: 28px;
    justify-content: center;
    align-items: center;
    font-size: 20px;
    color: #ccc;
  }

  .wifiTitle {
    font-size: 22px;
    color: #fff;
    font-weight: bold;
    margin-top: 4px;
  }

  .wifiForm {
    display: grid;
    grid-template-columns: 1fr 1fr auto;
    gap: 12px;
    align-items: center;
  }

  .wifiInput {
    font-size: 20px;
    padding: 12px 14px;
    border-radius: 10px;
    border: 1px solid #666;
    background: #0f0f0f;
    color: #fff;
    min-width: 0;
  }

  .copy {
    font-size: 18px;
    color: #aaa;
  }

  @media (max-width: 1100px) {
    .presetGrid {
      grid-template-columns: repeat(8, 1fr);
    }
  }

  @media (max-width: 820px) {
    .wifiForm {
      grid-template-columns: 1fr;
    }

    .netLine {
      gap: 12px;
      flex-direction: column;
    }

    .mainValue {
      font-size: 82px;
    }

    .attLabel,
    .unit {
      font-size: 40px;
    }

    .presetGrid {
      grid-template-columns: repeat(4, 1fr);
    }
  }
</style>

<script>
  async function refreshData() {
    try {
      const r = await fetch('/data');
      const d = await r.json();

      document.getElementById('val').textContent = d.att;
      document.getElementById('ip').textContent = d.ip;
      document.getElementById('mode').textContent = d.mode;
      document.getElementById('ssid').textContent = d.ssid;
    } catch (e) {
    }
  }

  async function setManual() {
    const v = document.getElementById('manualValue').value;
    await fetch('/set?value=' + encodeURIComponent(v));
    refreshData();
  }

  async function setPreset(value) {
    document.getElementById('manualValue').value = value.toFixed(1);
    await fetch('/set?value=' + encodeURIComponent(value.toFixed(1)));
    refreshData();
  }

  async function changeStep(delta) {
    await fetch('/step?delta=' + delta);
    refreshData();
  }

  async function setZero() {
    await fetch('/zero');
    refreshData();
  }

  async function saveWifi() {
    const ssid = document.getElementById('cfgssid').value;
    const pass = document.getElementById('cfgpass').value;

    const r = await fetch(
      '/savewifi?ssid=' + encodeURIComponent(ssid) +
      '&pass=' + encodeURIComponent(pass)
    );

    document.getElementById('saveinfo').textContent = await r.text();
    setTimeout(refreshData, 2000);
  }

  setInterval(refreshData, 1000);
  window.onload = refreshData;
</script>
</head>

<body>
<div class="wrap">

  <div class="box">
    <div class="title">Adjustable attenuator</div>

    <div class="valueRow">
      <div class="attLabel">ATT</div>

      <div class="centerWrap">
        <div id="val" class="mainValue">0.0</div>
      </div>

      <div class="unit">dBm</div>
    </div>
  </div>

  <div class="box">
    <div class="ctrlRow">
      <input
        id="manualValue"
        class="ctrlInput"
        type="text"
        inputmode="decimal"
        placeholder="np. 12.5"
      >

      <button class="btn" onclick="setManual()">ENTR</button>
      <button class="btn" onclick="changeStep(1)">+</button>
      <button class="btn" onclick="changeStep(-1)">-</button>
      <button class="btn" onclick="setZero()">ZERO</button>
    </div>

    <div class="hint">
      Zakres: 0.0 do 31.5 dB, krok zdalny: 0.5 dB
    </div>

    <div class="presetGrid">
)rawliteral";

  // 0.0, 0.5, 1.0 ... 31.5 dB
  for (int i = 0; i <= 63; i++) {
    float value = i * 0.5f;

    html += "<button class=\"presetBtn\" onclick=\"setPreset(";
    html += String(value, 1);
    html += ")\">";
    html += String(value, 1);
    html += "</button>";
  }

  html += R"rawliteral(
    </div>
  </div>

  <div class="box">
    <div class="netWrap">
      <div class="netLine">
        <div>SSID: <span id="ssid">)rawliteral";

  html += wifiSSID;

  html += R"rawliteral(</span></div>

        <div>
          Tryb IP:
          <span id="mode">)rawliteral";

  html += String(USE_STATIC_IP ? "STATIC" : "DHCP");

  html += R"rawliteral(</span>
        </div>

        <div>
          Adres IP:
          <span id="ip">)rawliteral";

  html += getIpString();

  html += R"rawliteral(</span>
        </div>
      </div>

      <div class="wifiTitle">Zmiana sieci WiFi</div>

      <div class="wifiForm">
        <input
          id="cfgssid"
          class="wifiInput"
          type="text"
          placeholder="Nowe SSID"
          value=")rawliteral";

  html += wifiSSID;

  html += R"rawliteral("
        >

        <input
          id="cfgpass"
          class="wifiInput"
          type="password"
          placeholder="Nowe hasło WiFi"
        >

        <button class="btn" onclick="saveWifi()">ZAPISZ</button>
      </div>

      <div id="saveinfo" class="hint">
        Dane zostaną zapisane w pamięci NVS i zostaną użyte
        po ponownym uruchomieniu.
      </div>
    </div>
  </div>

  <div class="box">
    <div class="copy">COPYRIGHT by VSS2026</div>
  </div>

</div>
</body>
</html>
)rawliteral";

  return html;
}

String makeJsonData() {
  String json = "{";

  json += "\"att\":\"" + attenuationToString(attenuationSteps) + "\",";
  json += "\"ip\":\"" + getIpString() + "\",";
  json += "\"mode\":\"" + String(USE_STATIC_IP ? "STATIC" : "DHCP") + "\",";
  json += "\"ssid\":\"" + wifiSSID + "\"";

  json += "}";

  return json;
}

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", makeHtmlPage());
}

void handleData() {
  server.send(200, "application/json; charset=utf-8", makeJsonData());
}

void handleSet() {
  if (server.hasArg("value")) {
    int steps = parseAttenuationToSteps(server.arg("value"));
    applyAttenuation(steps);
  }

  server.send(
    200,
    "text/plain; charset=utf-8",
    attenuationToString(attenuationSteps)
  );
}

void handleStep() {
  int delta = 0;

  if (server.hasArg("delta")) {
    delta = server.arg("delta").toInt();
  }

  if (delta > 0) {
    applyAttenuation(attenuationSteps + 1);
  } else if (delta < 0) {
    applyAttenuation(attenuationSteps - 1);
  }

  server.send(
    200,
    "text/plain; charset=utf-8",
    attenuationToString(attenuationSteps)
  );
}

void handleZero() {
  applyAttenuation(0);

  server.send(
    200,
    "text/plain; charset=utf-8",
    attenuationToString(attenuationSteps)
  );
}

void handleSaveWifi() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    String newSSID = server.arg("ssid");
    String newPass = server.arg("pass");

    newSSID.trim();
    newPass.trim();

    if (newSSID.length() > 0) {
      wifiSSID = newSSID;
      wifiPassword = newPass;

      saveWifiToNVS(wifiSSID, wifiPassword);

      server.send(
        200,
        "text/plain; charset=utf-8",
        "Zapisano. Trwa ponowne laczenie WiFi..."
      );

      reconnectWiFi();
      return;
    }
  }

  server.send(400, "text/plain; charset=utf-8", "Blad danych WiFi");
}

void startWebServer() {
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/set", handleSet);
  server.on("/step", handleStep);
  server.on("/zero", handleZero);
  server.on("/savewifi", handleSaveWifi);

  server.begin();
}

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  u8g2.begin();

  pinMode(PIN_C05, OUTPUT);
  pinMode(PIN_C1, OUTPUT);
  pinMode(PIN_C2, OUTPUT);
  pinMode(PIN_C4, OUTPUT);
  pinMode(PIN_C8, OUTPUT);
  pinMode(PIN_C16, OUTPUT);

  applyAttenuation(attenuationSteps);

  lastClkState = digitalRead(PIN_ENC_A);

  loadWifiFromNVS();
  connectWiFi();

  lastIpString = getIpString();

  drawScreen(attenuationSteps);
  lastDrawnSteps = attenuationSteps;

  startWebServer();
}

// ============================================================
// Loop
// ============================================================
void loop() {
  server.handleClient();

  bool oledNeedsRefresh = false;

  int clkState = digitalRead(PIN_ENC_A);

  if (clkState != lastClkState && clkState == LOW) {
    if (digitalRead(PIN_ENC_B) != clkState) {
      applyAttenuation(attenuationSteps + 1);
    } else {
      applyAttenuation(attenuationSteps - 1);
    }
  }

  lastClkState = clkState;

  bool reading = digitalRead(PIN_BUTTON);

  if (reading != lastButtonState) {
    lastButtonDebounce = millis();
  }

  if ((millis() - lastButtonDebounce) > 30) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        applyAttenuation(0);
      }
    }
  }

  lastButtonState = reading;

  if (lastDrawnSteps != attenuationSteps) {
    oledNeedsRefresh = true;
    lastDrawnSteps = attenuationSteps;
  }

  if (millis() - lastIpCheckMs > 1000) {
    lastIpCheckMs = millis();

    String currentIp = getIpString();

    if (currentIp != lastIpString) {
      lastIpString = currentIp;
      oledNeedsRefresh = true;
    }
  }

  if (oledNeedsRefresh) {
    drawScreen(attenuationSteps);
  }

  delay(2);
}
