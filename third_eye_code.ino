#define trigPin 9
#define echoPin 10
#define buzzer 6
#define led 5
#define button 4

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(button, INPUT_PULLUP); 
  Serial.begin(9600);
}

void loop() {
  if (digitalRead(button) == LOW) {
    long duration;
    int distance;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;

    Serial.print("Distance: ");
    Serial.println(distance);

    if (distance < 50) {
      digitalWrite(buzzer, HIGH);
      digitalWrite(led, HIGH);
      delay(100);                 // LED blink delay (safe without resistor)
      digitalWrite(led, LOW);
      delay(100);
    } else {
      digitalWrite(buzzer, LOW);
      digitalWrite(led, LOW);
    }

    delay(100);
  }
}
