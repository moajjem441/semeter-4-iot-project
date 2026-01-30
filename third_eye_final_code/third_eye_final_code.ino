#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <DHT.h>
#include <Wire.h>
#include <RTClib.h>
#include <TinyGPS++.h>

// ==== 🔧 Function Prototypes ==== //
void announceNumber(int number, int delayAfter = 0);
void announceLocation();
void checkObstacle();
void announceDate();
void announceTime();
void announceTemperature();
// =============================== //

// পিন কনফিগারেশন
#define DHTPIN 5
#define DHTTYPE DHT22
#define TRIG_PIN 6
#define ECHO_PIN 7

#define MODE_BUTTON_PIN 4
#define POWER_BUTTON_PIN 3
#define ULTRA_BUTTON_PIN 2
#define EMERGENCY_BUTTON_PIN 12

// সফটওয়্যার সিরিয়াল ইনস্ট্যান্স
SoftwareSerial dfSerial(10, 11);      // DFPlayer এর জন্য
SoftwareSerial gpsSerial(8, 9);       // GPS মডিউলের জন্য
SoftwareSerial sim800lSerial(A0, A1); // SIM800L মডিউলের জন্য

// বিভিন্ন ডিভাইস ইনিশিয়ালাইজ
DFRobotDFPlayerMini myDFPlayer;
DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;
TinyGPSPlus gps;

