#include <SoftwareSerial.h>

SoftwareSerial mp3(10, 11); // RX, TX for DFPlayer

const int trigLeft = 2;
const int echoLeft = 3;
const int trigRight = 4;
const int echoRight = 5;

long durationLeft, durationRight;
int distanceLeft, distanceRight;

int volume = 20;

void setup() {
  Serial.begin(9600);
  mp3.begin(9600);

  pinMode(trigLeft, OUTPUT);
  pinMode(echoLeft, INPUT);
  pinMode(trigRight, OUTPUT);
  pinMode(echoRight, INPUT);

 // setVolume(volume);

  Serial.println("Setup done");
}

void loop() {
  distanceLeft = getDistance(trigLeft, echoLeft);
  distanceRight = getDistance(trigRight, echoRight);

  Serial.print("Left: ");
  Serial.print(distanceLeft);
  Serial.print(" cm, Right: ");
  Serial.print(distanceRight);
  Serial.println(" cm");

  // Threshold distance (cm) for obstacle detection
  int threshold = 20;

  static bool leftObstaclePrev = false;
  static bool rightObstaclePrev = false;

  bool leftObstacle = distanceLeft > 0 && distanceLeft < threshold;
  bool rightObstacle = distanceRight > 0 && distanceRight < threshold;

  if (leftObstacle && !leftObstaclePrev) {
    // Bam dike obstacle detected first time
    Serial.println("Obstacle Left - Next Track");
    nextTrack();
  }
  if (rightObstacle && !rightObstaclePrev) {
    // Dan dike obstacle detected first time
    Serial.println("Obstacle Right - Previous Track");
    previousTrack();
  }
 // f (leftObstacle && rightObstacle) {
    // Both sides obstacle - Play command
   // Serial.println("Both obstacles - Play Command");
    //play();
  //}

  leftObstaclePrev = leftObstacle;
  rightObstaclePrev = rightObstacle;

  delay(500); // small delay to avoid rapid commands
}

long getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // timeout 30ms

  long distance = duration * 0.034 / 2; // in cm
  if (duration == 0) return -1; // no reading
  return distance;
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

void nextTrack() {
  sendCommand(0x01, 0, 0);  // Next track
}

void previousTrack() {
  sendCommand(0x02, 0, 0);  // Previous track
}

void play() {
  sendCommand(0x0D, 0, 0);  // Play / Resume
}
