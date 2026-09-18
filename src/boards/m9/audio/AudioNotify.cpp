#include "AudioNotify.h"
#include "config/BoardConfig.h"

void AudioNotify::begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    noTone(BUZZER_PIN);
    _ready = true;
    Serial.println("[AUDIO] M9 passive buzzer initialized");
}

void AudioNotify::end() {
    noTone(BUZZER_PIN);
    _ready = false;
}

void AudioNotify::play(uint16_t frequency, uint16_t durationMs) {
    if (!_enabled || !_ready || _volume == 0) return;
    tone(BUZZER_PIN, frequency, durationMs);
    delay(durationMs);
    noTone(BUZZER_PIN);
}

void AudioNotify::playMessage() {
    play(1000, 35);
    delay(25);
    play(1000, 35);
}

void AudioNotify::playAnnounce() { play(800, 35); }

void AudioNotify::playError() {
    for (uint8_t count = 0; count < 3; ++count) {
        play(400, 80);
        if (count < 2) delay(40);
    }
}

void AudioNotify::playBoot() {
    play(660, 45);
    play(830, 45);
    play(990, 60);
}

void AudioNotify::requestMessage() {
    if (_enabled) _messagePending = true;
}

void AudioNotify::loop() {
    if (!_messagePending) return;
    _messagePending = false;
    playMessage();
}
