#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "config/BoardConfig.h"

enum class InputMode {
    Navigation,
    TextInput
};

struct KeyEvent {
    char character;
    bool ctrl;
    bool shift;
    bool fn;
    bool alt;
    bool opt;
    bool enter;
    bool del;
    bool tab;
    bool space;
    bool up;
    bool down;
    bool left;
    bool right;
    bool encoder;
    bool repeat;
    bool home;
    bool messages;
};

class Keyboard {
public:
    bool begin();
    void update();

    InputMode getMode() const { return _mode; }
    void setMode(InputMode mode) { _mode = mode; }
    bool hasEvent() const { return _hasEvent; }
    const KeyEvent& getEvent() const { return _event; }
    bool hadLongPress() const { return _longPress; }

    bool setBacklightBrightness(uint8_t percent);
    bool backlightOn();
    bool backlightOff();
    bool backlightIsLit() const { return _backlightLit; }

private:
    bool readRegister(uint8_t reg, uint8_t& value);
    bool writeRegister(uint8_t reg, uint8_t value);

    InputMode _mode = InputMode::Navigation;
    KeyEvent _event = {};
    bool _hasEvent = false;
    bool _longPress = false;
    bool _available = false;
    bool _backlightLit = false;
    uint8_t _backlightBrightness = 255;
    uint32_t _lastPollMs = 0;
    uint32_t _lastProbeMs = 0;
};
