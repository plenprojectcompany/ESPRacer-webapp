#pragma once

// Bluetooth LE initialize
class Battery
{
public:
    Battery();
    void InitBatteryVoltage();
    int VoltageToPercent(double voltage);

private:
    int lastPercent = 50;
    double batteryVoltage[101];
};