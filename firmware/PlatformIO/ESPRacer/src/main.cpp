#include <Arduino.h>
#include <math.h>
#include <Wire.h>

#include <Preferences.h>

// Bluetooth Low Energy
#include "BLE/BLE.h"

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))

#define debugMode true

/*========== Version ==========*/
#define FIRMWARE_VER "F1.00"
#define HARDWARE_VER "H1.00"

/*========== Class ==========*/
Preferences pref;

/*========== Hardware Functions ==========*/
#define MOTOR_CH 0b10 // モーターCh1
#define SERVO_CH 0b0001 // サーボ Ch0
#define PORT_CH 0b0011 // LEDポート Ch0, Ch1

/*========== I/O (ESP32-C3) ==========*/
#define STATUS_LED_PIN  10
#define SERVO_PIN  4 // 6
#define MOTOR_A_PIN 1
#define MOTOR_B_PIN 3
#define PORT_0_PIN  5 // 4
#define PORT_1_PIN  5 
#define BATT_VOL_PIN  0

#define CHANNEL_STATUS_LED_PIN 0
#define CHANNEL_SERVO_PIN 1
#define CHANNEL_MOTOR_A_PIN 2
#define CHANNEL_MOTOR_B_PIN 3

#define SERVO_FREQUENCY 50 // 50 [Hz] = 20 [ms] / 1周期
#define SERVO_RESOLUTION 12 // 12bit=4096
#define SERVO_TIME_MIN 102 // 0.5 [ms] / 20 [ms] * 4096 = 102.4
#define SERVO_TIME_MAX 492 // 2.4 [ms] / 20 [ms] * 4096 = 491.52  491.52-102.4=389.12 > 256 (OK)

/*========== BLE UUID ==========*/
#define ADVERTISING_UUID    "389CAAF0-843F-4D3B-959D-C954CCE14655"
#define SERVICE_UUID        "389CAAF0-843F-4D3B-959D-C954CCE14655"
#define GET_BATTERY_VOLTAGE "389CAAF1-843F-4D3B-959D-C954CCE14655"
#define SET_MOTOR_PWM       "389CAAF2-843F-4D3B-959D-C954CCE14655"
#define SET_PORT_OUT        "389CAAF3-843F-4D3B-959D-C954CCE14655"
#define SET_SERVO_POSITION  "389CAAF4-843F-4D3B-959D-C954CCE14655"
#define BURST_COMMAND       "389CAAF5-843F-4D3B-959D-C954CCE14655"
#define GET_FUNCTIONS       "389CAAFF-843F-4D3B-959D-C954CCE14655"

/*========== BLE CharaCharacteristic ==========*/
BLECharacteristic *GET_BATTERY_VOLTAGE_CHARA;
BLECharacteristic *SET_MOTOR_PWM_CHARA;
BLECharacteristic *SET_PORT_OUT_CHARA;
BLECharacteristic *SET_SERVO_POSITION_CHARA;
BLECharacteristic *BURST_COMMAND_CHARA;
BLECharacteristic *GET_FUNCTIONS_CHARA;

BLE RacerBLE = BLE("ESPRacer", true);

/*========== Parameters ==========*/
const int deviceIdLen = 10;
const int sendFreqency = 50; // [ms]
const bool motorInv = false;
const bool servoInv = false;

/*========== Private Variable ==========*/
bool deviceConnectedFlag = false;
int lastSendTime = 0;
int ledBlinkTime = 0;
// unsigned char motor_power = 127;
// unsigned char servo_power = 127;
// bool port0_led = false;
// bool port1_led = false;

void HardwareReset();

