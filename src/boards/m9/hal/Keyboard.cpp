#include "Keyboard.h"

namespace {
constexpr uint8_t REG_HW_VERSION = 0x00;
constexpr uint8_t REG_KEY = 0x01;
constexpr uint8_t REG_BACKLIGHT = 0x02;
constexpr uint8_t REG_FW_VERSION = 0xFE;

constexpr uint8_t KEY_LEFT = 0xB4;
constexpr uint8_t KEY_UP = 0xB5;
constexpr uint8_t KEY_DOWN = 0xB6;
constexpr uint8_t KEY_RIGHT = 0xB7;
constexpr uint8_t KEY_ENTER = 0x0D;
constexpr uint8_t KEY_DELETE = 0x08;
constexpr uint8_t KEY_MESSAGES = 0x81;
constexpr uint8_t KEY_HOME = 0x82;
constexpr uint8_t KEY_SUB_MESSAGES = 0x83;
constexpr uint8_t KEY_BACK = 0x86;
constexpr uint8_t KEY_ENTER_LONG = 0xA3;
}

bool Keyboard::readRegister(uint8_t reg, uint8_t& value) {
    Wire1.beginTransmission(KB_I2C_ADDR);
    Wire1.write(reg);
    if (Wire1.endTransmission() != 0) return false;
    if (Wire1.requestFrom((uint8_t)KB_I2C_ADDR, (uint8_t)1) != 1) return false;
    value = Wire1.read();
    return true;
}

bool Keyboard::writeRegister(uint8_t reg, uint8_t value) {
    Wire1.beginTransmission(KB_I2C_ADDR);
    Wire1.write(reg);
    Wire1.write(value);
    return Wire1.endTransmission() == 0;
}

bool Keyboard::begin() {
    Wire1.begin(KB_I2C_SDA, KB_I2C_SCL);
    Wire1.setClock(100000);
    Wire1.setTimeOut(20);

    uint8_t hardwareVersion = 0;
    uint8_t firmwareVersion = 0;
    _available = readRegister(REG_HW_VERSION, hardwareVersion);
    if (_available) {
        readRegister(REG_FW_VERSION, firmwareVersion);
        Serial.printf("[KEYBOARD] M9 controller ready (hw=0x%02X fw=0x%02X)\n",
                      hardwareVersion, firmwareVersion);
    } else {
        Serial.println("[KEYBOARD] M9 controller not ready; retrying in background");
    }
    return _available;
}

void Keyboard::update() {
    _hasEvent = false;
    _longPress = false;

    const uint32_t now = millis();
    if (now - _lastPollMs < 15) return;
    _lastPollMs = now;

    if (!_available) {
        if (now - _lastProbeMs < 1000) return;
        _lastProbeMs = now;
        uint8_t hardwareVersion = 0;
        _available = readRegister(REG_HW_VERSION, hardwareVersion);
        if (!_available) return;
        Serial.printf("[KEYBOARD] M9 controller connected (hw=0x%02X)\n", hardwareVersion);
    }

    uint8_t key = 0;
    if (!readRegister(REG_KEY, key)) {
        _available = false;
        return;
    }
    if (key == 0 || key == 0xFF) return;

    _event = {};
    switch (key) {
        case KEY_LEFT:         _event.left = true; break;
        case KEY_UP:           _event.up = true; break;
        case KEY_DOWN:         _event.down = true; break;
        case KEY_RIGHT:        _event.right = true; break;
        case KEY_ENTER:        _event.enter = true; _event.character = '\n'; break;
        case KEY_DELETE:       _event.del = true; _event.character = 0x08; break;
        case KEY_BACK:         _event.character = 0x1B; break;
        case KEY_HOME:         _event.home = true; break;
        case KEY_MESSAGES:
        case KEY_SUB_MESSAGES: _event.messages = true; break;
        case KEY_ENTER_LONG:   _longPress = true; return;
        default:
            if (key >= 0x20 && key <= 0x7E) {
                _event.character = (char)key;
                _event.space = key == ' ';
            } else {
                return;
            }
    }
    _hasEvent = true;
}

bool Keyboard::setBacklightBrightness(uint8_t percent) {
    percent = constrain(percent, 0, 100);
    _backlightBrightness = (uint8_t)((uint16_t)percent * 255 / 100);
    return writeRegister(REG_BACKLIGHT, _backlightBrightness);
}

bool Keyboard::backlightOn() {
    _backlightLit = _backlightBrightness != 0;
    return writeRegister(REG_BACKLIGHT, _backlightBrightness);
}

bool Keyboard::backlightOff() {
    _backlightLit = false;
    return writeRegister(REG_BACKLIGHT, 0);
}
