#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <math.h>

// ESP32-S3 N16R8 Pin Definitions
#define I2C_SDA 8
#define I2C_SCL 9
#define ETH_RX_PIN 17  // Connect to YYS SM4-C2H4 TX
#define ETH_TX_PIN 18  // Connect to YYS SM4-C2H4 RX

Adafruit_SHT31 sht31 = Adafruit_SHT31();
HardwareSerial ethSerial(1);  // Use UART1 for ethylene sensor

float lastValidEthPPM = 0.0;

// Validates standard 9-byte YYS / Winsen UART checksum
bool verifyChecksum(const uint8_t *frame) {
  uint8_t sum = 0;
  for (int i = 1; i < 8; i++) {
    sum += frame[i];
  }
  uint8_t checksum = (~sum) + 1;
  return (checksum == frame[8]);
}

float readEthyleneUART() {
  // Drain old buffered frames so we always read the latest packet
  while (ethSerial.available() > 9) {
    ethSerial.read();
  }

  // If no active data is streaming yet, send standard Q&A poll command
  if (ethSerial.available() < 9) {
    const uint8_t pollCmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    ethSerial.write(pollCmd, 9);
    delay(50);
  }

  // Search for 0xFF start byte and parse 9-byte packet
  while (ethSerial.available() >= 9) {
    if (ethSerial.peek() != 0xFF) {
      ethSerial.read();  // Discard misaligned byte
      continue;
    }

    uint8_t frame[9];
    ethSerial.readBytes(frame, 9);

    if (verifyChecksum(frame)) {
      float ppm = 0.0;

      if (frame[1] == 0x86) {
        // Q&A Mode response: [FF, 86, HighByte, LowByte, ...]
        uint16_t rawVal = ((uint16_t)frame[2] << 8) | frame[3];
        ppm = rawVal / 10.0f;  // Standard 0.1 ppm resolution for SM4-C2H4
      } else {
        // Active Upload Mode: [FF, GasID, Unit, Decimals, HighByte, LowByte, ...]
        uint8_t decimals = frame[3];
        uint16_t rawVal = ((uint16_t)frame[4] << 8) | frame[5];
        ppm = rawVal / pow(10.0f, decimals);
      }

      lastValidEthPPM = (ppm < 0.0f) ? 0.0f : ppm;
      return lastValidEthPPM;
    }
  }

  // Return last confirmed reading if a packet wasn't ready this cycle
  return lastValidEthPPM;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize I2C for SHT31
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!sht31.begin(0x44)) {
    Serial.println("ERROR: SHT31 not found! Check SDA/SCL wiring.");
    while (1) delay(100);
  }

  // Initialize UART1 at 9600 baud for YYS SM4-C2H4
  ethSerial.begin(9600, SERIAL_8N1, ETH_RX_PIN, ETH_TX_PIN);

  // Print CSV header for easy copy-paste
  Serial.println("temp,humidity,ethylene");
}

void loop() {
  float temp = sht31.readTemperature();
  float hum  = sht31.readHumidity();
  float eth  = readEthyleneUART();

  if (!isnan(temp) && !isnan(hum)) {
    Serial.print(temp, 2);
    Serial.print(",");
    Serial.print(hum, 2);
    Serial.print(",");
    Serial.println(eth, 3);
  } else {
    Serial.println("ERROR: Failed to read SHT31");
  }

  delay(2000);  // Log 1 reading every 2 seconds
}