void SetupRacer()
{
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(SERVO_PIN,OUTPUT);
  pinMode(MOTOR_A_PIN, OUTPUT);
  pinMode(MOTOR_B_PIN, OUTPUT);
  pinMode(PORT_0_PIN, OUTPUT);
  pinMode(PORT_1_PIN, OUTPUT);
  pinMode(BATT_VOL_PIN, INPUT);

  // pwmの設定 channel,周波数,解像度
  ledcSetup(CHANNEL_STATUS_LED_PIN, 1000, 8);
  ledcSetup(CHANNEL_SERVO_PIN, SERVO_FREQUENCY, SERVO_RESOLUTION);
  ledcSetup(CHANNEL_MOTOR_A_PIN, 1000, 7);
  ledcSetup(CHANNEL_MOTOR_B_PIN, 1000, 7);
  // ピンをチャンネルに接続
  ledcAttachPin(STATUS_LED_PIN, CHANNEL_STATUS_LED_PIN);
  ledcAttachPin(SERVO_PIN, CHANNEL_SERVO_PIN);
  ledcAttachPin(MOTOR_A_PIN, CHANNEL_MOTOR_A_PIN);
  ledcAttachPin(MOTOR_B_PIN, CHANNEL_MOTOR_B_PIN);

  ledcWrite(CHANNEL_STATUS_LED_PIN, 255);

  HardwareReset();
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

void onMotorPWMReceived(const std::string& value);
void onPortOUTReceived(const std::string& value);
void onServoPWMReceived(const std::string& value);
void onBurstCommandReceived(const std::string& value);

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
  RacerBLE.firmwareVersion = FIRMWARE_VER;
  RacerBLE.hardwareVersion = HARDWARE_VER;
  RacerBLE.manufacturerName = "PLEN Project";

  RacerBLE.setup(ADVERTISING_UUID, SERVICE_UUID);
  RacerBLE.addDeviceInformationService();
  GET_BATTERY_VOLTAGE_CHARA = RacerBLE.addCharacteristic(GET_BATTERY_VOLTAGE, BLECharacteristic::PROPERTY_READ);
  SET_MOTOR_PWM_CHARA = RacerBLE.addCharacteristic(SET_MOTOR_PWM, BLECharacteristic::PROPERTY_WRITE_NR);
  SET_PORT_OUT_CHARA = RacerBLE.addCharacteristic(SET_PORT_OUT, BLECharacteristic::PROPERTY_WRITE_NR);
  SET_SERVO_POSITION_CHARA = RacerBLE.addCharacteristic(SET_SERVO_POSITION, BLECharacteristic::PROPERTY_WRITE_NR);
  BURST_COMMAND_CHARA = RacerBLE.addCharacteristic(BURST_COMMAND, BLECharacteristic::PROPERTY_WRITE_NR);
  GET_FUNCTIONS_CHARA = RacerBLE.addCharacteristic(GET_FUNCTIONS, BLECharacteristic::PROPERTY_READ);
  RacerBLE.startAdvertising();

  uint8_t byte0 = MOTOR_CH;
  uint8_t byte1 = (PORT_CH << 4) | SERVO_CH;
  uint16_t value = (byte1 << 8) | byte0;
  bleSetUint16Value(GET_FUNCTIONS_CHARA, value);

  RacerBLE.setCallback(SET_MOTOR_PWM_CHARA, onMotorPWMReceived, debugMode);
  RacerBLE.setCallback(SET_PORT_OUT_CHARA, onPortOUTReceived, debugMode);
  RacerBLE.setCallback(SET_SERVO_POSITION_CHARA, onServoPWMReceived, debugMode);
  RacerBLE.setCallback(BURST_COMMAND_CHARA, onBurstCommandReceived, debugMode);
}

void setMotorPWM(unsigned char motorPower) {
  const int neutral = 127;

  if (motorPower == neutral) {
    // 停止（両ブレーキ）
    ledcWrite(CHANNEL_MOTOR_A_PIN, 0);
    ledcWrite(CHANNEL_MOTOR_B_PIN, 0);
    return;
  }

  int power = motorPower - neutral;

  if (motorInv) {
    power *= -1;
  }

  uint8_t duty = abs(power);
  if(duty>127){
    duty = 127;
  }

  if (power > 0) {
    // 正転
    ledcWrite(CHANNEL_MOTOR_A_PIN, duty);
    ledcWrite(CHANNEL_MOTOR_B_PIN, 0);
  } else {
    // 逆転
    ledcWrite(CHANNEL_MOTOR_A_PIN, 0);
    ledcWrite(CHANNEL_MOTOR_B_PIN, duty);
  }
}

void setPortOUT(unsigned char port_out) {
  digitalWrite(PORT_0_PIN, (port_out & 0b0001) ? HIGH : LOW);  // bit 0 → PORT_0
  digitalWrite(PORT_1_PIN, (port_out & 0b0010) ? HIGH : LOW);  // bit 1 → PORT_1
}


void setServoPWM(unsigned char pwmValue) {
  if(servoInv){
    pwmValue = 255 - pwmValue;
  }
  ledcWrite(CHANNEL_SERVO_PIN, map(pwmValue, 0, 255, SERVO_TIME_MIN, SERVO_TIME_MAX));
}

void onMotorPWMReceived(const std::string& value) {
    if (value.length() < 1) return;

    uint8_t motor_power = static_cast<uint8_t>(value[1]);
    setMotorPWM(motor_power);
}

void onPortOUTReceived(const std::string& value) {
    if (value.length() < 1) return;

    uint8_t port_out = static_cast<uint8_t>(value[0]);
    setPortOUT(port_out);
}

void onServoPWMReceived(const std::string& value) {
    if (value.length() < 1) return;

    uint8_t servo_angle = static_cast<uint8_t>(value[1]);
    setServoPWM(servo_angle);
}

std::string lastBurstCommand = "";
void onBurstCommandReceived(const std::string& value) {
  if (value.length() < 7) return;
  if (value==lastBurstCommand) return;
  setMotorPWM(value[1]);
  setPortOUT(value[2]);
  setServoPWM(value[3]);
  lastBurstCommand = value;
}

void HardwareReset(){
  //　モータ停止
  setMotorPWM(127);
  // サーボ脱力
  ledcWrite(CHANNEL_SERVO_PIN, 0);
  // LED消灯
  setPortOUT(0);
  lastBurstCommand = "";
}

void onConnect()
{
}

void onDisconnect()
{
  HardwareReset();

  //　再接続
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
    ledcWrite(CHANNEL_STATUS_LED_PIN, max);
  }
  else
  {
    ledcWrite(CHANNEL_STATUS_LED_PIN, min);
  }
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
    LEDBlink(millis() - ledBlinkTime, 250, 0.5, 255, 0);
  }
  else
  {
    if (ledBlinkTime == 0)
      ledBlinkTime = millis();
    LEDBlink(millis() - ledBlinkTime, 1000, 0.5, 255, 0);
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
  uint16_t batteryMv = (uint16_t)(analogReadMilliVolts(BATT_VOL_PIN) * 3.0);
  bleSetUint16Value(GET_BATTERY_VOLTAGE_CHARA, batteryMv);
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