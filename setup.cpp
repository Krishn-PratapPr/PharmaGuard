#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include "ThingSpeak.h"

// ─── USER CONFIG ───
const char* WIFI_SSID      = "POCO-X7";
const char* WIFI_PASSWORD  = "Aman1234";

unsigned long THINGSPEAK_CHANNEL_ID = 3365330;
const char*   THINGSPEAK_WRITE_KEY  = "D43UICTVR8VTOE7J";

const float TEMP_MIN = 20.0;
const float TEMP_MAX = 35.0;

// ─── PINS ───
#define ONE_WIRE_BUS  15   // Pin 15 is safer than Pin 4 for OneWire
#define BUZZER_PIN    2
#define LED_PIN       5

// ─── DISPLAY ───
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C   // Your specific address

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ─── OBJECTS ───
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiClient wifiClient;

// ─── GLOBALS ───
float currentTemp = 0.0;
bool alertActive = false;
bool wifiConnected = false;
unsigned long lastRead = 0;
unsigned long lastUpload = 0;
int uploadCount = 0;

// ─────────────────────────────
void setup() {
  Serial.begin(115200);
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // Ensure pins are LOW at start
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  // Initialize I2C for OLED
  Wire.begin(21, 22);

  // Initialize OLED with a fallback check
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("OLED 0x3C failed, trying ..."));
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println(F("OLED hardware not found!"));
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 10);
  display.println("PharmaGuard v1.0");
  display.setCursor(10, 30);
  display.println("System Starting...");
  display.display();

  // Initialize Temperature Sensor
  sensors.begin();
  delay(500); 
  
  Serial.print("Checking Sensors...");
  int found = sensors.getDeviceCount();
  Serial.print(found);
  Serial.println(" found.");

  // Connect to WiFi
  connectWiFi();
  
  // Initialize ThingSpeak
  ThingSpeak.begin(wifiClient);
  
  display.clearDisplay();
}

// ─────────────────────────────
void loop() {
  unsigned long currentMillis = millis();

  // 1. Logic: Read sensor every 5 seconds
  if (currentMillis - lastRead >= 1000) {
    lastRead = currentMillis;
    readTemperature();
    checkAlert();
    updateDisplay();
    
    // Debug to Serial
    Serial.print("Temp: ");
    Serial.print(currentTemp);
    Serial.print("C | WiFi: ");
    Serial.println(wifiConnected ? "Connected" : "Lost");
  }

  // 2. Logic: Upload to ThingSpeak every 20 seconds
  if (currentMillis - lastUpload >= 20000) {
    lastUpload = currentMillis;
    uploadData();
  }
  
  // 3. Keep WiFi alive
  if (WiFi.status() != WL_CONNECTED && (currentMillis % 10000 == 0)) {
    wifiConnected = false;
    connectWiFi();
  }
}

// ─────────────────────────────
void readTemperature() {
  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);

  // Ignore error codes (-127, 85) so they don't trigger the buzzer
  if (t == DEVICE_DISCONNECTED_C || t == 85.00) {
    Serial.println("SENSOR ERROR: Check Wires!");
    return; 
  }

  currentTemp = t;
}

// ─────────────────────────────
void checkAlert() {
  // Trigger if temp is outside range, but NOT if it's exactly 0 (sensor error)
  if ((currentTemp < TEMP_MIN || currentTemp > TEMP_MAX) && currentTemp != 0.0) {
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    alertActive = true;
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    alertActive = false;
  }
}

// ─────────────────────────────
void updateDisplay() {
  display.clearDisplay();

  // Top Bar: WiFi status and Upload Count
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("WiFi: ");
  display.print(wifiConnected ? "OK" : "!!");
  display.setCursor(80, 0);
  display.print("UP:");
  display.print(uploadCount);

  // Middle: Big Temperature
  display.setTextSize(3);
  display.setCursor(15, 20);
  if (currentTemp == 0.0) {
    display.print("WAIT");
  } else {
    display.print(currentTemp, 1);
    display.print("C");
  }

  // Bottom: Status Message
  display.setTextSize(1);
  display.setCursor(0, 55);
  if (alertActive) {
    display.print(">> BREACH DETECTED <<");
  } else {
    display.print("Status: SAFE");
  }

  display.display();
}

// ─────────────────────────────
void uploadData() {
  if (!wifiConnected || currentTemp == 0.0) return;

  ThingSpeak.setField(1, currentTemp);
  ThingSpeak.setField(2, alertActive ? 1 : 0);

  int response = ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID, THINGSPEAK_WRITE_KEY);

  if (response == 200) {
    uploadCount++;
    Serial.println("ThingSpeak: Upload Successful");
  } else {
    Serial.print("ThingSpeak: Error ");
    Serial.println(response);
  }
}

// ─────────────────────────────
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  
  Serial.print("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println(" Connected!");
  } else {
    wifiConnected = false;
    Serial.println(" Failed.");
  }
}