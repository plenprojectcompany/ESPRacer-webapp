// update: 2025/04/30  v3 コア完全対応 + タイマー再設計 + 広告名明示

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

/*========== BLE UUID ==========*/
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
#define motor0_pin 3
#define motor1_pin 4
#define port0_led  5
#define port1_led  6

/*========== globals ==========*/
BLECharacteristic *pCharacteristic1;
BLECharacteristic *pCharacteristic2;
BLECharacteristic *pCharacteristic3;
BLECharacteristic *pCharacteristic4;
BLECharacteristic *pCharacteristic5;
BLECharacteristic *pCharacteristic6;
BLEAdvertising   *pAdvertising;

unsigned char motor_power = 127;
unsigned char servo_power = 127;

volatile uint32_t interruptCounter = 0;     // 割り込みカウンタ
hw_timer_t *timer = nullptr;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
static int light = LOW;                     // LED トグル用

/*========== timer ISR ==========*/
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  ++interruptCounter;
  portEXIT_CRITICAL_ISR(&timerMux);
}

/*========== BLE Callbacks ==========*/
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value.length() < 4) return;         // データ不足時は無視

    motor_power = value[1];                 // DC モータ
    if (value[2] == 3) {                    // ポート LED
      digitalWrite(port0_led, HIGH);
      digitalWrite(port1_led, HIGH);
    } else {
      digitalWrite(port0_led, LOW);
      digitalWrite(port1_led, LOW);
    }
    servo_power = value[3];                 // サーボ
  }
};

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer*) override {
    Serial.println("connect.");
    motor_power = servo_power = 127;
    timerAlarm(timer, 100'000, true, 0);    // LED：0.1 s 周期
  }
  void onDisconnect(BLEServer*) override {
    Serial.println("disconnect.");
    motor_power = servo_power = 127;
    timerAlarm(timer, 500'000, true, 0);    // LED：0.5 s 周期
  }
};

/*========== SETUP ==========*/
void setup() {
  Serial.begin(115200);
  Serial.print("starting...");

  pinMode(servo_pin,  OUTPUT);
  pinMode(motor0_pin, OUTPUT);
  pinMode(motor1_pin, OUTPUT);
  pinMode(power_led,  OUTPUT);
  pinMode(port0_led,  OUTPUT);
  pinMode(port1_led,  OUTPUT);

  /*----- ハードウェアタイマー -----*/
  timer = timerBegin(1'000'000);               // 1 MHz (1 µs 分解能)
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 500'000, true, 0);          // 初期：0.5 s 周期

  /*----- BLE 初期化 -----*/
  BLEDevice::init("ESP Racer");
  BLEServer  *pServer  = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic1 = pService->createCharacteristic(GET_BATTERY_VOLTAGE, BLECharacteristic::PROPERTY_READ);
  pCharacteristic2 = pService->createCharacteristic(SET_MOTOR_PWM,       BLECharacteristic::PROPERTY_WRITE_NR);
  pCharacteristic3 = pService->createCharacteristic(SET_PORT_OUT,        BLECharacteristic::PROPERTY_WRITE_NR);
  pCharacteristic4 = pService->createCharacteristic(SET_SERVO_POSITION,  BLECharacteristic::PROPERTY_WRITE_NR);
  pCharacteristic5 = pService->createCharacteristic(BURST_COMMAND,       BLECharacteristic::PROPERTY_WRITE_NR);
  pCharacteristic6 = pService->createCharacteristic(GET_FUNCTIONS,       BLECharacteristic::PROPERTY_READ);

  pCharacteristic5->setCallbacks(new MyCallbacks());
  pServer->setCallbacks(new ServerCallbacks());

  pService->start();

  /*----- 広告データに名前を明示 -----*/
  pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);          // 名前はスキャン応答へ
  BLEAdvertisementData scanData;
  scanData.setName("ESP Racer");                // ブラウザ等に表示される名前
  pAdvertising->setScanResponseData(scanData);
  pAdvertising->start();

  Serial.println("done.");
}

/*========== LOOP ==========*/
void loop() {
  constexpr unsigned int control_phase = 16'000;                 // 16 ms
  unsigned int servo_on = 1'500 + (servo_power - 127) * 4;       // 1012–1992 µs
  unsigned int motor_on = control_phase * abs(motor_power - 127) / 200;

  /*----- 電源 LED 点滅 -----*/
  if (interruptCounter > 0) {
    portENTER_CRITICAL(&timerMux);
    --interruptCounter;
    portEXIT_CRITICAL(&timerMux);

    light ^= 1;
    digitalWrite(power_led, light);
  }

  /*----- DC モータ & サーボ制御 -----*/
  if (motor_on > servo_on) {                 // モータ優先
    if (motor_power > 127) {                 // 正転
      digitalWrite(motor0_pin, HIGH);
      digitalWrite(motor1_pin, LOW);
      motor_on += control_phase / 4;
    } else if (motor_power < 127) {          // 逆転
      digitalWrite(motor0_pin, LOW);
      digitalWrite(motor1_pin, HIGH);
      motor_on += control_phase / 4;
    } else {                                 // 停止
      digitalWrite(motor0_pin, HIGH);
      digitalWrite(motor1_pin, HIGH);
    }
    digitalWrite(servo_pin, HIGH);
    delayMicroseconds(servo_on);
    digitalWrite(servo_pin, LOW);
    delayMicroseconds(motor_on - servo_on);
    digitalWrite(motor0_pin, LOW);
    digitalWrite(motor1_pin, LOW);
    delayMicroseconds(control_phase - motor_on);

  } else {                                   // サーボのみ
    digitalWrite(servo_pin, HIGH);
    delayMicroseconds(servo_on);
    digitalWrite(servo_pin, LOW);
    delayMicroseconds(control_phase - servo_on);
  }
}
