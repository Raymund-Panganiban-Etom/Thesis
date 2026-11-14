#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <math.h>

// Standard ESP32 Pin Definitions
#define I2C_SDA 21
#define I2C_SCL 22
#define ETH_RX_PIN 16  // ESP32 RX2 -> Connect to YYS SM4-C2H4 TX
#define ETH_TX_PIN 17  // ESP32 TX2 -> Connect to YYS SM4-C2H4 RX

Adafruit_SHT31 sht31 = Adafruit_SHT31();
HardwareSerial ethSerial(2);  // Use UART2 on standard ESP32

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
    delay(100);  // Give sensor 100ms to respond
  }

  // Check if the sensor replied at all
  if (ethSerial.available() == 0) {
    Serial.println("UART DEBUG: 0 bytes received (Check TX->RX crossover & 5V power)");
    return lastValidEthPPM;
  }

  // Search for 0xFF start byte and parse 9-byte packet
  while (ethSerial.available() >= 9) {
    if (ethSerial.peek() != 0xFF) {
      uint8_t badByte = ethSerial.read();
      Serial.printf("UART DEBUG: Skipped non-header byte: 0x%02X\n", badByte);
      continue;
    }

    uint8_t frame[9];
    ethSerial.readBytes(frame, 9);

    // Print raw 9-byte hex packet to Serial Monitor
    Serial.print("UART RX BYTES: ");
    for (int i = 0; i < 9; i++) {
      Serial.printf("%02X ", frame[i]);
    }

    if (verifyChecksum(frame)) {
      Serial.println("[Checksum OK]");
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
    } else {
      Serial.println("[Checksum FAIL]");
    }
  }

  // Print any leftover partial bytes if fewer than 9 arrived
  if (ethSerial.available() > 0) {
    Serial.printf("UART DEBUG: Partial packet (%d bytes): ", ethSerial.available());
    while (ethSerial.available() > 0) {
      Serial.printf("%02X ", ethSerial.read());
    }
    Serial.println();
  }

  return lastValidEthPPM;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize I2C on standard ESP32 pins (GPIO 21 & 22)
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!sht31.begin(0x44)) {
    Serial.println("ERROR: SHT31 not found! Check GPIO 21 (SDA) and GPIO 22 (SCL).");
    while (1) delay(100);
  }

  // Initialize UART2 at 9600 baud for YYS SM4-C2H4
  ethSerial.begin(9600, SERIAL_8N1, ETH_RX_PIN, ETH_TX_PIN);

  // Print CSV header
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

  delay(2000);
}
