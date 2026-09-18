#pragma once

#include "hal/Keyboard.h"
#include "hal/Power.h"

class InputManager {
public:
    void begin(Keyboard* keyboard) { _keyboard = keyboard; }
    void setPowerMgr(Power* power) { _power = power; }
    void setTrackballSpeed(uint8_t) {}
    void update();

    bool hasKeyEvent() const { return _hasKey; }
    const KeyEvent& getKeyEvent() const { return _keyEvent; }
    bool hadActivity() const { return _activity; }
    bool hadStrongActivity() const { return _activity; }
    bool hadLongPress() const { return _longPress; }

private:
    Keyboard* _keyboard = nullptr;
    Power* _power = nullptr;
    KeyEvent _keyEvent = {};
    bool _hasKey = false;
    bool _activity = false;
    bool _longPress = false;
};
