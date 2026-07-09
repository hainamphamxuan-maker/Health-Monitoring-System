/*
 * ============================================================
 * SMART HEALTH SYSTEM - ESP32 Firmware 
 * ============================================================
 */

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#include <OneWire.h>
#include <DallasTemperature.h>

#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─── Cấu hình WiFi & Firebase ────────────────────────────────

// ─── Chân GPIO ───────────────────────────────────────────────
#define ONE_WIRE_BUS  4       
#define SDA_PIN       21      
#define SCL_PIN       22      
#define LED_ALERT     2       

// ─── Cấu hình OLED 0.96" ─────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1      
#define OLED_ADDRESS  0x3C    

// ─── Thông số chu kỳ ─────────────────────────────────────────
#define SEND_INTERVAL       3000
#define TEMP_READ_INTERVAL  2000
#define OLED_UPDATE_MS      500   

// ─── MAX30102 ────────────────────────────────────────────────
#define BUFFER_LENGTH     100
#define FINGER_THRESHOLD  50000

// ─── Ngưỡng cảnh báo ─────────────────────────────────────────
#define TEMP_ALERT_HIGH   38.5f
#define SPO2_ALERT_LOW    90
#define HR_ALERT_HIGH     120
#define HR_ALERT_LOW      50

// ═════════════════════════════════════════════════════════════
FirebaseData   fbdo;
FirebaseData   streamFBDO; 
FirebaseAuth   auth;
FirebaseConfig config;

OneWire          oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

MAX30105 particleSensor;
uint32_t irBuffer[BUFFER_LENGTH];
uint32_t redBuffer[BUFFER_LENGTH];

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ─── Biến trạng thái ─────────────────────────────────────────
float   currentTemp  = 0.0;
int32_t currentHR    = 0;
int8_t  hrValid      = 0;
int32_t currentSpO2  = 0;
int8_t  spo2Valid    = 0;
bool    fingerOn     = false;
bool    isAlerting   = false;

unsigned long lastSendTime   = 0;
unsigned long lastTempTime   = 0;
unsigned long lastOledTime   = 0;

int currentMode = 0; 
String currentName = "Cho thong tin...";
String currentID = "---";
bool needFetchUserInfo = true; 

// ═════════════════════════════════════════════════════════════
//  HÀM LẮNG NGHE SỰ THAY ĐỔI TỪ APP (FIREBASE STREAM)
// ═════════════════════════════════════════════════════════════
void streamCallback(FirebaseStream data) {
  String path = data.dataPath();
  String type = data.dataType();
  
  path.toLowerCase();

  Serial.println("------------------------------------");
  Serial.printf("[STREAM] Nhan lenh - Path: %s | Type: %s\n", path.c_str(), type.c_str());

  if (type == "json" || path == "/") {
    needFetchUserInfo = true;
  } 
  else if (path == "/mode") {
    if (type == "string") currentMode = data.stringData().toInt();
    else currentMode = data.intData();
    
    display.clearDisplay();
    currentTemp = 0.0; currentHR = 0; currentSpO2 = 0;
    hrValid = 0; spo2Valid = 0;
    Serial.printf("=> Đổi chế độ sang: %d\n", currentMode);
  } 
  else if (path == "/name") {
    currentName = data.stringData(); 
  } 
  else if (path == "/id" || path == "/mssv") {
    if (type == "int") currentID = String(data.intData());
    else currentID = data.stringData();
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) Serial.println("[STREAM] Timeout, dang ket noi lai...");
}

// ═════════════════════════════════════════════════════════════
// SETUP
// ═════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  pinMode(LED_ALERT, OUTPUT);
  digitalWrite(LED_ALERT, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);

  initOLED();
  sensors.begin();
  sensors.setResolution(12);

  oledSplash("Init MAX30102...");
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("[MAX30102] ERROR: not found!");
    oledError("MAX30102\nkhong tim thay!");
  } else {
    particleSensor.setup(60, 4, 2, 100, 411, 4096);
    particleSensor.setPulseAmplitudeRed(0x0A);
    particleSensor.setPulseAmplitudeGreen(0);
  }

  oledSplash("Ket noi WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(500);
  }

  oledSplash("Ket noi Firebase...");
  config.api_key                    = API_KEY;
  config.database_url               = DATABASE_URL;
  auth.user.email                   = USER_EMAIL;
  auth.user.password                = USER_PASSWORD;
  config.token_status_callback      = tokenStatusCallback;
  config.max_token_generation_retry = 5;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  t0 = millis();
  while (!Firebase.ready() && millis() - t0 < 15000) delay(500);

  // Lắng nghe Trạm trung chuyển
  if (Firebase.RTDB.beginStream(&streamFBDO, "/Smart_Health_System/Control")) {
    Firebase.RTDB.setStreamCallback(&streamFBDO, streamCallback, streamTimeoutCallback);
  }

  oledSplash("San sang!");
  delay(800);
}

