#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <DHT.h>

#define DHT_PIN   4
#define DHT_TYPE  DHT11

#define SERVICE_UUID "6e400001-b5a8-f393-e0a9-e50e24dcca9e"
#define TX_CHAR_UUID "6e400003-b5a8-f393-e0a9-e50e24dcca9e"

DHT dht(DHT_PIN, DHT_TYPE);
BLEServer*         pServer = nullptr;
BLECharacteristic* pTx     = nullptr;
bool deviceConnected    = false;
bool oldDeviceConnected = false;
unsigned long lastSend  = 0;
const unsigned long SEND_INTERVAL_MS = 10000;

/* --- Log every read Chrome makes (this is our diagnostic) --- */
class TxCallbacks : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic* c) {
    Serial.println("<< READ from browser (keep-alive fired)");
  }
};

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* srv, esp_ble_gatts_cb_param_t* param) {
    deviceConnected = true;
    lastSend = millis() - SEND_INTERVAL_MS + 1500;
    srv->updateConnParams(param->connect.remote_bda, 12, 24, 0, 400);
    Serial.println("BLE client connected");
    Serial.print("  peer addr: ");
    for (int i = 0; i < 6; i++)
      Serial.printf("%02X%s", param->connect.remote_bda[i], i < 5 ? ":" : "\n");
  }
  void onDisconnect(BLEServer* srv) {
    deviceConnected = false;
    Serial.println("BLE client disconnected");
    // Log the disconnect reason code — very useful!
    esp_gap_ble_cb_event_t* dummy = nullptr;  // just so the compiler is happy
    Serial.printf("  conn_id=%d\n", srv->getConnId());
  }
};

void setup() {
  Serial.begin(115200);
  delay(300);
  dht.begin();

  BLEDevice::init("ESP32-Storage");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* service = pServer->createService(SERVICE_UUID);

  pTx = service->createCharacteristic(
          TX_CHAR_UUID,
          BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY);
  pTx->addDescriptor(new BLE2902());
  pTx->setValue("T:--,H:--\n");
  pTx->setCallbacks(new TxCallbacks());   // <-- logs reads

  service->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("Advertising as 'ESP32-Storage'...");
}

void loop() {
  if (deviceConnected && millis() - lastSend >= SEND_INTERVAL_MS) {
    lastSend = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      Serial.println("DHT11 read failed");
    } else {
      char msg[32];
      int len = snprintf(msg, sizeof(msg), "T:%.1f,H:%.1f\n", t, h);
      pTx->setValue((uint8_t*)msg, len);
      pTx->notify();
      Serial.print("Sent: "); Serial.print(msg);
    }
  }
  if (!deviceConnected && oldDeviceConnected) {
    delay(300);
    BLEDevice::startAdvertising();
    Serial.println("Advertising again...");
  }
  oldDeviceConnected = deviceConnected;
  delay(10);
}