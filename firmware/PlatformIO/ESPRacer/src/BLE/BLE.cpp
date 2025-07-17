#include "Arduino.h"
#include "BLE.h"

String strPad(String bin, int zeroCount)
{
    int len = sizeof(bin);
    for (int i = len; i < zeroCount; i++)
    {
        bin = "0" + bin;
    }
    return bin;
}

ServerCallbacks::ServerCallbacks(bool debug = false)
{
    debugMode = debug;
}

BLE::BLE(const char *deviceName, bool debugMode = false)
{
    this->debugMode = debugMode;
    this->deviceName = deviceName;
}

bool BLE::isConnected()
{
    return *deviceConnected;
}

void BLE::startAdvertising()
{
    BLEDevice::startAdvertising();
    if (debugMode)
        Serial.println("BLE Advertising");
}

void BLE::setup(const char *ADVERTISING_UUID, const char *SERVICE_UUID, const char *CHARACTERISTIC_UUID_RX, const char *CHARACTERISTIC_UUID_TX)
{
    this->ADVERTISING_UUID = ADVERTISING_UUID;
    this->SERVICE_UUID = SERVICE_UUID;
    this->CHARACTERISTIC_UUID_RX = CHARACTERISTIC_UUID_RX;
    this->CHARACTERISTIC_UUID_TX = CHARACTERISTIC_UUID_TX;

    if (debugMode)
        Serial.println("Starting BLE");

    // Create the BLE Device
    char deviceName_Id[32];
    strcpy(deviceName_Id, deviceName);
    strcat(deviceName_Id, " - ");
    strcat(deviceName_Id, deviceId);
    BLEDevice::init(deviceName_Id);

    Serial.println(deviceName_Id);

    // Create the BLE Server
    BLEServer *bleServer = BLEDevice::createServer();
    callback = new ServerCallbacks(debugMode);
    bleServer->setCallbacks(callback);

    // Create the BLE Service
    BLEService *bleService = bleServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    bleTxCharacteristic = bleService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY);
    bleTxCharacteristic->addDescriptor(new BLE2902());

    bleRxCharacteristic = bleService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE);
    bleRxCharacteristic->setCallbacks(new MyCallbacks());

    // Create the BLE Service
    BLEService *bleServiceBattery = bleServer->createService(SERVICE_UUID_BATTERY);

    // Create a BLE Characteristic
    bleBatteryCharacteristic = bleServiceBattery->createCharacteristic(
        CHARACTERISTIC_UUID_BATTERY,
        BLECharacteristic::PROPERTY_READ);
    setBattery(-1);

    // Start the service
    bleService->start();
    bleServiceBattery->start();

    BLEAdvertising *bleAdvertising = bleServer->getAdvertising(); // アドバタイズオブジェクトを取得
    bleAdvertising->addServiceUUID(ADVERTISING_UUID);

    // Start Advertising
    startAdvertising();

    // Pointer Set
    deviceConnected = &callback->deviceConnected;
    *deviceConnected = false;
}

void BLE::notify(int data[], int datalen) // BLE
{
    char sendData[datalen] = {0};

    for (int i = 0; i < datalen; i++)
    {
        sendData[i] = data[i] & 0xff;
    }

    bleTxCharacteristic->setValue((uint8_t *)sendData, sizeof(sendData));
    bleTxCharacteristic->notify();
}

void BLE::setBattery(int percent) // BLE
{
    uint8_t sendData = 255;
    if (percent >= 0 && percent <= 100)
    {
        sendData = static_cast<uint8_t>(percent);
    }
    bleBatteryCharacteristic->setValue(&sendData, 1);
}
