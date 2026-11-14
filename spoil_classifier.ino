#include <Wire.h>
#include <Adafruit_SHT31.h>

// Safe pins for standard ESP32 (avoids SPI Flash pins 6-11)
#define I2C_SDA 21
#define I2C_SCL 22
#define ETH_V0_PIN 34

const float ZERO_OFFSET_MV = 400.0;
const float MAX_SPAN_MV    = 1600.0;
const float MAX_PPM_RANGE  = 100.0;

// 5 minutes in milliseconds (5 * 60 * 1000)
const unsigned long RECORD_DURATION_MS = 300000UL;
unsigned long startTime = 0;

Adafruit_SHT31 sht31 = Adafruit_SHT31();
bool sht31Ready = false;

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

  // Do not halt if SHT31 is disconnected; SM4-C2H4 will still run
  sht31Ready = sht31.begin(0x44);

  analogReadResolution(12);
  analogSetPinAttenuation(ETH_V0_PIN, ADC_11db);

  Serial.println("temp,humidity,ethylene");

  // Record the exact time logging starts
  startTime = millis();
}

void loop() {
  // Check if 5 minutes (300,000 ms) have elapsed
  if (millis() - startTime >= RECORD_DURATION_MS) {
    Serial.println("--- 5 MINUTES COMPLETE: LOGGING STOPPED ---");
    while (true) {
      delay(1000); // Halt further readings until you press the ESP32 EN/RST button
    }
  }

  // 1. Always read YYS SM4-C2H4 independently
  float eth = readSmoothedEthylenePPM();

  // 2. Attempt SHT31 read (and auto-recover if reconnected mid-run)
  if (!sht31Ready) {
    sht31Ready = sht31.begin(0x44);
  }

  float temp = NAN;
  float hum  = NAN;

  if (sht31Ready) {
    temp = sht31.readTemperature();
    hum  = sht31.readHumidity();

    // Mark false if unplugged mid-run so it re-initializes when plugged back in
    if (isnan(temp) || isnan(hum)) {
      sht31Ready = false;
    }
  }

  // 3. Keep valid 3-column CSV format even when SHT31 is disconnected
  if (!isnan(temp) && !isnan(hum)) {
    Serial.print(temp, 2);
    Serial.print(",");
    Serial.print(hum, 2);
  } else {
    Serial.print("NaN,NaN");
  }

  Serial.print(",");
  Serial.println(eth, 3);

  delay(2000);
}
