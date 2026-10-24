#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>

// ==========================================
// PIN DEFINITIONS & HARDWARE
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
// BLE UUIDs
// ==========================================
#define SERVICE_UUID "6e400001-b5a8-f393-e0a9-e50e24dcca9e"
#define TX_CHAR_UUID "6e400003-b5a8-f393-e0a9-e50e24dcca9e"

// ==========================================
// OBJECTS
// ==========================================
TwoWire I2C_1 = TwoWire(0);
TwoWire I2C_2 = TwoWire(1);

Adafruit_SHT31 sht31_1 = Adafruit_SHT31(&I2C_1);
Adafruit_SHT31 sht31_2 = Adafruit_SHT31(&I2C_2);

HardwareSerial Ethylene1(1);
HardwareSerial Ethylene2(2);

BLEServer*         pServer = nullptr;
BLECharacteristic* pTx     = nullptr;
bool deviceConnected    = false;
bool oldDeviceConnected = false;
unsigned long lastSend  = 0;
const unsigned long SEND_INTERVAL_MS = 10000;

// ==========================================
// SENSOR VARIABLES
// ==========================================
float temp1 = 0.0, hum1 = 0.0, eth1 = 0.0;
float temp2 = 0.0, hum2 = 0.0, eth2 = 0.0;
unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 2000; 

// Fan mapping (Ethylene ppm to PWM 0-255)
const float MIN_PPM = 5.0;    
const float MAX_PPM = 100.0;  
const int MIN_PWM = 0;        
const int MAX_PWM = 255;      

// ==========================================
// BLE CALLBACKS
// ==========================================
class TxCallbacks : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic* c) {
    Serial.println("<< READ from browser (keep-alive fired)");
  }
};

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* srv, esp_ble_gatts_cb_param_t* param) {
    deviceConnected = true;
    lastSend = millis() - SEND_INTERVAL_MS + 1500;
    // Request faster connection parameters (15ms-30ms interval)
    srv->updateConnParams(param->connect.remote_bda, 12, 24, 0, 400);
    Serial.println("BLE client connected");
  }
  void onDisconnect(BLEServer* srv) {
    deviceConnected = false;
    Serial.println("BLE client disconnected");
  }
};

// ==========================================
// HELPER: Read Ethylene
// ==========================================
float getEthyleneReading(HardwareSerial &serialPort) {
  if (serialPort.available()) {
    String data = serialPort.readStringUntil('\n');
    data.trim();
    float val = data.toFloat();
    if (val > 0.0 && val < 500.0) {
      return val;
    }
  }
  return -1.0;
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(300);

  // 1. Init I2C for SHT31s
  I2C_1.begin(SDA1_PIN, SCL1_PIN);
  I2C_2.begin(SDA2_PIN, SCL2_PIN);
  
  if (!sht31_1.begin(0x44)) {
    Serial.println("ERROR: SHT31 #1 not found on Container 1 bus!");
  }
  if (!sht31_2.begin(0x44)) {
    Serial.println("ERROR: SHT31 #2 not found on Container 2 bus!");
  }

  // 2. Init UART for Ethylene sensors
  Ethylene1.begin(9600, SERIAL_8N1, RX1_PIN, TX1_PIN);
  Ethylene2.begin(9600, SERIAL_8N1, RX2_PIN, TX2_PIN);

  // 3. Init PWM for Fans
  ledcSetup(0, 25000, 8);
  ledcAttachPin(FAN1_PIN, 0);
  ledcWrite(0, 0);

  ledcSetup(1, 25000, 8);
  ledcAttachPin(FAN2_PIN, 1);
  ledcWrite(1, 0);

  // 4. Init BLE (Tuned for Discoverability)
  Serial.println("Initializing BLE...");
  BLEDevice::init("ESP32-S3 N16R8-Storage"); // This sets the name Android/Windows will see
  BLEDevice::setMTU(100);           // Required for long strings

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* service = pServer->createService(SERVICE_UUID);
  
  pTx = service->createCharacteristic(
          TX_CHAR_UUID,
          BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pTx->addDescriptor(new BLE2902());
  pTx->setValue("T1:--,H1:--,E1:--,T2:--,H2:--,E2:--\n");
  pTx->setCallbacks(new TxCallbacks());

  service->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true); // Ensures the name is sent back when scanners request it
  
  // ---> THESE TWO LINES MAKE IT DISCOVERABLE FASTER ON ANDROID/WINDOWS <---
  adv->setMinPreferred(0x06); // Helps with iPhone/Android connection intervals
  adv->setMaxPreferred(0x12);
  
  BLEDevice::startAdvertising();

  Serial.println("=================================================");
  Serial.println("Advertising as: ESP32-Storage");
  Serial.println("Waiting for a Bluetooth connection...");
  Serial.println("=================================================");
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  // 1. Continuously read UART
  float newEth1 = getEthyleneReading(Ethylene1);
  if (newEth1 >= 0.0) eth1 = newEth1;

  float newEth2 = getEthyleneReading(Ethylene2);
  if (newEth2 >= 0.0) eth2 = newEth2;

  // 2. Every 2 seconds: Read SHTs, adjust fans, print to Serial
  if (millis() - lastReadTime >= READ_INTERVAL) {
    lastReadTime = millis();

    float t1 = sht31_1.readTemperature();
    float h1 = sht31_1.readHumidity();
    if (!isnan(t1)) temp1 = t1;
    if (!isnan(h1)) hum1 = h1;

    float t2 = sht31_2.readTemperature();
    float h2 = sht31_2.readHumidity();
    if (!isnan(t2)) temp2 = t2;
    if (!isnan(h2)) hum2 = h2;

    // Update Fans
    int fan1Speed = map(eth1 * 100, MIN_PPM * 100, MAX_PPM * 100, MIN_PWM, MAX_PWM);
    ledcWrite(0, constrain(fan1Speed, MIN_PWM, MAX_PWM));

    int fan2Speed = map(eth2 * 100, MIN_PPM * 100, MAX_PPM * 100, MIN_PWM, MAX_PWM);
    ledcWrite(1, constrain(fan2Speed, MIN_PWM, MAX_PWM));

    Serial.printf("C1: T:%.1f H:%.1f E:%.1f | C2: T:%.1f H:%.1f E:%.1f\n", 
                  temp1, hum1, eth1, temp2, hum2, eth2);
  }

  // 3. Every 10 seconds: Send BLE Notification
  if (deviceConnected && millis() - lastSend >= SEND_INTERVAL_MS) {
    lastSend = millis();

    char msg[80];
    int len = snprintf(msg, sizeof(msg), "T1:%.1f,H1:%.1f,E1:%.1f,T2:%.1f,H2:%.1f,E2:%.1f\n", 
                       temp1, hum1, eth1, temp2, hum2, eth2);
    
    pTx->setValue((uint8_t*)msg, len);
    pTx->notify();
    Serial.print("Sent BLE: "); Serial.print(msg);
  }

  // 4. Handle disconnects (Restart Advertising)
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Give the Bluetooth stack a moment to clear
    pServer->startAdvertising(); // Restart advertising
    Serial.println("Client disconnected. Advertising again...");
  }
  oldDeviceConnected = deviceConnected;
  
  delay(10);
}
