#pragma once

// Bluetooth Low Energy
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

typedef std::function<void(const std::string&)> BLECallbackFunc;

void bleSetUint16Value(BLECharacteristic* characteristic, uint16_t value);

// Bluetooth LE Change Connect State
class ServerCallbacks : public BLEServerCallbacks {
public:
    explicit ServerCallbacks(bool debug = false);

    bool debugMode;
    bool deviceConnected = false;

    void onConnect(BLEServer* bleServer) override;
    void onDisconnect(BLEServer* bleServer) override;
};

// Bluetooth LE Recive
class CharacteristicCallbacks : public BLECharacteristicCallbacks {
public:
    CharacteristicCallbacks(BLECallbackFunc callback, bool debug = false);
    void onWrite(BLECharacteristic *pCharacteristic) override;

private:
    bool debugMode;
    BLECallbackFunc callbackFunc;
};


// Bluetooth LE initialize
class BLE
{
public:
    BLE(const char *deviceName, bool debugMode);
    bool debugMode;
    bool *deviceConnected;
    const char *deviceName;
    const char *deviceId = "";
    const char *firmwareVersion = "";
    const char *hardwareVersion = "";
    const char *manufacturerName = "";
    BLEServer *bleServer;
    BLEService *bleService;
    void addDeviceInformationService();
    void setup(const char *ADVERTISING_UUID, const char *SERVICE_UUID);
    BLECharacteristic *addCharacteristic(const char *characteristic_UUID, uint32_t properties);
    void setCallback(BLECharacteristic* characteristic, BLECallbackFunc callback, bool debug = false);
    void startAdvertising();
    bool isConnected();
    void notify(int data[], int datalen);
    void setBattery(int percent);
    ServerCallbacks *serverCallback;
    const char *ADVERTISING_UUID;
    const char *SERVICE_UUID;

private:
};