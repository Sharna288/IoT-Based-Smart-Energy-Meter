// ===== BLYNK INFO =====
#define BLYNK_TEMPLATE_ID   "TMPL6RxUTT0rj"
#define BLYNK_TEMPLATE_NAME "SmartMeterESP8266"
#define BLYNK_AUTH_TOKEN    "sOsuMGt64DRjEQEhJepg4MYMmrMFcNMR"

// ===== LIBRARIES =====
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <BlynkSimpleEsp8266.h>
#include <LiquidCrystal_I2C.h>
#include <EmonLib.h>

// ===== LCD =====
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===== CURRENT SENSOR =====
EnergyMonitor emonCurrent;
const double I_CALIB = 60.6;  
double zeroOffset = 0;

// ===== RELAY & SWITCH =====
#define RELAY_STATE_ON  LOW
#define RELAY_STATE_OFF HIGH

#define RELAY_BULB  D3
#define RELAY_FAN   D7
#define SWITCH_BULB D5
#define SWITCH_FAN  D6

int bulbRemote = 0;
int fanRemote  = 0;

BlynkTimer timer;

// ===== ZERO OFFSET CALIBRATION =====
void calibrateZero() {
  double sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += emonCurrent.calcIrms(1480);
    delay(50);
  }
  zeroOffset = sum / 20.0;
}

// ===== BLYNK CONTROL =====
BLYNK_WRITE(V0) { bulbRemote = param.asInt(); }
BLYNK_WRITE(V1) { fanRemote  = param.asInt(); }

// ===== RELAY CONTROL =====
void updateRelays() {
  bool manualBulb = (digitalRead(SWITCH_BULB) == LOW);
  bool manualFan  = (digitalRead(SWITCH_FAN)  == LOW);

  int finalBulb = RELAY_STATE_OFF;
  int finalFan  = RELAY_STATE_OFF;

  if (manualBulb || bulbRemote == 1) finalBulb = RELAY_STATE_ON;
  if (manualFan  || fanRemote  == 1) finalFan  = RELAY_STATE_ON;

  digitalWrite(RELAY_BULB, finalBulb);
  digitalWrite(RELAY_FAN,  finalFan);
}

// ===== SENSOR READING =====
void sendSensorData() {
  double Iraw = emonCurrent.calcIrms(1480) - zeroOffset;
  if (Iraw < 0) Iraw = 0;

  double Vraw = 220.0;     // Fixed Voltage
  double P = Vraw * Iraw;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("V:"); lcd.print(Vraw);
  lcd.print(" I:"); lcd.print(Iraw, 2);

  lcd.setCursor(0, 1);
  lcd.print("P:"); lcd.print(P, 1); lcd.print("W");

  Blynk.virtualWrite(V2, Vraw);
  Blynk.virtualWrite(V3, Iraw);
  Blynk.virtualWrite(V4, P);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.print("Smart Meter");

  pinMode(RELAY_BULB, OUTPUT);
  pinMode(RELAY_FAN, OUTPUT);
  pinMode(SWITCH_BULB, INPUT_PULLUP);
  pinMode(SWITCH_FAN, INPUT_PULLUP);

  digitalWrite(RELAY_BULB, RELAY_STATE_OFF);
  digitalWrite(RELAY_FAN,  RELAY_STATE_OFF);

  WiFiManager wifiManager;
  wifiManager.autoConnect("SmartMeter-Setup");

  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  emonCurrent.current(A0, I_CALIB);
  calibrateZero();

  timer.setInterval(2000L, sendSensorData);
  timer.setInterval(500L, updateRelays);
}

// ===== LOOP =====
void loop() {
  Blynk.run();
  timer.run();
}
