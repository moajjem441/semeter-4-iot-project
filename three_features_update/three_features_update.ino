#include <DHT.h>
#include <Wire.h>
#include "RTClib.h"

// DHT22 Setup
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// RTC Setup
RTC_DS3231 rtc;

// Ultrasonic & Buzzer Setup
const int trigPin = 9;
const int echoPin = 10;
const int buzzerPin = 6;

// Time tracking
unsigned long previousWeatherTime = 0;
unsigned long previousUltrasonicTime = 0;
const unsigned long weatherInterval = 6000;     // 6 sec
const unsigned long ultrasonicInterval = 1000;  // 2 sec

void setup() {
  Serial.begin(9600);
  dht.begin();
  Wire.begin();
  rtc.begin();

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // 🌡️ Weather + Time update every 6 seconds
  if (currentMillis - previousWeatherTime >= weatherInterval) {
    previousWeatherTime = currentMillis;

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
      Serial.println("❌ তাপমাত্রা বা আর্দ্রতা পাওয়া যায়নি!");
    } else {
      Serial.print("🌡 তাপমাত্রা: ");
      Serial.print(temp);
      Serial.println(" °C");

      Serial.print("💧 আর্দ্রতা: ");
      Serial.print(hum);
      Serial.println(" %");

      predictRain(temp, hum);
    }

    DateTime now = rtc.now();
    Serial.print("📅 তারিখ: ");
    Serial.print(now.day()); Serial.print("/");
    Serial.print(now.month()); Serial.print("/");
    Serial.println(now.year());

    Serial.print("🕒 সময়: ");
    if (now.hour() < 10) Serial.print("0");
    Serial.print(now.hour()); Serial.print(":");
    if (now.minute() < 10) Serial.print("0");
    Serial.print(now.minute()); Serial.print(":");
    if (now.second() < 10) Serial.print("0");
    Serial.println(now.second());

    Serial.println("-----------------------------");
  }

  // 🚧 Ultrasonic + Buzzer update every 2 seconds
  if (currentMillis - previousUltrasonicTime >= ultrasonicInterval) {
    previousUltrasonicTime = currentMillis;

    digitalWrite(trigPin, LOW); delayMicroseconds(2);
    digitalWrite(trigPin, HIGH); delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    float distance = duration * 0.034 / 2;

    Serial.print("🚧 সামনের বাধা আছে: ");
    Serial.print(distance, 2);
    Serial.println(" সেমি");

    if (distance > 0 && distance <= 20) {
      beepPattern(100, 100, 3);
    } 
    else if (distance > 21 && distance <= 35) {
      beepPattern(200, 150, 2);
    } 
    else if (distance > 35 && distance <= 50) {
      beepPattern(400, 200, 1);
    } 
    else {
      digitalWrite(buzzerPin, LOW);
    }
  }
}

// 🔁 Buzzer Beep Function
void beepPattern(int beepDuration, int gapDuration, int repeat) {
  for (int i = 0; i < repeat; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(beepDuration);
    digitalWrite(buzzerPin, LOW);
    delay(gapDuration);
  }
}

// ☁️ বৃষ্টির সম্ভাবনার অনুমান
void predictRain(float temp, float hum) {
  if (hum >= 95 && temp >= 28) {
    Serial.println("⛈️ খুব বেশি আর্দ্রতা ও গরম — বজ্রসহ বৃষ্টি হতে পারে!");
  }
  else if (hum >= 90 && temp <= 30) {
    Serial.println("☔ সম্ভবত বৃষ্টি খুব শীঘ্রই হতে পারে!");
  }
  else if (hum >= 80 && temp >= 25 && temp <= 33) {
    Serial.println("🌧️ বাতাসে জলীয়বাষ্প অনেক, মেঘ জমছে — বৃষ্টির সম্ভাবনা আছে।");
  }
  else if (hum >= 70 && temp < 24) {
    Serial.println("☁️ আবহাওয়া ঠান্ডা ও আর্দ্র, হালকা বৃষ্টি হতে পারে।");
  }
  else if (hum < 60 && temp > 32) {
    Serial.println("🌞 গরম ও শুষ্ক — বৃষ্টির সম্ভাবনা কম।");
  }
  else {
    Serial.println("🌤️ আবহাওয়া স্বাভাবিক, বৃষ্টির সম্ভাবনা খুব কম।");
  }
}