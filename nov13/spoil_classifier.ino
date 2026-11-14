#include <Wire.h>
#include <Adafruit_SHT31.h>
#include "SpoilageClassifier.h"

// Standard ESP32 Pin Definitions (matching your training setup)
#define I2C_SDA 21
#define I2C_SCL 22
#define ETH_V0_PIN 34

// Must match the exact calibration constants used during CSV logging
const float ZERO_OFFSET_MV = 400.0;
const float MAX_SPAN_MV    = 1600.0;
const float MAX_PPM_RANGE  = 100.0;

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Eloquent::ML::Port::SpoilageClassifier classifier;

bool sht31Ready = false;

// Debounce variables (requires 3 matching readings before switching status)
int consecutiveCount = 0;
int lastRawState = 0;
int confirmedState = 0;

float readSmoothedEthylenePPM() {
  long mvSum = 0;
  const int samples = 10;

  for (int i = 0; i < samples; i++) {
    mvSum += analogReadMilliVolts(ETH_V0_PIN);
    delay(10);
  }

  float avgMilliVolts = mvSum / (float)samples;
  float ppm = ((avgMilliVolts - ZERO_OFFSET_MV) / MAX_SPAN_MV) * MAX_PPM_RANGE;
  return (ppm < 0.0) ? 0.0 : ppm;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(I2C_SDA, I2C_SCL);
  sht31Ready = sht31.begin(0x44);

  analogReadResolution(12);
  analogSetPinAttenuation(ETH_V0_PIN, ADC_11db);

  Serial.println("ESP32 Spoilage Classifier Ready.");
}

void loop() {
  float eth = readSmoothedEthylenePPM();

  if (!sht31Ready) {
    sht31Ready = sht31.begin(0x44);
  }

  float temp = NAN;
  float hum  = NAN;

  if (sht31Ready) {
    temp = sht31.readTemperature();
    hum  = sht31.readHumidity();

    if (isnan(temp) || isnan(hum)) {
      sht31Ready = false;
    }
  }

  if (isnan(temp) || isnan(hum)) {
    Serial.printf("SHT31 Disconnected | Ethylene: %.3f ppm (Waiting for SHT31...)\n", eth);
    delay(2000);
    return;
  }

  // Feature order must match train_ml.py: [temp, humidity, ethylene]
  float features[3] = { temp, hum, eth };
  int rawPrediction = classifier.predict(features);

  // Confirm state only after 3 consecutive identical predictions (6 seconds)
  if (rawPrediction == lastRawState) {
    consecutiveCount++;
    if (consecutiveCount >= 3) {
      confirmedState = rawPrediction;
    }
  } else {
    consecutiveCount = 1;
    lastRawState = rawPrediction;
  }

  Serial.printf("Temp: %.2f C | Hum: %.2f %% | Ethylene: %.3f ppm -> ", temp, hum, eth);

  if (confirmedState == 0) {
    Serial.println("NORMAL");
  } else if (confirmedState == 1) {
    Serial.println("WARNING (Ripe)");
  } else if (confirmedState == 2) {
    Serial.println("CRITICAL (Spoiled)");
  }

  delay(2000);
}