// ═════════════════════════════════════════════════════════════
// LOOP
// ═════════════════════════════════════════════════════════════
void loop() {
  unsigned long now = millis();

  if (needFetchUserInfo && Firebase.ready()) {
    
    if (Firebase.RTDB.get(&fbdo, "/Smart_Health_System/Control/Name") || Firebase.RTDB.get(&fbdo, "/Smart_Health_System/Control/name")) {
      if (fbdo.dataType() == "string") currentName = fbdo.stringData();
      else currentName = String(fbdo.intData());
    } else currentName = "Loi Ten";
    
    // Kéo ID/MSSV
    if (Firebase.RTDB.get(&fbdo, "/Smart_Health_System/Control/ID") || Firebase.RTDB.get(&fbdo, "/Smart_Health_System/Control/id") || Firebase.RTDB.get(&fbdo, "/Smart_Health_System/Control/MSSV")) {
      if (fbdo.dataType() == "string") currentID = fbdo.stringData();
      else currentID = String(fbdo.intData());
    } else currentID = "Loi ID";

    if (Firebase.RTDB.get(&fbdo, "/Smart_Health_System/Control/Mode") || Firebase.RTDB.get(&fbdo, "/Smart_Health_System/Control/mode")) {
      if (fbdo.dataType() == "int") currentMode = fbdo.intData();
      else if (fbdo.dataType() == "string") currentMode = fbdo.stringData().toInt();

      display.clearDisplay();
      currentTemp = 0.0; currentHR = 0; currentSpO2 = 0;
      hrValid = 0; spo2Valid = 0;
    }
    
    needFetchUserInfo = false; 
  }

  if (currentMode == 1) {
    if (now - lastTempTime >= TEMP_READ_INTERVAL) {
      lastTempTime = now;
      readTemperature();
    }
  }

  if (currentMode == 2 || currentMode == 3) {
    readMAX30102();
  }

  if (now - lastOledTime >= OLED_UPDATE_MS) {
    lastOledTime = now;
    updateOLED();
  }

  if (now - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = now;
    sendToFirebase();
    checkAlerts();
  }

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(500);
  }
}

// ═════════════════════════════════════════════════════════════
void readTemperature() {
  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);
  if (t != DEVICE_DISCONNECTED_C && t > 0.0f && t < 60.0f) {
    currentTemp = t;
  }
}

void readMAX30102() {
  uint32_t irValue = particleSensor.getIR();
  fingerOn = (irValue >= FINGER_THRESHOLD);

  if (!fingerOn) {
    currentHR = 0; currentSpO2 = 0;
    hrValid = 0;   spo2Valid   = 0;
    return;
  }

  for (byte i = 25; i < BUFFER_LENGTH; i++) {
    redBuffer[i - 25] = redBuffer[i];
    irBuffer[i - 25]  = irBuffer[i];
  }
  for (byte i = 75; i < BUFFER_LENGTH; i++) {
    while (!particleSensor.available()) particleSensor.check();
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i]  = particleSensor.getIR();
    particleSensor.nextSample();
  }
  maxim_heart_rate_and_oxygen_saturation(
    irBuffer, BUFFER_LENGTH, redBuffer,
    &currentSpO2, &spo2Valid, &currentHR, &hrValid);

  if (hrValid   && (currentHR   < 30 || currentHR   > 220)) hrValid   = 0;
  if (spo2Valid && (currentSpO2 < 70 || currentSpO2 > 100)) spo2Valid = 0;
}

void initOLED() {
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.display();
  }
}

void oledSplash(const char* msg) {
  display.clearDisplay();
  display.fillRect(0, 0, 128, 16, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(8, 4);
  display.print("Smart Health");
  
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 28);
  display.print(msg);
  display.display();
}

void oledError(const char* msg) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("! LOI !");
  display.setCursor(0, 16);
  display.print(msg);
  display.display();
}

void updateOLED() {
  display.clearDisplay();
  switch (currentMode) {
    case 0: drawPageUserInfo(); break; 
    case 1: drawPageTemp();     break;
    case 2: drawPageHR();       break;
    case 3: drawPageSpO2();     break;
    default: drawPageUserInfo(); break;
  }
  display.display();
}