// মাস ও সপ্তাহের অডিও ফাইল নাম্বার
const int monthFiles[12] = {40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
const int dayFiles[7] = {60, 61, 62, 63, 64, 65, 66};

// অডিও ফাইল নাম্বার ডিফাইন
#define AUDIO_SEARCHING_LOC 76
#define AUDIO_TRY_OPEN_AREA 77
#define AUDIO_GPS_NOT_FOUND 78

// প্রাথমিক অবস্থা সেটআপ
int modeCounter = 0; // ১ = তারিখ, ২ = সময়, ৩ = তাপমাত্রা, ৪ = অবস্থান
bool powerOn = true;
bool sensorTrigger = false;
bool emergencyMode = false;

void setup() {
  // বাটন ইনপুট হিসেবে সেট করা
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(POWER_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ULTRA_BUTTON_PIN, INPUT_PULLUP);
  pinMode(EMERGENCY_BUTTON_PIN, INPUT_PULLUP);

  // আলট্রাসোনিক পিন
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // সিরিয়াল শুরু
  Serial.begin(9600);
  dfSerial.begin(9600);
  gpsSerial.begin(9600);
  sim800lSerial.begin(9600);

  dht.begin(); // DHT শুরু

  if (!rtc.begin()) {
    Serial.println("RTC পাওয়া যায়নি!");
    while (1);
  }

  if (!myDFPlayer.begin(dfSerial)) {
    Serial.println("DFPlayer পাওয়া যায়নি!");
    while (1);
  }

  myDFPlayer.volume(25);  // ভলিউম সেট

  // GPS কানেকশন চেক
  unsigned long start = millis();
  bool gpsConnected = false;
  while (millis() - start < 2000) {
    if (gpsSerial.available()) {
      gpsConnected = true;
      break;
    }
    delay(100);
  }

  if (!gpsConnected) {
    myDFPlayer.play(AUDIO_GPS_NOT_FOUND); // GPS পাওয়া যায়নি অডিও
  }
}

// নির্দিষ্ট সময়ের মধ্যে GPS লোকেশন পাওয়ার চেষ্টা
bool getGPSLocationWithTimeout(unsigned long timeout) {
  unsigned long startTime = millis();
  while (millis() - startTime < timeout) {
    while (gpsSerial.available() > 0) {
      if (gps.encode(gpsSerial.read())) {
        if (gps.location.isValid() && gps.location.isUpdated()) {
          return true;
        }
      }
    }
    delay(10);
  }
  return false;
}

// অবস্থান ঘোষণা করা
void announceLocation() {
  if (!powerOn) return;

  myDFPlayer.play(AUDIO_SEARCHING_LOC); // "অবস্থান খোঁজা হচ্ছে"

  if (getGPSLocationWithTimeout(15000)) {
    double latitude = gps.location.lat();
    double longitude = gps.location.lng();

    if (abs(latitude - 23.8) < 0.5 && abs(longitude - 90.4) < 0.5) {
      myDFPlayer.play(82); // "আপনি ঢাকায় আছেন"
    } else {
      myDFPlayer.play(81); delay(1500); // "আপনার অবস্থান"
      myDFPlayer.play(32); delay(1500); // "Latitude"
      announceNumber((int)latitude, 0); delay(1500);
      myDFPlayer.play(31); delay(1500); // "Longitude"
      announceNumber((int)longitude, 0); delay(1500);
    }
  } else {
    myDFPlayer.play(78); // "অবস্থান পাওয়া যায়নি"
    myDFPlayer.play(AUDIO_TRY_OPEN_AREA); // "খোলা জায়গায় চেষ্টা করুন"
  }
}

// বাধা শনাক্ত করা
void checkObstacle() {
  if (!powerOn) return;
  long distance = readDistanceCM();
  if (distance < 100 && distance > 0) {
    myDFPlayer.play(36); delay(1000); // "সামনে বাধা আছে"
    announceNumber((int)distance, 0); delay(1000);
    myDFPlayer.play(37); // "সেন্টিমিটার"
  }
}

// দূরত্ব পড়া
long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

// সংখ্যা ঘোষণা ফাংশন
void announceNumber(int number, int delayAfter = 0) {
  if (number == 0) {
    myDFPlayer.play(10); // "zero"
  }

  if (number < 0) {
    myDFPlayer.play(11); // "minus"
    number = -number;
  }

  String numStr = String(number);
  for (int i = 0; i < numStr.length(); i++) {
    int digit = numStr.charAt(i) - '0';
    myDFPlayer.play(digit);
    if (delayAfter > 0) delay(delayAfter);
  }
}

// তারিখ ঘোষণা
void announceDate() {
  if (!powerOn) return;
  DateTime now = rtc.now();
  myDFPlayer.play(80); delay(500); // "আজকের তারিখ"
  announceNumber(now.day()); delay(500);
  myDFPlayer.play(dayFiles[now.dayOfTheWeek()]); delay(500);
  myDFPlayer.play(monthFiles[now.month() - 1]); delay(500);
}

// সময় ঘোষণা
void announceTime() {
  if (!powerOn) return;
  DateTime now = rtc.now();
  myDFPlayer.play(70); delay(500); // "এখন সময়"
  announceNumber(now.hour()); delay(500);
  myDFPlayer.play(71); // "টা"
  announceNumber(now.minute()); delay(500);
  myDFPlayer.play(72); // "মিনিট"
}

// তাপমাত্রা ঘোষণা
void announceTemperature() {
  if (!powerOn) return;
  float temp = dht.readTemperature();
  if (isnan(temp)) {
    myDFPlayer.play(79); // "তাপমাত্রা পাওয়া যায়নি"
    return;
  }
  myDFPlayer.play(30); delay(500); // "বর্তমান তাপমাত্রা"
  announceNumber((int)temp); delay(500);
  myDFPlayer.play(35); // "ডিগ্রি সেলসিয়াস"
}

// লুপ চলমান
void loop() {
  // মোড বাটন চেক
  if (digitalRead(MODE_BUTTON_PIN) == LOW) {
    delay(200);
    modeCounter++;
    if (modeCounter > 4) modeCounter = 1;
    switch (modeCounter) {
      case 1: announceDate(); break;
      case 2: announceTime(); break;
      case 3: announceTemperature(); break;
      case 4: announceLocation(); break;
    }
  }

  // পাওয়ার বাটন
  if (digitalRead(POWER_BUTTON_PIN) == LOW) {
    delay(200);
    powerOn = !powerOn;
    if (powerOn) {
      myDFPlayer.play(88); // "চালু"
    } else {
      myDFPlayer.play(89); // "বন্ধ"
    }
  }

  // আলট্রাসোনিক মোড বাটন
  if (digitalRead(ULTRA_BUTTON_PIN) == LOW) {
    delay(200);
    sensorTrigger = !sensorTrigger;
    if (sensorTrigger) {
      myDFPlayer.play(86); // "সেন্সর চালু"
    } else {
      myDFPlayer.play(87); // "সেন্সর বন্ধ"
    }
  }

  // আলট্রাসোনিক মোড চালু থাকলে বাধা চেক করা
  if (sensorTrigger && powerOn) {
    checkObstacle(); 
  }

  // ইমার্জেন্সি মোড
  if (digitalRead(EMERGENCY_BUTTON_PIN) == LOW) {
    delay(200);
    emergencyMode = !emergencyMode;
    if (emergencyMode) {
      myDFPlayer.play(33); // "ইমার্জেন্সি মোড চালু"
      sim800lSerial.println("AT+CMGF=1"); delay(1000);
      sim800lSerial.println("AT+CMGS=\"+8801708372039\""); delay(1000);
      sim800lSerial.print("Emergency! Please check the location or contact immediately.");
      sim800lSerial.write(26);
    } else {
      myDFPlayer.play(34); // "ইমার্জেন্সি মোড বন্ধ"
    }
  }

  delay(100);
}
