#include <Wire.h>
#include <Adafruit_SHT31.h>

// ==========================================
// PIN DEFINITIONS
// ==========================================
// Container 1
#define SDA1_PIN 8
#define SCL1_PIN 9
#define RX1_PIN 17
#define TX1_PIN 18
#define FAN1_PIN 6

// Container 2
#define SDA2_PIN 4
#define SCL2_PIN 5
#define RX2_PIN 15
#define TX2_PIN 16
#define FAN2_PIN 7

// ==========================================
// PWM SETTINGS (Fan Speed)
// ==========================================
#define PWM_FREQ 25000  // 25 kHz (silent operation)
#define PWM_RES  8      // 8-bit (0 to 255)

// --- Ethylene to Fan Speed Mapping ---
// If ethylene is below MIN_PPM, fan is off (or minimum speed).
// If ethylene is above MAX_PPM, fan runs at 100% (255).
const float MIN_PPM = 5.0;    // Start spinning at 5 ppm
const float MAX_PPM = 100.0;  // Max out at 100 ppm
const int MIN_PWM = 0;        // 0% duty cycle at MIN_PPM
const int MAX_PWM = 255;      // 100% duty cycle at MAX_PPM

// ==========================================
// OBJECT INITIALIZATION
// ==========================================
TwoWire I2C_1 = TwoWire(0);
TwoWire I2C_2 = TwoWire(1);
Adafruit_SHT31 sht31_1 = Adafruit_SHT31();
Adafruit_SHT31 sht31_2 = Adafruit_SHT31();
HardwareSerial Ethylene1(1);
HardwareSerial Ethylene2(2);

// Variables to hold sensor data
float temp1 = 0.0, hum1 = 0.0, eth1 = 0.0;
float temp2 = 0.0, hum2 = 0.0, eth2 = 0.0;

// Timer for reading sensors
unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 2000; // Read every 2 seconds

// ==========================================
// HELPER FUNCTION: Read Ethylene UART
// ==========================================
float getEthyleneReading(HardwareSerial &serialPort) {
  if (serialPort.available()) {
    String data = serialPort.readStringUntil('\n');
    data.trim(); // Remove whitespace/newlines
    
    // Attempt to extract the number.
    // Adjust this parsing logic if your SM4 module sends a specific string format
    // like "C2H4: 12.5 ppm". For now, it extracts the first valid float it finds.
    float val = data.toFloat();
    
    // If the value is valid and not zero, update it. 
    // (Sometimes serial noise gives 0.0 on failure)
    if (val > 0.0 && val < 500.0) {
      return val;
    }
  }
  return -1.0; // Return -1 if no new reading
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  Serial.println("Starting Edge AI Storage System...");

  // 1. Initialize I2C for SHT31s
  I2C_1.begin(SDA1_PIN, SCL1_PIN);
  I2C_2.begin(SDA2_PIN, SCL2_PIN);
  
  if (!sht31_1.begin(0x44, &I2C_1)) {
    Serial.println("ERROR: SHT31 #1 not found on Container 1 bus!");
  }
  if (!sht31_2.begin(0x44, &I2C_2)) {
    Serial.println("ERROR: SHT31 #2 not found on Container 2 bus!");
  }

  // 2. Initialize UART for SM4-C2H4s
  // Assuming 9600 baud. Change if your module requires a different baud rate.
  Ethylene1.begin(9600, SERIAL_8N1, RX1_PIN, TX1_PIN);
  Ethylene2.begin(9600, SERIAL_8N1, RX2_PIN, TX2_PIN);

  // 3. Initialize PWM for Fans
  ledcSetup(0, PWM_FREQ, PWM_RES);
  ledcAttachPin(FAN1_PIN, 0);
  ledcWrite(0, 0); // Start off

  ledcSetup(1, PWM_FREQ, PWM_RES);
  ledcAttachPin(FAN2_PIN, 1);
  ledcWrite(1, 0); // Start off

  Serial.println("Setup Complete. Waiting for sensors...");
  Serial.println("-----------------------------------------------------------------");
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  // Read ethylene continuously (UART data can arrive at any time)
  float newEth1 = getEthyleneReading(Ethylene1);
  if (newEth1 >= 0.0) eth1 = newEth1;

  float newEth2 = getEthyleneReading(Ethylene2);
  if (newEth2 >= 0.0) eth2 = newEth2;

  // Every 2 seconds, read Temp/Humidity, calculate fan speed, and print
  if (millis() - lastReadTime >= READ_INTERVAL) {
    lastReadTime = millis();

    // --- Read SHT31 Sensors ---
    float t1 = sht31_1.readTemperature();
    float h1 = sht31_1.readHumidity();
    if (!isnan(t1) && !isnan(h1)) { temp1 = t1; hum1 = h1; }

    float t2 = sht31_2.readTemperature();
    float h2 = sht31_2.readHumidity();
    if (!isnan(t2) && !isnan(h2)) { temp2 = t2; hum2 = h2; }

    // --- Calculate Fan Speeds ---
    // Map the ethylene PPM to a PWM value (0 to 255)
    int fan1Speed = map(eth1 * 100, MIN_PPM * 100, MAX_PPM * 100, MIN_PWM, MAX_PWM);
    fan1Speed = constrain(fan1Speed, MIN_PWM, MAX_PWM); // Keep between 0 and 255
    ledcWrite(0, fan1Speed);

    int fan2Speed = map(eth2 * 100, MIN_PPM * 100, MAX_PPM * 100, MIN_PWM, MAX_PWM);
    fan2Speed = constrain(fan2Speed, MIN_PWM, MAX_PWM);
    ledcWrite(1, fan2Speed);

    // --- Print to Serial Monitor ---
    printDashboard(fan1Speed, fan2Speed);
  }
}

// ==========================================
// PRETTY PRINT FUNCTION
// ==========================================
void printDashboard(int f1Speed, int f2Speed) {
  // Convert 0-255 PWM back to 0-100% for display
  int fan1Percent = map(f1Speed, 0, 255, 0, 100);
  int fan2Percent = map(f2Speed, 0, 255, 0, 100);

  Serial.println("================ SYSTEM STATUS ================");
  
  Serial.println("[ CONTAINER 1 ]");
  Serial.printf("  Temp:     %.1f C\n", temp1);
  Serial.printf("  Humidity: %.1f %%\n", hum1);
  Serial.printf("  Ethylene: %.1f ppm\n", eth1);
  Serial.printf("  Fan Speed: %d%% (PWM: %d)\n", fan1Percent, f1Speed);

  Serial.println("-----------------------------------------------");

  Serial.println("[ CONTAINER 2 ]");
  Serial.printf("  Temp:     %.1f C\n", temp2);
  Serial.printf("  Humidity: %.1f %%\n", hum2);
  Serial.printf("  Ethylene: %.1f ppm\n", eth2);
  Serial.printf("  Fan Speed: %d%% (PWM: %d)\n", fan2Percent, f2Speed);
  
  Serial.println("===============================================\n");
}