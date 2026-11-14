// ============================================================
// FORWARD DECLARATIONS
// ============================================================
struct SensorData;
struct CropLimits;
struct SoftI2C;

#include <WiFi.h>
#include <WebServer.h>
#include <SD_MMC.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>

// ==========================================
// SD CARD PINS
// ==========================================
#define SD_MMC_CMD  38
#define SD_MMC_CLK  39
#define SD_MMC_D0   40

// ==========================================
// FANS (LEDC PWM)
// ==========================================
// PWM Controller 1 (Zone 1)
#define FAN1_PIN  6    // Banana Intake
#define FAN2_PIN  7    // Banana Exhaust

// PWM Controller 2 (Zone 2)
#define FAN3_PIN  4    // Zone 2 Intake 1
#define FAN4_PIN  5    // Zone 2 Exhaust 1

// PWM Controller 3 (Zone 2)
#define FAN5_PIN  15   // Zone 2 Intake 2

// PWM Controller 4 (Zone 2)
#define FAN6_PIN  16   // Zone 2 Exhaust 2

#define FAN_PWM_FREQ  25000
#define FAN_PWM_RES   8      // 0..255

// ==========================================
// SENSORS
// ==========================================
// I2C Buses for SHT31
#define BANANA_SDA  14
#define BANANA_SCL  47
#define SITAW_SDA   41
#define SITAW_SCL   42
#define POMELO_SDA   8
#define POMELO_SCL   9
#define KALAMANSI_SDA 10
#define KALAMANSI_SCL 11
#define CORN_SDA     12
#define CORN_SCL     13

// SM4-C2H4 UART Pins
#define RX1_PIN  17
#define TX1_PIN  18
#define RX2_PIN  43
#define TX2_PIN  44

// ==========================================
// WI-FI — static IP
// ==========================================
const char *WIFI_SSID = "GFiber_608C6";
const char *WIFI_PASS = "E3ABA760";

#define USE_STATIC_IP true
IPAddress local_IP(192, 168, 254, 190);
IPAddress gateway (192, 168, 254, 254);
IPAddress subnet  (255, 255, 255, 0);

WebServer server(80);

// ==========================================
// SENSOR OBJECTS & SOFTWARE I2C
// ==========================================
TwoWire I2C_A = TwoWire(0);
TwoWire I2C_B = TwoWire(1);
Adafruit_SHT31 shtBanana = Adafruit_SHT31(&I2C_A);
Adafruit_SHT31 shtSitaw  = Adafruit_SHT31(&I2C_B);

struct SoftI2C {
  uint8_t sda, scl;
  void begin(uint8_t s, uint8_t c) {
    sda = s; scl = c;
    pinMode(sda, INPUT_PULLUP); pinMode(scl, INPUT_PULLUP);
  }
  inline void dly()        { delayMicroseconds(5); }
  inline void sdaLow()     { pinMode(sda, OUTPUT);      digitalWrite(sda, LOW); }
  inline void sdaRelease() { pinMode(sda, INPUT_PULLUP); }
  inline void sclLow()     { pinMode(scl, OUTPUT);      digitalWrite(scl, LOW); }
  inline void sclRelease() { pinMode(scl, INPUT_PULLUP); }
  inline bool sdaRead()    { return digitalRead(sda); }

  void start() { sdaRelease(); dly(); sclRelease(); dly(); sdaLow(); dly(); sclLow(); dly(); }
  void stop()  { sdaLow(); dly(); sclRelease(); dly(); sdaRelease(); dly(); }
  bool writeByte(uint8_t b) {
    for (int i = 7; i >= 0; i--) {
      if ((b >> i) & 1) sdaRelease(); else sdaLow();
      dly(); sclRelease(); dly(); sclLow(); dly();
    }
    sdaRelease(); dly(); sclRelease(); dly();
    bool ack = !sdaRead(); sclLow(); dly();
    return ack;
  }
  uint8_t readByte(bool sendAck) {
    uint8_t b = 0; sdaRelease();
    for (int i = 7; i >= 0; i--) {
      dly(); sclRelease(); dly();
      if (sdaRead()) b |= (1 << i);
      sclLow(); dly();
    }
    if (sendAck) sdaLow(); else sdaRelease();
    dly(); sclRelease(); dly(); sclLow(); dly(); sdaRelease();
    return b;
  }
};

SoftI2C softPomelo, softKalamansi, softCorn;

HardwareSerial SM4_Serial1(1);
HardwareSerial SM4_Serial2(2);

