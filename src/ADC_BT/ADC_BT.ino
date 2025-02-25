#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLESecurity.h>

const int adcPin = 1;  
int adcValue = 0;    
String BT_name = "ESP32_IBME"; 
BLECharacteristic *pCharacteristic;

void setup() {
  Serial.begin(115200);
 
  BLEDevice::init(BT_name);
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
  pCharacteristic = pService->createCharacteristic(
    "beb5483e-36e1-4688-b7f5-ea07361b26a8",
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic->setValue("Start");
  pService->start();
 
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->start();
 
  // pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
  // pSecurity->setCapability(ESP_IO_CAP_IO);
  // pSecurity->setKeySize(16);
  // pSecurity->setStaticPIN(123456);  // Set a 6-digit passkey here
  // BLEDevice::setSecurityCallbacks(new MySecurity());
  // pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  pAdvertising->setScanResponse(true);
  // pAdvertising->setMinPreferred(0x06);
  // pAdvertising->setMinPreferred(0x12);

  Serial.println("ESP32 Bluetooth Address: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());
}

void loop() {
  adcValue = analogRead(adcPin);  // Read the ADC value from pin 1
  float voltage = adcValue * (3.3 / 4095.0);  // Convert ADC value to voltage

  pCharacteristic->setValue(String(adcValue).c_str());  
  pCharacteristic->notify();

  // Optional: Print out the ADC value and voltage to the Serial Monitor
  // Serial.print("ADC Value: ");
  // Serial.print(adcValue);
  // Serial.print(" | Voltage: ");
  // Serial.println(voltage, 3); 

  delay(1000);  // Wait for 1 second before sending the next value
}
