#pragma once

#include <Arduino.h>

class AudioNotify {
public:
    void begin();
    void end();
    void playMessage();
    void playAnnounce();
    void playError();
    void playBoot();
    void requestMessage();
    void loop();

    void setEnabled(bool enabled) { _enabled = enabled; }
    bool isEnabled() const { return _enabled; }
    void setVolume(uint8_t volume) { _volume = volume; }
    uint8_t volume() const { return _volume; }

private:
    void play(uint16_t frequency, uint16_t durationMs);

    bool _enabled = true;
    bool _ready = false;
    volatile bool _messagePending = false;
    uint8_t _volume = 80;
};
