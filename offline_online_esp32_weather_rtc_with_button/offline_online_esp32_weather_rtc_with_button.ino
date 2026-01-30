#include <HardwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <RTClib.h>
#include <time.h>  // For NTP

// Ultrasonic Sensor Pins
#define trigPin 27
#define echoPin 26

// Switch pins
#define switchWiFiPin 33  // WiFi
#define switchUltraPin 32 // Ultrasonic
#define switchRTC_DHTPin 25  // RTC + DHT22

// DHT Sensor
#define DHTPIN 14
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// RTC
RTC_DS3231 rtc;

// DFPlayer on UART2 (RX2=16, TX2=17)
HardwareSerial mp3(2);

// WiFi credentials
const char* ssid = "Sahin";
const char* password = "87654321";
const char* city = "Dhaka";
const char* apiKey = "08425a8650df8d9f23a777284e81845a";

// State tracking
bool wifiEnabled = false;
bool ultrasonicEnabled = false;
bool prevWifiSwitchState = false;
bool prevUltraSwitchState = false;
bool prevRTC_DHT_Switch = false;

// Timing
unsigned long previousUltrasonicTime = 0;
const unsigned long ultrasonicInterval = 1000;

unsigned long previousWeatherTime = 0;
const unsigned long weatherInterval = 6000;

void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(switchWiFiPin, INPUT_PULLUP);
  pinMode(switchUltraPin, INPUT_PULLUP);
  pinMode(switchRTC_DHTPin, INPUT_PULLUP);

  dht.begin();
  Wire.begin();
  rtc.begin();

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Set to compile time
  }

  mp3.begin(9600, SERIAL_8N1, 16, 17);
  setVolume(20);

  Serial.println("Setup complete.");
}

void loop() {
  unsigned long currentMillis = millis();

  bool wifiSwitchState = digitalRead(switchWiFiPin) == LOW;
  bool ultraSwitchState = digitalRead(switchUltraPin) == LOW;
  bool rtcDhtSwitchState = digitalRead(switchRTC_DHTPin) == LOW;

  // WiFi toggle
  if (wifiSwitchState != prevWifiSwitchState) {
    if (wifiSwitchState) {
      Serial.println("WiFi Enabled");
      connectToWiFi();
      wifiEnabled = true;
    } else {
      Serial.println("WiFi Disabled");
      WiFi.disconnect(true);
      wifiEnabled = false;
    }
    prevWifiSwitchState = wifiSwitchState;
  }

  // Ultrasonic toggle
  if (ultraSwitchState != prevUltraSwitchState) {
    ultrasonicEnabled = ultraSwitchState;
    Serial.println(ultrasonicEnabled ? "Ultrasonic Enabled" : "Ultrasonic Disabled");
    prevUltraSwitchState = ultraSwitchState;
  }

  // RTC + DHT switch
  if (rtcDhtSwitchState != prevRTC_DHT_Switch) {
    if (rtcDhtSwitchState) {
      Serial.println("DHT + RTC Enabled");

      float t = dht.readTemperature();
      float h = dht.readHumidity();
      DateTime now = rtc.now();

      Serial.printf("Date (RTC): %02d/%02d/%d\n", now.day(), now.month(), now.year());
      Serial.printf("Time (RTC): %02d:%02d:%02d\n", now.hour(), now.minute(), now.second());

      if (isnan(t) || isnan(h)) {
        Serial.println("Failed to read from DHT sensor!");
      } else {
        Serial.printf("Temperature: %.2f°C\n", t);
        Serial.printf("Humidity   : %.1f%%\n", h);
      }
    } else {
      Serial.println("DHT + RTC Disabled");
    }
    prevRTC_DHT_Switch = rtcDhtSwitchState;
  }

  // Obstacle detection
  if (ultrasonicEnabled && currentMillis - previousUltrasonicTime >= ultrasonicInterval) {
    previousUltrasonicTime = currentMillis;
    float distance = readUltrasonicDistance();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    if (distance > 0 && distance < 50) {
      playAudio();  // Play every second if object is present
    }
  }

  // Weather data fetch
  if (wifiEnabled && currentMillis - previousWeatherTime >= weatherInterval) {
    previousWeatherTime = currentMillis;
    fetchOnlineWeather();
  }

  delay(50);  // Debounce
}

// --------- Helper Functions ------------

float readUltrasonicDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

void sendCommand(uint8_t cmd, uint8_t param1, uint8_t param2) {
  uint16_t checksum = 0xFFFF - (0xFF + 0x06 + cmd + 0x00 + param1 + param2) + 1;
  uint8_t packet[] = {
    0x7E, 0xFF, 0x06, cmd, 0x00, param1, param2,
    (uint8_t)(checksum >> 8), (uint8_t)(checksum & 0xFF), 0xEF
  };
  for (int i = 0; i < 10; i++) mp3.write(packet[i]);
}

void playAudio() {
  sendCommand(0x0D, 0, 0);
  Serial.println("Playing audio...");
}

void setVolume(uint8_t vol) {
  if (vol > 30) vol = 30;
  sendCommand(0x06, 0, vol);
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    setupNTP();  // NTP sync after WiFi connect
  } else {
    Serial.println("\nFailed.");
  }
}

void setupNTP() {
  configTime(6 * 3600, 0, "pool.ntp.org", "time.nist.gov");  // UTC+6 for Bangladesh
  Serial.println("Waiting for NTP time sync...");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nTime synced!");
}

void fetchOnlineWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return;
  }

  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + String(city) + "&appid=" + String(apiKey) + "&units=metric";
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Weather JSON:");
    Serial.println(payload);

    const size_t capacity = 1024;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("JSON error: ");
      Serial.println(error.f_str());
      http.end();
      return;
    }

    float temp = doc["main"]["temp"] | 0.0;
    int humidity = doc["main"]["humidity"] | 0;
    const char* condition = doc["weather"][0]["main"] | "Unknown";

    Serial.println("Weather Info:");
    Serial.printf("Condition: %s\n", condition);
    Serial.printf("Temperature: %.2f °C\n", temp);
    Serial.printf("Humidity: %d %%\n", humidity);

    // Get current time from NTP
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      Serial.printf("Date (NTP): %02d/%02d/%04d\n", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
      Serial.printf("Time (NTP): %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
      Serial.println("Failed to get NTP time.");
    }

    Serial.println("----------------------");
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(httpCode);
  }
  http.end();
}
