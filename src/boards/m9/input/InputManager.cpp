#include "InputManager.h"

void InputManager::update() {
    _hasKey = false;
    _activity = false;
    _longPress = false;
    if (!_keyboard) return;

    _keyboard->update();
    _longPress = _keyboard->hadLongPress();
    if (_keyboard->hasEvent()) {
        _keyEvent = _keyboard->getEvent();
        _hasKey = true;
    }
    _activity = _hasKey || _longPress;
}