// ==========================================
// DATA STRUCTURES & CACHE
// ==========================================
struct SensorData { float t; float h; bool ok; };
SensorData sBanana = {NAN, NAN, false}, sSitaw = {NAN, NAN, false};
SensorData sPomelo = {NAN, NAN, false}, sKalamansi = {NAN, NAN, false}, sCorn = {NAN, NAN, false};

float eth1 = NAN, eth2 = NAN;
unsigned long lastRxTime1 = 0, lastRxTime2 = 0;

// Fan commands (0-255)
int fanPWM[6] = {0, 0, 0, 0, 0, 0};
unsigned long lastCommandMs = 0;
const unsigned long STALE_MS = 15000;

// ==========================================
// HELPERS
// ==========================================
void scanI2C(TwoWire &wire, const char* name) {
  Serial.printf("[I2C] Scanning %s...\n", name);
  byte count = 0;
  for (byte i = 1; i < 127; i++) {
    wire.beginTransmission(i);
    if (wire.endTransmission() == 0) {
      Serial.printf("  -> Found device at 0x%02X\n", i);
      count++;
    }
  }
  if (count == 0) Serial.println("  -> No devices found. Check wiring!");
}

void readHW_SHT(Adafruit_SHT31 &dev, SensorData &out) {
  float t = dev.readTemperature(); float h = dev.readHumidity();
  if (!isnan(t) && !isnan(h)) { out.t = t; out.h = h; out.ok = true; }
  else { out.t = NAN; out.h = NAN; out.ok = false; }
}

uint8_t sht31Crc(const uint8_t *data, int len) {
  uint8_t crc = 0xFF;
  for (int i = 0; i < len; i++) {
    crc ^= data[i];
    for (int b = 0; b < 8; b++) crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
  }
  return crc;
}

bool sht31ReadSoft(SoftI2C &i2c, SensorData &out) {
  uint8_t addrW = 0x44 << 1, addrR = addrW | 1;
  i2c.start();
  if (!i2c.writeByte(addrW) || !i2c.writeByte(0x24) || !i2c.writeByte(0x00)) { i2c.stop(); return false; }
  i2c.stop(); delay(20);
  i2c.start();
  if (!i2c.writeByte(addrR)) { i2c.stop(); return false; }
  uint8_t d[6];
  for (int i = 0; i < 5; i++) d[i] = i2c.readByte(true);
  d[5] = i2c.readByte(false);
  i2c.stop();
  
  if (sht31Crc(d, 2) != d[2] || sht31Crc(d + 3, 2) != d[5]) { out.ok = false; return false; }
  uint16_t tRaw = ((uint16_t)d[0] << 8) | d[1];
  uint16_t hRaw = ((uint16_t)d[3] << 8) | d[4];
  out.t = -45.0f + 175.0f * (float)tRaw / 65535.0f;
  out.h = 100.0f * (float)hRaw / 65535.0f;
  out.ok = true;
  return true;
}

float readSM4(HardwareSerial &port, int sensorNum) {
  if (port.available() > 0) {
    uint8_t buf[50];
    int len = 0;
    while (port.available() > 0 && len < 50) buf[len++] = port.read();

    if (sensorNum == 1) lastRxTime1 = millis();
    if (sensorNum == 2) lastRxTime2 = millis();

    for (int i = 0; i <= len - 9; i++) {
      if (buf[i] == 0xFF && buf[i+1] == 0x86) {
        uint8_t s = buf[i+1]+buf[i+2]+buf[i+3]+buf[i+4]+buf[i+5]+buf[i+6]+buf[i+7];
        uint8_t chk = (~s) + 1;
        if (chk == buf[i+8]) {
          return ((buf[i+2] << 8) | buf[i+3]) * 0.1f; // Resolution is 0.1ppm
        }
      }
    }
  }
  return -1.0f;
}

void sampleAll() {
  static uint32_t last = 0;
  if (last != 0 && millis() - last < 2000) return; 
  last = millis();

  readHW_SHT(shtBanana, sBanana);
  readHW_SHT(shtSitaw, sSitaw);
  sht31ReadSoft(softPomelo, sPomelo);
  sht31ReadSoft(softKalamansi, sKalamansi);
  sht31ReadSoft(softCorn, sCorn);
}

