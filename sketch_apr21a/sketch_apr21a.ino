#include <SoftwareSerial.h>

SoftwareSerial mp3(10, 11); // RX, TX

int volume = 20;  // Default volume (0–30)

void setup() {
  Serial.begin(9600);
  mp3.begin(9600);
  delay(1000);

  setVolume(volume);
  Serial.println("MP3 Player Ready");
  Serial.println("Type a track number to play (e.g., 1)");
  Serial.println("Or type: play, pause, next, prev, volup, voldown");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();  // remove spaces or newlines

    if (input.equalsIgnoreCase("play")) {
      play();
    } else if (input.equalsIgnoreCase("pause")) {
      pause();
    } else if (input.equalsIgnoreCase("next")) {
      next();
    } else if (input.equalsIgnoreCase("prev")) {
      previous();
    } else if (input.equalsIgnoreCase("volup")) {
      if (volume < 30) volume++;
      setVolume(volume);
      Serial.print("Volume: "); Serial.println(volume);
    } else if (input.equalsIgnoreCase("voldown")) {
      if (volume > 0) volume--;
      setVolume(volume);
      Serial.print("Volume: "); Serial.println(volume);
    } else {
      int trackNum = input.toInt();
      if (trackNum > 0) {
        Serial.print("Playing track: ");
        Serial.println(trackNum);
        playTrack(trackNum);
      } else {
        Serial.println("Invalid command or track number.");
      }
    }
  }
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
  sendCommand(0x06, 0, vol);
}

void playTrack(uint16_t trackNumber) {
  sendCommand(0x0F, (trackNumber >> 8) & 0xFF, trackNumber & 0xFF);
}

void play() {
  sendCommand(0x0D, 0, 0); // Resume play
  Serial.println("Playback resumed");
}

void pause() {
  sendCommand(0x0E, 0, 0); // Pause playback
  Serial.println("Playback paused");
}

void next() {
  sendCommand(0x01, 0, 0); // Next track
  Serial.println("Next track");
}

void previous() {
  sendCommand(0x02, 0, 0); // Previous track
  Serial.println("Previous track");
}
