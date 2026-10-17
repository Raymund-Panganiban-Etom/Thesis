/*
  ESP32-S3 post-harvest storage node -> Bluetooth serial -> DroidScript app

  The ESP32-S3 has NO classic Bluetooth (only BLE), and the app connects with
  classic Bluetooth serial (SPP). So this sketch sends its data out of a UART
  to an HC-05 (or HC-06) module, which the phone pairs with like any SPP device.

  Wiring (change the pins below if your board differs)
    SHT31  (temperature / humidity)  SDA -> GPIO 8,  SCL -> GPIO 9,  3V3, GND
    Ethylene sensor analog output    -> GPIO 4  (ADC1)
    HC-05  TXD -> GPIO 18 (ESP32 RX),  RXD -> GPIO 17 (ESP32 TX)  [use a divider
           or level shifter if your HC-05 RXD is not 3.3 V tolerant]
           VCC -> 5 V (HC-05 module), GND -> GND

  Setup once: put the HC-05 in AT mode and set its name to ESP32_S3_Storage
  (AT+NAME=ESP32_S3_Storage), then pair it in Android Bluetooth settings
  (default PIN 1234 or 0000).

  Libraries (Library Manager): "Adafruit SHT31 Library" (installs Adafruit BusIO).
  Board: ESP32S3 Dev Module.

  Output, one JSON line every 2 s (what the app expects):
    {"temp":37.1,"humidity":87.5,"ethylene":42.0}
*/

#include <Wire.h>
#include <Adafruit_SHT31.h>

// ---- pins ----
const int PIN_SDA = 8;
const int PIN_SCL = 9;
const int PIN_ETH = 4;
const int PIN_BT_RX = 18;   // ESP32 receives from HC-05 TXD
const int PIN_BT_TX = 17;   // ESP32 sends to HC-05 RXD

// ---- settings ----
const unsigned long BT_BAUD = 9600;       // HC-05 default data rate
const unsigned long SEND_MS = 2000;

// Ethylene sensor calibration. REPLACE these with your sensor's datasheet or
// your own calibration points: raw ADC (0..4095) -> ppm, linear between two points.
const float ETH_RAW_ZERO = 200.0;   // ADC reading in clean air
const float ETH_PPM_ZERO = 0.0;
const float ETH_RAW_SPAN = 3000.0;  // ADC reading at the known span concentration
const float ETH_PPM_SPAN = 500.0;   // ppm at that reading

Adafruit_SHT31 sht31 = Adafruit_SHT31();
HardwareSerial BT(1);
unsigned long lastSend = 0;
bool shtOk = false;

float readEthylenePpm() {
  long sum = 0;
  for (int i = 0; i < 16; i++) {   // average to smooth ADC noise
    sum += analogRead(PIN_ETH);
    delay(2);
  }
  float raw = sum / 16.0;
  float ppm = (raw - ETH_RAW_ZERO) * (ETH_PPM_SPAN - ETH_PPM_ZERO) / (ETH_RAW_SPAN - ETH_RAW_ZERO) + ETH_PPM_ZERO;
  if (ppm < 0) ppm = 0;
  return ppm;
}

void setup() {
  Serial.begin(115200);
  BT.begin(BT_BAUD, SERIAL_8N1, PIN_BT_RX, PIN_BT_TX);
  analogReadResolution(12);
  Wire.begin(PIN_SDA, PIN_SCL);
  shtOk = sht31.begin(0x44);       // use 0x45 if your SHT31 ADDR pin is high
  if (!shtOk) Serial.println("SHT31 not found, check wiring");
}

void loop() {
  if (millis() - lastSend < SEND_MS) return;
  lastSend = millis();

  float t = shtOk ? sht31.readTemperature() : NAN;
  float h = shtOk ? sht31.readHumidity() : NAN;
  float e = readEthylenePpm();

  // never send made-up numbers: skip the line if a sensor failed
  if (isnan(t) || isnan(h)) {
    Serial.println("Sensor read failed, nothing sent");
    return;
  }

  char line[96];
  snprintf(line, sizeof(line), "{\"temp\":%.1f,\"humidity\":%.1f,\"ethylene\":%.2f}", t, h, e);
  BT.println(line);       // newline-terminated, as the app expects
  Serial.println(line);   // same line on the USB serial monitor
}