// ==========================================
// CROP LIMITS & EVAL
// ==========================================
struct CropLimits { float tWarn, tHigh, hLo, hHi, hHighLo, hHighHi, eWarn, eHigh; };
const CropLimits LIM_BANANA    = { 14.5, 16.0, 90, 95, 85, 95, 0.10, 1.00 };
const CropLimits LIM_SITAW     = {  7.5,  9.0, 90, 95, 85, 95, 0.05, 0.10 };
const CropLimits LIM_POMELO    = { 12.0, 14.0, 88, 93, 85, 95, 0.05, 0.50 };
const CropLimits LIM_KALAMANSI = { 12.0, 14.0, 88, 93, 85, 95, 0.05, 0.50 };
const CropLimits LIM_CORN      = {  2.0,  5.0, 95, 98, 90, 98, 0.10, 0.50 };

int evalCrop(float t, float h, float e, const CropLimits &L) {
  if (isnan(t) || isnan(h) || isnan(e)) return 0; // Fail-safe to normal
  if (t > L.tHigh || h < L.hHighLo || h > L.hHighHi || e > L.eHigh) return 2;
  if (t > L.tWarn || h < L.hLo    || h > L.hHi     || e > L.eWarn) return 1;
  return 0;
}

// ==========================================
// NETWORK & FANS
// ==========================================
void connectWiFi() {
  WiFi.mode(WIFI_STA); WiFi.setSleep(false);
  if (USE_STATIC_IP) WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 25000) delay(500);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WIFI] Connected: %s\n", WiFi.localIP().toString().c_str());
  }
}

void applyFans() {
  bool stale = (lastCommandMs == 0) || (millis() - lastCommandMs > STALE_MS);
  
  int p1 = stale ? 0 : constrain(fanPWM[0], 0, 255); // PWM1
  int p2 = stale ? 0 : constrain(fanPWM[1], 0, 255); // PWM2
  int p3 = stale ? 0 : constrain(fanPWM[2], 0, 255); // PWM3
  int p4 = stale ? 0 : constrain(fanPWM[3], 0, 255); // PWM4

  ledcWrite(FAN1_PIN, p1);
  ledcWrite(FAN2_PIN, p1); 
  
  ledcWrite(FAN3_PIN, p2);
  ledcWrite(FAN4_PIN, p2); 
  
  ledcWrite(FAN5_PIN, p3); 
  ledcWrite(FAN6_PIN, p4); 
}

void addCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Cache-Control", "no-store");
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(400);

  pinMode(RX1_PIN, INPUT_PULLUP);
  pinMode(RX2_PIN, INPUT_PULLUP);

  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5);
  connectWiFi();

  ledcAttach(FAN1_PIN, FAN_PWM_FREQ, FAN_PWM_RES);
  ledcAttach(FAN2_PIN, FAN_PWM_FREQ, FAN_PWM_RES);
  ledcAttach(FAN3_PIN, FAN_PWM_FREQ, FAN_PWM_RES);
  ledcAttach(FAN4_PIN, FAN_PWM_FREQ, FAN_PWM_RES);
  ledcAttach(FAN5_PIN, FAN_PWM_FREQ, FAN_PWM_RES);
  ledcAttach(FAN6_PIN, FAN_PWM_FREQ, FAN_PWM_RES);
  applyFans();

  I2C_A.begin(BANANA_SDA, BANANA_SCL);
  I2C_B.begin(SITAW_SDA, SITAW_SCL);
  
  // Initialize SHT31
  shtBanana.begin(0x44);
  shtSitaw.begin(0x44);

  // SCAN I2C TO CHECK FOR HARDWARE ISSUES
  scanI2C(I2C_A, "Banana (I2C_A)");
  scanI2C(I2C_B, "Sitaw (I2C_B)");

  softPomelo.begin(POMELO_SDA, POMELO_SCL);
  softKalamansi.begin(KALAMANSI_SDA, KALAMANSI_SCL);
  softCorn.begin(CORN_SDA, CORN_SCL);

  SM4_Serial1.begin(9600, SERIAL_8N1, RX1_PIN, TX1_PIN);
  SM4_Serial2.begin(9600, SERIAL_8N1, RX2_PIN, TX2_PIN);

  sampleAll();

  // ---------- ROUTES ----------
  server.on("/api/fans", HTTP_OPTIONS, []() { addCors(); server.send(204); });

  server.on("/api/fans", HTTP_POST, []() {
    addCors();
    if (!server.hasArg("plain")) { server.send(400, "text/plain", "bad request"); return; }
    
    String body = server.arg("plain");
    int vals[6] = {0}, idx = 0;
    
    for (unsigned int i = 0; i < body.length() && idx < 6; i++) {
      if (body[i] >= '0' && body[i] <= '9') vals[idx] = vals[idx] * 10 + (body[i] - '0');
      else if (body[i] == ',') idx++;
    }
    
    fanPWM[0] = constrain(vals[0], 0, 255); // PWM1 (Fans 1 & 2)
    fanPWM[1] = constrain(vals[2], 0, 255); // PWM2 (Fans 3 & 4)
    fanPWM[2] = constrain(vals[4], 0, 255); // PWM3 (Fan 5)
    fanPWM[3] = constrain(vals[5], 0, 255); // PWM4 (Fan 6)

    lastCommandMs = millis();
    applyFans();
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/data", []() {
    addCors();

    float v1 = readSM4(SM4_Serial1, 1);
    if (v1 >= 0) eth1 = v1;
    float v2 = readSM4(SM4_Serial2, 2);
    if (v2 >= 0) eth2 = v2;

    float bT = sBanana.ok ? sBanana.t : NAN, bH = sBanana.ok ? sBanana.h : NAN;
    float bE = isnan(eth1) ? NAN : eth1;
    float mE = isnan(eth2) ? NAN : eth2;

    int bS = evalCrop(bT, bH, bE, LIM_BANANA);

    int sSitaw_state     = evalCrop(sSitaw.t, sSitaw.h, mE, LIM_SITAW);
    int sPomelo_state    = evalCrop(sPomelo.t, sPomelo.h, mE, LIM_POMELO);
    int sKalamansi_state = evalCrop(sKalamansi.t, sKalamansi.h, mE, LIM_KALAMANSI);
    int sCorn_state      = evalCrop(sCorn.t, sCorn.h, mE, LIM_CORN);

    int avgS = round((sSitaw_state + sPomelo_state + sKalamansi_state + sCorn_state) / 4.0);

    const int PWM_TABLE[3] = {51, 128, 255};
    int pwm1 = PWM_TABLE[bS];
    int pwm2 = PWM_TABLE[avgS];
    int pwm3 = PWM_TABLE[avgS];
    int pwm4 = PWM_TABLE[avgS];

    if (millis() - lastCommandMs > STALE_MS) {
      pwm1 = pwm2 = pwm3 = pwm4 = 0;
    } else {
      fanPWM[0] = pwm1;
      fanPWM[1] = pwm2;
      fanPWM[2] = pwm3;
      fanPWM[3] = pwm4;
      applyFans();
    }

    auto fmt = [](float v, int d) { return isnan(v) ? String("null") : String(v, d); };

    String json = "{";
    json += "\"b\":{\"t\":" + fmt(bT, 2) + ",\"h\":" + fmt(bH, 2) + ",\"e\":" + fmt(bE, 3) + ",\"s\":" + String(bS) + ",\"pwm\":" + String(pwm1) + "},";
    json += "\"m\":{\"e\":" + fmt(mE, 3) + ",\"s\":" + String(avgS) + ",\"pwm\":" + String(pwm2) + "},";
    
    json += "\"c\":[";
    json += "{\"id\":\"sitaw\",\"t\":" + fmt(sSitaw.t, 2) + ",\"h\":" + fmt(sSitaw.h, 2) + ",\"s\":" + String(sSitaw_state) + "},";
    json += "{\"id\":\"pomelo\",\"t\":" + fmt(sPomelo.t, 2) + ",\"h\":" + fmt(sPomelo.h, 2) + ",\"s\":" + String(sPomelo_state) + "},";
    json += "{\"id\":\"kalamansi\",\"t\":" + fmt(sKalamansi.t, 2) + ",\"h\":" + fmt(sKalamansi.h, 2) + ",\"s\":" + String(sKalamansi_state) + "},";
    json += "{\"id\":\"corn\",\"t\":" + fmt(sCorn.t, 2) + ",\"h\":" + fmt(sCorn.h, 2) + ",\"s\":" + String(sCorn_state) + "}";
    json += "],";
    
    json += "\"f\":[" + String(pwm1) + "," + String(pwm1) + "," + String(pwm2) + "," + String(pwm2) + "," + String(pwm3) + "," + String(pwm4) + "]";
    json += "}";

    server.send(200, "application/json", json);
  });

  server.serveStatic("/", SD_MMC, "/");
  server.begin();
}

void loop() {
  server.handleClient();
  sampleAll();
  static unsigned long lastApply = 0;
  if (millis() - lastApply > 1000) { lastApply = millis(); applyFans(); }
}
