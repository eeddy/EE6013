#include <BLE2902.h>
#include <BLE2901.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

const int adc1Pin = 1;  
const int adc2Pin = 1; 
int adc1Value = 0;    
int adc2Value = 0; 

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
BLE2901 *descriptor_2901 = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
String BT_name = "IBME"; 

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Connected"); 
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    BLEDevice::startAdvertising();
    Serial.println("Disconnected"); 
  }
};

void setup() {
  Serial.begin(115200);
 
  BLEDevice::init("ESP32");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE
  );

  pCharacteristic->addDescriptor(new BLE2902());
  descriptor_2901 = new BLE2901();
  descriptor_2901->setDescription("My own description for this characteristic.");
  // descriptor_2901->setAccessPermissions(ESP_GATT_PERM_READ);
  pCharacteristic->addDescriptor(descriptor_2901);

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0); 
  BLEDevice::startAdvertising();

  Serial.println("ESP32 Bluetooth Address: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());
}

void loop() {
  adc1Value = analogRead(adc1Pin); 
  // adc2Value = analogRead(adc2Pin); 
  float voltage1 = adc1Value * (3.3 / 4095.0); 
  float voltage2 = adc2Value * (3.3 / 4095.0); 

  uint8_t voltageBytes[sizeof(voltage1) + sizeof(voltage2)];  

  memcpy(voltageBytes, &voltage1, sizeof(voltage1));  
  memcpy(voltageBytes + sizeof(voltage1), &voltage2, sizeof(voltage2));  

  // Serial.print("ADC1 Value: ");
  // Serial.print(adc1Value);
  // Serial.print(" | Voltage: ");
  // Serial.print(voltage1, 3); 
  // Serial.print(" ; ADC2 Value: ");
  // Serial.print(adc2Value);
  // Serial.print(" | Voltage: ");
  // Serial.println(voltage2, 3); 

  if (deviceConnected) {
    pCharacteristic->setValue(voltageBytes, sizeof(voltageBytes));
    pCharacteristic->notify();
  }

  delay(1); 
}
