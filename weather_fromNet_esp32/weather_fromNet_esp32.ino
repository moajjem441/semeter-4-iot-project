#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"

// WiFi credentials
const char* ssid = "Sahin";
const char* password = "87654321";

// NTP Server info
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 6 * 3600; // Bangladesh GMT+6
const int daylightOffset_sec = 0;

// OpenWeatherMap Info
const char* city = "Dhaka";
const char* apiKey = "f0866b97eab408a4d22955ac64d87fe0";  // <-- তোমার API Key এখানে
// base url = http://api.openweathermap.org/data/2.5/weather?q=Dhaka&appid=YOUR_API_KEY&units=metric

// Function to print local time
void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("❌ Failed to obtain time");
    return;
  }
  Serial.print("⏰ Time: ");
  Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
}

// Function to fetch weather
void fetchWeather() {
  HTTPClient http;
  String url = String("http://api.openweathermap.org/data/2.5/weather?q=") + city + "&appid=" + apiKey + "&units=metric";

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();

    Serial.println("🔄 Raw JSON payload:");
    Serial.println(payload);  // Debug purpose

    // JSON parsing
    const size_t capacity = 2048;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("❌ JSON parsing failed: ");
      Serial.println(error.f_str());
      return;
    }

    float temp = doc["main"]["temp"] | 0.0;
    int humidity = doc["main"]["humidity"] | 0;
    const char* condition = doc["weather"][0]["main"] | "Unknown";

    Serial.println("🌤️ Weather Information:");
    Serial.print("Temperature: "); Serial.print(temp); Serial.println(" °C");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
    Serial.print("Condition: "); Serial.println(condition);

  } else {
    Serial.print("❌ HTTP request failed, error code: ");
    Serial.println(httpCode);
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi Connected!");
  Serial.print("📶 IP Address: ");
  Serial.println(WiFi.localIP());

  // Setup time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(2000); // Wait for time sync

  printLocalTime();  // Show current time
  fetchWeather();    // Get weather info
}

void loop() {
  delay(600000); // প্রতি 10 মিনিটে আপডেট

  printLocalTime();
  fetchWeather();
}
