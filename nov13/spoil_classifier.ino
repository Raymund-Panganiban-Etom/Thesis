#include <Wire.h>
#include <Adafruit_SHT31.h>
#include "SpoilageClassifier.h"

// ---- Pin definitions ----
#define I2C_SDA 21
#define I2C_SCL 22
#define ETH_RX_PIN 16   // ESP32 RX2 <- sensor TX
#define ETH_TX_PIN 17   // ESP32 TX2 -> sensor RX
#define ETH_BAUD   9600

HardwareSerial ethSerial(2);
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Eloquent::ML::Port::SpoilageClassifier classifier;

const char* CLASS_NAMES[] = { "NORMAL", "WARNING (Ripe)", "CRITICAL (Spoiled)" };
const int   NUM_CLASSES   = 3;

bool sht31Ready = false;
int consecutiveCount = 0;
int lastRawState = -1;
int confirmedState = 0;

// ---- Ethylene smoothing ----
const int ETH_AVG_N = 5;
float ethHistory[ETH_AVG_N] = {0};
int   ethIndex = 0;
int   ethCount = 0;
float ethLast = 0.0f;

void pushEthylene(float v) {
  ethHistory[ethIndex] = v;
  ethIndex = (ethIndex + 1) % ETH_AVG_N;
  if (ethCount < ETH_AVG_N) ethCount++;
}

float getSmoothedEthylene() {
  if (ethCount == 0) return ethLast;
  float s = 0;
  for (int i = 0; i < ethCount; i++) s += ethHistory[i];
  return s / ethCount;
}

// ---- YYS SM4-C2H4 Binary Frame Parser ----
// Returns true if a valid frame was parsed and a new value was stored.
bool pollEthyleneUART() {
  // Look for the 0xFF start byte
  while (ethSerial.available() >= 9) {
    if (ethSerial.peek() != 0xFF) {
      ethSerial.read(); // discard non-header bytes
      continue;
    }

    uint8_t frame[9];
    ethSerial.readBytes(frame, 9);

    // 1) Verify checksum
    uint8_t sum = 0;
    for (int i = 1; i <= 7; i++) sum += frame[i];
    uint8_t expectedChecksum = (uint8_t)((~sum) + 1);
    if (frame[8] != expectedChecksum) {
      continue; // invalid frame, try next
    }

    // 2) Only process concentration responses (0x86)
    if (frame[1] != 0x86) {
      continue;
    }

    // 3) Decode C2H4 concentration
    //    RawValue = (High Byte * 256) + Low Byte
    //    Resolution for C2H4 is 0.1 ppm
    uint16_t rawValue = ((uint16_t)frame[2] << 8) | frame[3];
    float ppm = rawValue * 0.1f;   // <-- C2H4 resolution: 0.1 ppm

    if (ppm < 0.0f) ppm = 0.0f;

    ethLast = ppm;
    pushEthylene(ppm);
    return true;
  }
  return false;
}

// ---- 1 Hz Scheduler ----
unsigned long lastPrintMs = 0;
const unsigned long PRINT_PERIOD_MS = 1000;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(I2C_SDA, I2C_SCL);
  sht31Ready = sht31.begin(0x44);

  // Start UART for YYS sensor
  ethSerial.begin(ETH_BAUD, SERIAL_8N1, ETH_RX_PIN, ETH_TX_PIN);
  ethSerial.setTimeout(50); // short timeout for readBytes

  Serial.println("ESP32 Banana Spoilage Classifier Ready (YYS SM4-C2H4 binary parser).");
}

void loop() {
  // 1) Continuously drain UART (non-blocking)
  pollEthyleneUART();

  // 2) 1-second scheduled task
  unsigned long now = millis();
  if (now - lastPrintMs < PRINT_PERIOD_MS) return;
  lastPrintMs = now;

  float eth = getSmoothedEthylene();

  if (!sht31Ready) sht31Ready = sht31.begin(0x44);

  float temp = NAN, hum = NAN;
  if (sht31Ready) {
    temp = sht31.readTemperature();
    hum  = sht31.readHumidity();
    if (isnan(temp) || isnan(hum)) sht31Ready = false;
  }

  if (isnan(temp) || isnan(hum)) {
    Serial.printf("SHT31: N/A | Ethylene(UART): %.3f ppm -> [waiting]\n", eth);
    return;
  }

  float features[3] = { temp, hum, eth };
  int rawPrediction = classifier.predict(features);

  if (rawPrediction < 0 || rawPrediction >= NUM_CLASSES) {
    Serial.printf("Temp: %.2f C | Hum: %.2f %% | Eth: %.3f ppm -> OUT-OF-RANGE(%d)\n",
                  temp, hum, eth, rawPrediction);
    return;
  }

  if (rawPrediction == lastRawState) {
    if (consecutiveCount < 100) consecutiveCount++;
    if (consecutiveCount >= 3) confirmedState = rawPrediction;
  } else {
    consecutiveCount = 1;
    lastRawState = rawPrediction;
  }

  Serial.printf("Temp: %.2f C | Hum: %.2f %% | Ethylene(UART): %.3f ppm -> %s\n",
                temp, hum, eth, CLASS_NAMES[confirmedState]);
}
