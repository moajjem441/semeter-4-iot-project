#define trigPin 27
#define echoPin 26

// DFPlayer RX = GPIO 10, TX = GPIO 11 (আপনি পিন নিজের মতো পরিবর্তন করতে পারেন)
#define DFPLAYER_RX 16
#define DFPLAYER_TX 17

HardwareSerial mp3(1);  // Use UART1 (Serial1)

void setup() {
  Serial.begin(115200);
  
  // UART1 init: RX, TX, baudrate
  mp3.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  delay(1000);
  setVolume(20);  // ভলিউম সেট করুন (0-30)
  Serial.println("System Ready");
}

void loop() {
  long duration;
  float distance;

  // Ultrasonic Trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Echo pulse measurement
  duration = pulseIn(echoPin, HIGH, 30000);  // timeout 30ms
  if (duration == 0) {
    Serial.println("No echo received");
    delay(200);
    return;
  }
  
  distance = (duration * 0.034) / 2.0;
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance > 0 && distance < 50) {
    play();
    delay(3000);  // অডিও প্লে হয়ে শেষ হতে সময় দিন
  }

  delay(500);
}

void sendCommand(uint8_t cmd, uint8_t param1, uint8_t param2) {
  uint16_t checksum = 0xFFFF - (0xFF + 0x06 + cmd + 0x00 + param1 + param2) + 1;
  uint8_t packet[] = {
    0x7E, 0xFF, 0x06, cmd, 0x00, param1, param2,
    (uint8_t)(checksum >> 8), (uint8_t)(checksum), 0xEF
  };
  for (int i = 0; i < 10; i++) {
    mp3.write(packet[i]);
  }
}

void setVolume(uint8_t vol) {
  sendCommand(0x06, 0, vol);  // Volume: 0–30
}

void play() {
  sendCommand(0x0D, 0, 0);  // Resume/play current track
  Serial.println("Playing audio...");
}
