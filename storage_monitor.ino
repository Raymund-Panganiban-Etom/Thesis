/*
  IoT-Enabled Edge AI Post-Harvest Storage Systems  -  ESP32 firmware

  Sends one JSON line every 2 s over Bluetooth Classic (SPP) to the DroidScript app:
    {"temp":37.1,"humidity":87.5,"ethylene":42.0,"battery":88}

  The app looks for the Bluetooth device named BT_NAME below (must match BT_NAME in the app),
  so pair the ESP32 in Android Bluetooth settings first.

  IMPORTANT - BOARD:
    Bluetooth Classic (BluetoothSerial) works on the ORIGINAL ESP32 (DevKit / WROOM / WROVER).
    The ESP32-S3 has BLE only, so this sketch will NOT compile for an S3 (see the #error below).

  Libraries (Library Manager):  "Adafruit SHT31 Library"  (installs Adafruit BusIO as well)
  Board:  "ESP32 Dev Module"  (esp32 board package by Espressif)

  Wiring (default pins, change below if needed):
    SHT31      SDA -> GPIO21, SCL -> GPIO22, VIN -> 3.3V, GND -> GND   (address 0x44)
    Ethylene   analog output -> GPIO34   (see readEthylenePpm(), CALIBRATE THIS)
    Battery    pack+ -> divider -> GPIO35 (see BAT_DIVIDER)
    Fan relay  IN -> GPIO26   (HIGH = fan ON; set RELAY_ACTIVE_LOW if your module is active-low)
*/

#include <Wire.h>
#include <Adafruit_SHT31.h>
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED) || !defined(CONFIG_BT_SPP_ENABLED)
#error "Bluetooth Classic (SPP) is not available on this board. Use an original ESP32 (not ESP32-S3/C3) and select 'ESP32 Dev Module'."
#endif

// ---------- Settings ----------
const char*   BT_NAME        = "ESP32_S3_Storage";  // must match BT_NAME in the app
const uint32_t SEND_EVERY_MS = 2000;

// Pins
const int PIN_SDA   = 21;
const int PIN_SCL   = 22;
const int PIN_ETH   = 34;   // ethylene sensor analog output (input-only pin)
const int PIN_BAT   = 35;   // battery divider (input-only pin)
const int PIN_RELAY = 26;   // purge fan relay
const bool RELAY_ACTIVE_LOW = false;

// Fan control (same limit as the app: 330 ppm), with hysteresis so the relay doesn't chatter
const float FAN_ON_PPM  = 330.0;
const float FAN_OFF_PPM = 280.0;

// Ethylene sensor calibration (PLACEHOLDER - replace with your sensor's datasheet / calibration)
// Assumes a linear analog output: ETH_MV_ZERO mV = 0 ppm, ETH_MV_FULL mV = ETH_PPM_FULL ppm.
const float ETH_MV_ZERO  = 400.0;
const float ETH_MV_FULL  = 3000.0;
const float ETH_PPM_FULL = 500.0;

// Battery (LiFePO4 pack) - voltage divider and pack limits
const float BAT_DIVIDER = 5.0;    // (R1+R2)/R2, e.g. 4:1 ratio -> 5.0
const float BAT_V_EMPTY = 10.0;   // pack voltage treated as 0%   (example: 4S LiFePO4)
const float BAT_V_FULL  = 14.4;   // pack voltage treated as 100%
// Note: LiFePO4 voltage is very flat, so this percentage is only an approximation.

// ---------- Globals ----------
BluetoothSerial SerialBT;
Adafruit_SHT31 sht31;
bool shtOk = false;
bool fanOn = false;
uint32_t lastSend = 0;

// ---------- Helpers ----------
void setFan(bool on) {
  fanOn = on;
  digitalWrite(PIN_RELAY, (on != RELAY_ACTIVE_LOW) ? HIGH : LOW);
}

// Average several ADC samples (in millivolts) to reduce noise
float readMilliVolts(int pin) {
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) {
    sum += analogReadMilliVolts(pin);
    delay(2);
  }
  return sum / 16.0f;
}

float readEthylenePpm() {
  float mv  = readMilliVolts(PIN_ETH);
  float ppm = (mv - ETH_MV_ZERO) / (ETH_MV_FULL - ETH_MV_ZERO) * ETH_PPM_FULL;
  if (ppm < 0) ppm = 0;
  if (ppm > 500) ppm = 500;
  return ppm;
}

float readBatteryPercent() {
  float packV = readMilliVolts(PIN_BAT) / 1000.0f * BAT_DIVIDER;
  float pct = (packV - BAT_V_EMPTY) / (BAT_V_FULL - BAT_V_EMPTY) * 100.0f;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

// ---------- Arduino ----------
void setup() {
  Serial.begin(115200);

  pinMode(PIN_RELAY, OUTPUT);
  setFan(false);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_ETH, ADC_11db);   // ~0-3.1 V range
  analogSetPinAttenuation(PIN_BAT, ADC_11db);

  Wire.begin(PIN_SDA, PIN_SCL);
  shtOk = sht31.begin(0x44);                    // try 0x45 if your module uses the alternate address
  if (!shtOk) Serial.println("SHT31 not found - check wiring");

  SerialBT.begin(BT_NAME);                      // device visible as BT_NAME
  Serial.print("Bluetooth ready. Pair with: ");
  Serial.println(BT_NAME);
}

void loop() {
  if (millis() - lastSend < SEND_EVERY_MS) return;
  lastSend = millis();

  float t = shtOk ? sht31.readTemperature() : NAN;
  float h = shtOk ? sht31.readHumidity()    : NAN;
  float e = readEthylenePpm();
  float b = readBatteryPercent();

  // Fan logic with hysteresis
  if (!fanOn && e >= FAN_ON_PPM)  setFan(true);
  if (fanOn  && e <= FAN_OFF_PPM) setFan(false);

  // The app ignores a line unless temp, humidity and ethylene are all valid numbers
  if (isnan(t) || isnan(h)) {
    Serial.println("SHT31 read failed - skipping this reading");
    shtOk = sht31.begin(0x44);                  // try to recover
    return;
  }

  char line[96];
  snprintf(line, sizeof(line),
           "{\"temp\":%.1f,\"humidity\":%.1f,\"ethylene\":%.1f,\"battery\":%.0f}",
           t, h, e, b);

  Serial.println(line);                         // for the Serial Monitor
  if (SerialBT.hasClient()) SerialBT.println(line);   // newline-terminated, as the app expects
}
