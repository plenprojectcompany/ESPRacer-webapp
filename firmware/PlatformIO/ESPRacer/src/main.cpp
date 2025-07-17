#include <Arduino.h>
#include <math.h>
#include <Wire.h>

#include <Preferences.h>

#ifdef _WIN32
#else // _WIN32

// Bluetooth Low Energy
#include "BLE/BLE.h"

// Battery
#include "Battery/Battery.h"
#endif // _WIN32

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

/*

*/

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))

bool debugMode = true;

Preferences pref;

/*========== BLE UUID ==========*/
#define ADVERTISING_UUID        "389CAAF0-843F-4D3B-959D-C954CCE14655"
#define SERVICE_UUID        "389CAAF0-843F-4D3B-959D-C954CCE14655"
#define GET_BATTERY_VOLTAGE "389CAAF1-843F-4D3B-959D-C954CCE14655"
#define SET_MOTOR_PWM       "389CAAF2-843F-4D3B-959D-C954CCE14655"
#define SET_PORT_OUT        "389CAAF3-843F-4D3B-959D-C954CCE14655"
#define SET_SERVO_POSITION  "389CAAF4-843F-4D3B-959D-C954CCE14655"
#define BURST_COMMAND       "389CAAF5-843F-4D3B-959D-C954CCE14655"
#define GET_FUNCTIONS       "389CAAFF-843F-4D3B-959D-C954CCE14655"

/*========== I/O (Seeed Xiao ESP32-S3) ==========*/
#define power_led  1
#define servo_pin  2
#define motorA_pin 3
#define motorB_pin 4
#define port0_led  5
#define port1_led  6
#define batt_vol  6

bool deviceConnectedFlag = false;

const int deviceIdLen = 12;

BLE RacerBLE = BLE("ESPRacer", true);
Battery RacerBattery = Battery();

const int sendFreqency = 50; // [ms]
int lastSendTime = 0;
const int channel_power_led = 0;
int ledBlinkTime = 0;

void SetupRacer()
{
  // // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector

  pinMode(power_led, OUTPUT);
  pinMode(servo_pin, OUTPUT);
  pinMode(motorA_pin, OUTPUT);
  pinMode(motorB_pin, OUTPUT);
  pinMode(port0_led, OUTPUT);
  pinMode(port1_led, OUTPUT);
  pinMode(batt_vol, INPUT);

  // pwmの設定。最初の引数がchannel,次が周波数,最後が解像度（ここでは8bit = 256段階）
  ledcSetup(0, 12800, 8);
  // ピンをチャンネルに接続
  ledcAttachPin(power_led, channel_power_led);

  ledcWrite(channel_power_led, 255);
}

String GenerateID(int len)
{
  char id[len];

  for (int i = 0; i < len; i++)
  {
    int random = esp_random() % 36; // 0~35の乱数 (数字10個、アルファベット26個)

    if (random < 10)
    {
      id[i] = random + 48; // 数字
    }
    else
    {
      id[i] = random + 55; // アルファベット
    }
  }

  return String(id);
}

void SetupBLE()
{
  pref.begin("My-Prof", false);
  String device_id_str = pref.getString("device_id");

  char device_id[32];
  if (device_id_str.length() == deviceIdLen)
  {
    strcpy(device_id, device_id_str.c_str());
    device_id[deviceIdLen] = '\0';
    if (debugMode)
      Serial.println("device_id:" + String(device_id));
  }
  else
  {
    strcpy(device_id, GenerateID(deviceIdLen).c_str());
    device_id[deviceIdLen] = '\0';
    pref.putString("device_id", String(device_id));
    if (debugMode)
      Serial.println("Generate device_id:" + String(device_id));
  }
  pref.end();
  RacerBLE.deviceId = device_id;

  RacerBLE.setup(ADVERTISING_UUID, SERVICE_UUID, BURST_COMMAND, GET_FUNCTIONS);
}

void onConnect()
{
}

void onDisconnect()
{
  RacerBLE.startAdvertising();
}

int AccelConvert(float accel)
{
  return (int)(accel * 100);
}

int AngleConvert(float angle)
{
  return (int)(angle * 100);
}

void LEDBlink(int msec, int cycle = 1000, double duty = 0.5, int max = 255, int min = 0)
{
  if ((double)(msec % cycle) / (double)cycle < duty)
  {
    ledcWrite(channel_power_led, max);
  }
  else
  {
    ledcWrite(channel_power_led, min);
  }
}

void GetDriveData()
{

}

void BLEupdate()
{
  if (deviceConnectedFlag != RacerBLE.isConnected())
  {
    deviceConnectedFlag = RacerBLE.isConnected();
    if (deviceConnectedFlag)
    {
      onConnect();
    }
    else
    {
      onDisconnect();
    }
  }
  
  if (deviceConnectedFlag)
  {
    ledBlinkTime = 0;
    ledcWrite(channel_power_led, 255);
    GetDriveData();
  }
  else
  {
    if (ledBlinkTime == 0)
      ledBlinkTime = millis();
    LEDBlink(millis() - ledBlinkTime, 2000, 0.9, 255, 5);
  }
}

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    delay(10); // wait for serial port to open!

  Serial.println("ESPRacer 2025");
  Serial.println("Copyright 2025 PLEN Project");

  SetupRacer();
  SetupBLE();
}

void PowerManagement()
{
  float batteryVoltage = analogReadMilliVolts(batt_vol) / 1000.0 * 2.0;
  int percent = RacerBattery.VoltageToPercent(batteryVoltage);
  RacerBLE.setBattery(percent);
}

void loop()
{
  PowerManagement();
  BLEupdate();

  while (millis() - lastSendTime < sendFreqency)
  {
    delay(1);
  }

  lastSendTime = millis();
}
