#include "Arduino.h"
#include "BLE.h"

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))

void bleSetUint16Value(BLECharacteristic* characteristic, uint16_t value) {
    uint8_t data[2] = { (uint8_t)(value & 0xFF), (uint8_t)(value >> 8) };
    characteristic->setValue(data, 2);
}

ServerCallbacks::ServerCallbacks(bool debug) {
    debugMode = debug;
}

void ServerCallbacks::onConnect(BLEServer* bleServer) {
    if (debugMode)
        Serial.println("BLE Connected");
    deviceConnected = true;
}

void ServerCallbacks::onDisconnect(BLEServer* bleServer) {
    if (debugMode)
        Serial.println("BLE Disconnected");
    deviceConnected = false;
}

CharacteristicCallbacks::CharacteristicCallbacks(BLECallbackFunc callback, bool debug)
    : debugMode(debug), callbackFunc(callback) {}

void CharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
    Serial.print("Received Characteristic: ");
    Serial.println(pCharacteristic->getUUID().toString().c_str());

    std::string rxValue = pCharacteristic->getValue();
    if (!rxValue.empty()) {
        String cmd = String(rxValue.c_str());
        if (debugMode) {
            Serial.print("Received Value: ");
            for (size_t i = 0; i < rxValue.length(); i++) {
                Serial.printf("%02X ", (uint8_t)rxValue[i]);
            }
            Serial.println();
        }
        if (callbackFunc) {
            callbackFunc(rxValue);
        }
    }
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

void BLE::setup(const char *ADVERTISING_UUID, const char *SERVICE_UUID)
{
    this->ADVERTISING_UUID = ADVERTISING_UUID;
    this->SERVICE_UUID = SERVICE_UUID;

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
    bleServer = BLEDevice::createServer();
    serverCallback = new ServerCallbacks(debugMode);
    bleServer->setCallbacks(serverCallback);

    // Create the BLE Service
    bleService = bleServer->createService(SERVICE_UUID);
    BLEAdvertising *bleAdvertising = bleServer->getAdvertising(); // アドバタイズオブジェクトを取得
    bleAdvertising->addServiceUUID(ADVERTISING_UUID);

    // Pointer Set
    deviceConnected = &serverCallback->deviceConnected;
    *deviceConnected = false;
}

void BLE::addDeviceInformationService(){
    BLEService* deviceInfoService = bleServer->createService("180A");

    const char* uuids[] = { "2A24", "2A25", "2A26", "2A27", "2A29" };
    const char* values[] = { deviceName, deviceId, firmwareVersion, hardwareVersion, manufacturerName };

    for (size_t i = 0; i < ARRAY_LENGTH(uuids); ++i) {
    if (values[i] && values[i][0] != '\0') {
        BLECharacteristic* c = deviceInfoService->createCharacteristic(uuids[i], BLECharacteristic::PROPERTY_READ);
        c->setValue(values[i]);
    }
    }
    deviceInfoService->start();
}

BLECharacteristic* BLE::addCharacteristic(const char* characteristic_UUID, uint32_t properties) {
    BLECharacteristic* characteristic = bleService->createCharacteristic(characteristic_UUID, properties);
    return characteristic;
}

void BLE::setCallback(BLECharacteristic* characteristic, BLECallbackFunc callback, bool debug) {
    characteristic->setCallbacks(new CharacteristicCallbacks(callback, debug));
}

void BLE::startAdvertising()
{
    bleService->start();
    BLEDevice::startAdvertising();
    if (debugMode)
        Serial.println("BLE Advertising");
}

// void BLE::notify(int data[], int datalen) // BLE
// {
//     char sendData[datalen] = {0};

//     for (int i = 0; i < datalen; i++)
//     {
//         sendData[i] = data[i] & 0xff;
//     }

//     bleTxCharacteristic->setValue((uint8_t *)sendData, sizeof(sendData));
//     bleTxCharacteristic->notify();
// }

// void BLE::setBattery(int percent) // BLE
// {
//     uint8_t sendData = 255;
//     if (percent >= 0 && percent <= 100)
//     {
//         sendData = static_cast<uint8_t>(percent);
//     }
//     bleBatteryCharacteristic->setValue(&sendData, 1);
// }
