#include <DHT.h>
#include <Wire.h>
#include "RTClib.h"

#define DHTPIN 2  
#define DHTTYPE DHT22  
DHT dht(DHTPIN, DHTTYPE);

RTC_DS3231 rtc;

const int trigPin = 9;
const int echoPin = 10;

void setup() {
  Serial.begin(9600);
  dht.begin();
  Wire.begin();
  rtc.begin();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

 
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  // তাপমাত্রা রিডিং
  float temp = dht.readTemperature();

  
  if (isnan(temp)) {
    Serial.println("Error: DHT22 তাপমাত্রা পড়তে পারেনি!");
  } else {
   
    Serial.print("🌡 তাপমাত্রা: ");
    Serial.print(temp);
    Serial.println(" °C");
  }

 
  DateTime now = rtc.now();

 
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2;


  Serial.print("📅 তারিখ: ");
  Serial.print(now.day());
  Serial.print("/");
  Serial.print(now.month());
  Serial.print("/");
  Serial.println(now.year());

  Serial.print("🕒 সময়: ");
  if (now.hour() < 10) Serial.print("0");
  Serial.print(now.hour());
  Serial.print(":");
  if (now.minute() < 10) Serial.print("0");
  Serial.print(now.minute());
  Serial.print(":");
  if (now.second() < 10) Serial.print("0");
  Serial.println(now.second());

  Serial.print("🚧 সামনের বাধা: ");
  Serial.print(distance, 2);
  Serial.println(" সেমি");

  Serial.println("-----------------------------");
  delay(5000); 
}