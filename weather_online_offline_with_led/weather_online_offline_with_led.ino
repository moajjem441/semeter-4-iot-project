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
#define switchWiFiPin 33      // WiFi
#define switchUltraPin 32     // Ultrasonic
#define switchRTC_DHTPin 25   // RTC + DHT22

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

unsigned long previousUltrasonicTime = 0;
const unsigned long ultrasonicInterval = 1000;

unsigned long previousWeatherTime = 0;
const unsigned long weatherInterval = 10000;

unsigned long lastDebounceTimeWiFi = 0;
unsigned long lastDebounceTimeUltra = 0;
unsigned long lastDebounceTimeRTC_DHT = 0;
const unsigned long debounceDelay = 200;

// LED Pins
#define LED1 12
#define LED2 13
unsigned long previousMillis = 0;
const long interval = 500;
bool ledState = false;

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

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  wifiEnabled = false;

  Serial.println("Setup complete.");
}

void loop() {
  unsigned long currentMillis = millis();

  // LED Blink
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    ledState = !ledState;
    digitalWrite(LED1, ledState);
    digitalWrite(LED2, !ledState);
  }

  // Switch Reads
  bool wifiSwitchState = digitalRead(switchWiFiPin) == LOW;
  bool ultraSwitchState = digitalRead(switchUltraPin) == LOW;
  bool rtcDhtSwitchState = digitalRead(switchRTC_DHTPin) == LOW;

  // WiFi toggle
  if (wifiSwitchState != prevWifiSwitchState && millis() - lastDebounceTimeWiFi > debounceDelay) {
    lastDebounceTimeWiFi = millis();
    wifiEnabled = wifiSwitchState;
    if (wifiEnabled) {
      Serial.println("WiFi switch ON → connecting...");
      connectToWiFi();
      setupNTP();
    } else {
      Serial.println("WiFi switch OFF → disconnecting...");
      WiFi.disconnect();
    }
    prevWifiSwitchState = wifiSwitchState;
  }

  // Ultrasonic toggle
  if (ultraSwitchState != prevUltraSwitchState && millis() - lastDebounceTimeUltra > debounceDelay) {
    lastDebounceTimeUltra = millis();
    ultrasonicEnabled = ultraSwitchState;
    Serial.println(ultrasonicEnabled ? "Ultrasonic Enabled" : "Ultrasonic Disabled");
    prevUltraSwitchState = ultraSwitchState;
  }

  // RTC + DHT toggle
  if (rtcDhtSwitchState != prevRTC_DHT_Switch && millis() - lastDebounceTimeRTC_DHT > debounceDelay) {
    lastDebounceTimeRTC_DHT = millis();
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

  // Ultrasonic reading
  if (ultrasonicEnabled && currentMillis - previousUltrasonicTime >= ultrasonicInterval) {
    previousUltrasonicTime = currentMillis;
    float distance = readUltrasonicDistance();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    if (distance > 0 && distance < 50) {
      playAudio();
    }
  }

  // Weather fetch
  if (wifiEnabled && currentMillis - previousWeatherTime >= weatherInterval) {
    previousWeatherTime = currentMillis;
    fetchOnlineWeather();
  }

  delay(50);
}

// ---------- Functions ----------

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
  Serial.print("Connecting to WiFi");

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Failed to connect to WiFi.");
  }
}

void setupNTP() {
  configTime(6 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for NTP time sync...");

  struct tm timeinfo;
  int attempts = 0;

  while (!getLocalTime(&timeinfo) && attempts < 10) {
    Serial.print(".");
    delay(500);
    attempts++;
  }

  if (getLocalTime(&timeinfo)) {
    Serial.println("\nNTP time synced!");
    DateTime now(
      timeinfo.tm_year + 1900,
      timeinfo.tm_mon + 1,
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec
    );
    rtc.adjust(now);
    Serial.println("RTC updated from NTP!");
  } else {
    Serial.println("\nFailed to sync NTP time.");
  }
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
    Serial.println("\n--- Raw JSON Response ---");
    Serial.println(payload);

    const size_t capacity = 2048;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.f_str());
      http.end();
      return;
    }

  String cityName     = doc["name"] | "Unknown";
    float temp          = doc["main"]["temp"] | 0.0;
    float feels_like    = doc["main"]["feels_like"] | 0.0;
    int humidity        = doc["main"]["humidity"] | 0;
    int pressure        = doc["main"]["pressure"] | 0;
    float windSpeed     = doc["wind"]["speed"] | 0.0;
    int windDeg         = doc["wind"]["deg"] | 0;
    int visibility      = doc["visibility"] | 0;
    int cloudCover      = doc["clouds"]["all"] | 0;
    long sunriseUnix    = doc["sys"]["sunrise"] | 0;
    long sunsetUnix     = doc["sys"]["sunset"] | 0;
    String weatherMain  = doc["weather"][0]["main"] | "N/A";
    String description  = doc["weather"][0]["description"] | "N/A";

    // Print time from RTC
    Serial.println("\n--- Time and Date Info ---");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      Serial.printf("Date        : %02d/%02d/%04d\n", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
      Serial.printf("Time        : %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    // Convert UNIX time to readable format
    char sunriseStr[20], sunsetStr[20];
    time_t sunriseTime = sunriseUnix;
    time_t sunsetTime = sunsetUnix;
    strftime(sunriseStr, sizeof(sunriseStr), "%H:%M:%S", localtime(&sunriseTime));
    strftime(sunsetStr, sizeof(sunsetStr), "%H:%M:%S", localtime(&sunsetTime));

     Serial.printf("Today Sunrise      : %s\n", sunriseStr);
    Serial.printf("Today Sunset       : %s\n", sunsetStr);

    // Print weather info
    Serial.println("\n--- Weather Info ---");
    Serial.printf("City         : %s\n", cityName.c_str());
    Serial.printf("Temperature  : %.2f°C\n", temp);
    Serial.printf("Feels Like   : %.2f°C\n", feels_like);
    Serial.printf("Humidity     : %d%%\n", humidity);
    Serial.printf("Pressure     : %dhPa\n", pressure);
    Serial.printf("Wind Speed   : %.2fm/s\n", windSpeed);
    Serial.printf("Wind Dir     : %d°\n", windDeg);
    Serial.printf("Visibility   : %.2f km\n", visibility / 1000.0);
    Serial.printf("Cloud Cover  : %d%%\n", cloudCover);
    Serial.printf("Condition    : %s (%s)\n", weatherMain.c_str(), description.c_str());

    Serial.println("\n--- Weather Prediction ---");

if (pressure < 1005 && cloudCover > 70) {
  Serial.println("⚠️ Rain is likely. Don't forget your umbrella!");
} else if (cloudCover > 80) {
  Serial.println("☁️ Very cloudy, but low chance of rain.");
} else if (pressure >= 1010 && cloudCover < 20) {
  Serial.println("☀️ Clear sky. It's likely to be a nice sunny day.");
} else if (weatherMain == "Rain") {
  Serial.println("🌧️ It's currently raining or will rain soon.");
} else if (weatherMain == "Clear") {
  Serial.println("☀️ The sky is clear right now.");
} else {
  Serial.println("ℹ️ Normal weather conditions. Nothing extreme.");
}
    
  }
   else {
    Serial.print("HTTP GET failed, code: ");
    Serial.println(httpCode);
  }

  http.end();
}