void drawPageUserInfo() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(8, 0);
  display.print("THONG TIN NGUOI DO");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setCursor(0, 18);
  display.print("Ten: ");
  display.print(currentName);
  
  display.setCursor(0, 32);
  display.print("ID : ");
  display.print(currentID);

  display.setCursor(0, 52);
  if ((millis() / 500) % 2 == 0) {
    display.print("-> CHO LENH TU APP..");
  }
}

void drawPageTemp() {
  display.drawRect(8, 6, 8, 28, SSD1306_WHITE);
  display.fillCircle(12, 38, 6, SSD1306_WHITE);
  float pct = constrain((currentTemp - 35.0f) / 10.0f, 0.0f, 1.0f);
  int fill = (int)(pct * 24);
  display.fillRect(9, 7 + (24 - fill), 6, fill, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(28, 4);
  display.print("Nhiet do");

  display.setTextSize(3);
  display.setCursor(26, 18);
  char buf[10];
  dtostrf(currentTemp, 4, 1, buf);
  display.print(buf);

  display.setTextSize(1);
  display.setCursor(104, 20);
  display.print((char)247); 
  display.print("C");

  display.setTextSize(1);
  display.setCursor(0, 50);
  if (currentTemp >= TEMP_ALERT_HIGH) display.print(">> SOT! <<");
  else if (currentTemp >= 37.5f)      display.print("Cao nhe");
  else if (currentTemp >= 36.1f)      display.print("Binh thuong");
  else if (currentTemp > 0.0f)        display.print("Thap");
}

void drawPageHR() {
  display.fillTriangle(12, 24, 0, 12, 24, 12, SSD1306_WHITE);
  display.fillCircle(6,  10, 6, SSD1306_WHITE);
  display.fillCircle(18, 10, 6, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 4);
  display.print("Nhip tim");

  if (!fingerOn) {
    display.setTextSize(1);
    display.setCursor(28, 22);
    display.print("Dat ngon tay");
    display.setCursor(28, 34);
    display.print("len cam bien");
  } else if (hrValid == 1) {
    display.setTextSize(3);
    display.setCursor(30, 18);
    display.print(currentHR);
    display.setTextSize(1);
    display.setCursor(88, 28);
    display.print("BPM");
  } else {
    display.setTextSize(2);
    display.setCursor(30, 20);
    display.print("---");
    display.setTextSize(1);
    display.setCursor(28, 42);
    display.print("Dang tinh...");
  }
}

void drawPageSpO2() {
  display.fillTriangle(12, 30, 4, 18, 20, 18, SSD1306_WHITE);
  display.fillCircle(12, 16, 8, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 4);
  display.print("SpO2");

  if (!fingerOn) {
    display.setTextSize(1);
    display.setCursor(28, 22);
    display.print("Dat ngon tay");
  } else if (spo2Valid == 1) {
    display.setTextSize(3);
    display.setCursor(28, 18);
    display.print(currentSpO2);
    display.setTextSize(2);
    display.setCursor(94, 22);
    display.print(" %");
  } else {
    display.setTextSize(2);
    display.setCursor(30, 20);
    display.print("---");
  }
}

void sendToFirebase() {
  if (!Firebase.ready()) return;

  if (currentMode == 1) {
    Firebase.RTDB.setDouble(&fbdo, "Smart_Health_System/Live_Data/NhietDo", (double)currentTemp);
  }
  else if (currentMode == 2 || currentMode == 3) {
    int hrSend   = (hrValid   == 1) ? (int)currentHR   : 0;
    int spo2Send = (spo2Valid == 1) ? (int)currentSpO2 : 0;
    
    Firebase.RTDB.setInt(&fbdo, "Smart_Health_System/Live_Data/NhipTim", hrSend);
    Firebase.RTDB.setInt(&fbdo, "Smart_Health_System/Live_Data/SpO2", spo2Send);
  }
}

void checkAlerts() {
  isAlerting = false;
  if (currentMode == 1 && currentTemp >= TEMP_ALERT_HIGH) isAlerting = true;
  if (currentMode == 3 && spo2Valid && currentSpO2 < SPO2_ALERT_LOW) isAlerting = true;
  if (currentMode == 2 && hrValid && (currentHR > HR_ALERT_HIGH || currentHR < HR_ALERT_LOW)) isAlerting = true;
  digitalWrite(LED_ALERT, isAlerting ? HIGH : LOW);
}