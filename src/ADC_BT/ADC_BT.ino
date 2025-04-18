#include <BLE2902.h>
#include <BLE2901.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// === Configuration ===
const int channels = 8;
const int buffer = 16;
const int ave_len = 1; // averaging samples

// === ADC Pins ===
const adc1_channel_t adcPins[8] = {
  ADC1_CHANNEL_0, ADC1_CHANNEL_1, ADC1_CHANNEL_2, ADC1_CHANNEL_3,
  ADC1_CHANNEL_4, ADC1_CHANNEL_5, ADC1_CHANNEL_6, ADC1_CHANNEL_7
};

// === BLE UUIDs ===
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// === Globals ===
uint8_t voltageBytes[sizeof(float) * channels * buffer];
SemaphoreHandle_t dataMutex;

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
BLE2901 *descriptor_2901 = NULL;
bool deviceConnected = false;
String BT_name = "IBME";

esp_adc_cal_characteristics_t *adc_chars;

// === BLE Server Callbacks ===
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("BLE Connected");
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    BLEDevice::startAdvertising();
    Serial.println("BLE Disconnected");
  }
};

// === ADC Calibration ===
void calibrate_adc() {
  adc1_config_width(ADC_WIDTH_BIT_12);
  for (int i = 0; i < channels; i++) {
    adc1_config_channel_atten(adcPins[i], ADC_ATTEN_DB_11);
  }

  adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, adc_chars);
}

// === ADC Task ===
void adcTask(void *param) {
  unsigned long lastSendTimeSample = micros();
  unsigned long lastSendTimeADC = micros();

  while (true) {
    uint8_t localBuf[sizeof(float) * channels * buffer];

    for (int i = 0; i < buffer; i++) {
      unsigned long now = micros();
      long delta = now - lastSendTimeSample;
      lastSendTimeSample = now;

      float voltages[channels] = {0};

      for (int j = 0; j < ave_len; j++) {
        for (int c = 0; c < channels; c++) {
          voltages[c] += adc1_get_raw(adcPins[c]);
        }
      }

      for (int c = 0; c < channels; c++) {
        voltages[c] /= (float)ave_len;
        voltages[c] = esp_adc_cal_raw_to_voltage((int)voltages[c], adc_chars) / 1000.0f;
      }

      int offset = i * channels * sizeof(float);
      for (int c = 0; c < channels; c++) {
        memcpy(localBuf + offset + c * sizeof(float), &voltages[c], sizeof(float));
      }

      delta = 500 - delta;
      delayMicroseconds(delta > 0 ? delta : 1);
    }

    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      memcpy(voltageBytes, localBuf, sizeof(voltageBytes));
      xSemaphoreGive(dataMutex);
    }

    unsigned long now = micros();
    long delta = now - lastSendTimeADC;
    lastSendTimeADC = now;

    // Serial.print("ADC buffer interval: ");
    // Serial.print(delta);
    // Serial.println(" µs");
  }
}

// === BLE Transmission Task ===
void bleTask(void *param) {
  unsigned long lastSendTimeBLE = micros();

  while (true) {
    if (deviceConnected) {
      if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        pCharacteristic->setValue(voltageBytes, sizeof(voltageBytes));
        pCharacteristic->notify();
        xSemaphoreGive(dataMutex);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(8)); // adjust as needed

    unsigned long now = micros();
    long delta = now - lastSendTimeBLE;
    lastSendTimeBLE = now;

    // Serial.print("BLE transmission interval: ");
    // Serial.print(delta);
    // Serial.println(" µs");
  }
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  calibrate_adc();

  BLEDevice::init(BT_name.c_str());
  BLEDevice::setMTU(517); // Max BLE MTU

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );

  pCharacteristic->addDescriptor(new BLE2902());
  descriptor_2901 = new BLE2901();
  descriptor_2901->setDescription("Multi-channel ADC Streaming");
  pCharacteristic->addDescriptor(descriptor_2901);

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();

  Serial.println("BLE ready. ESP32 Address:");
  Serial.println(BLEDevice::getAddress().toString().c_str());

  dataMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(adcTask, "ADC Task", 4096, NULL, 1, NULL, 1); // Core 1
  xTaskCreatePinnedToCore(bleTask, "BLE Task", 4096, NULL, 1, NULL, 0); // Core 0
}

// === Main Loop (unused) ===
void loop() {
  // Nothing here; tasks handle everything.
}
