#pragma once

// Bluetooth Low Energy
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Bluetooth LE Change Connect State
class ServerCallbacks : public BLEServerCallbacks
{
public:
    ServerCallbacks(bool debug);
    bool debugMode;
    bool deviceConnected = false;
    virtual void onConnect(BLEServer *bleServer)
    {
        if (debugMode)
            Serial.println("BLE Connected");
        deviceConnected = true;
    };
    virtual void onDisconnect(BLEServer *bleServer)
    {
        if (debugMode)
            Serial.println("BLE Disconnected");
        deviceConnected = false;
    }
};

// Bluetooth LE Recive
class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0)
        {
            String cmd = String(rxValue.c_str());
            Serial.print("Received Value: ");
            Serial.println(cmd);
        }
    }
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
    void setup(const char *ADVERTISING_UUID, const char *SERVICE_UUID, const char *CHARACTERISTIC_UUID_RX, const char *CHARACTERISTIC_UUID_TX);
    bool isConnected();
    void startAdvertising();
    void notify(int data[], int datalen);
    void setBattery(int percent);
    ServerCallbacks *callback;
    BLECharacteristic *bleTxCharacteristic;
    BLECharacteristic *bleRxCharacteristic;
    BLECharacteristic *bleBatteryCharacteristic;
    const char *ADVERTISING_UUID;
    const char *SERVICE_UUID;
    const char *CHARACTERISTIC_UUID_RX;
    const char *CHARACTERISTIC_UUID_TX;

    const char *SERVICE_UUID_BATTERY = "180f";
    const char *CHARACTERISTIC_UUID_BATTERY = "2A19";

private:
};