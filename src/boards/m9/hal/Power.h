#pragma once

#include <Arduino.h>
#include "config/BoardConfig.h"

class Power {
public:
    static void enablePeripherals();

    void begin();
    void loop();
    void activity();
    void weakActivity();
    void forceScreenOff();

    float batteryVoltage() const;
    int batteryPercent() const;
    bool isCharging() const;
    void setBatteryModel(uint8_t model);
    void setChargeThreshold(float voltage);
    void setFullBatteryVoltage(float voltage);

    void setBrightness(uint8_t percent);
    void setDimTimeout(uint16_t seconds) { _dimTimeout = seconds * 1000UL; }
    void setOffTimeout(uint16_t seconds) { _offTimeout = seconds * 1000UL; }
    void setKbBrightness(uint8_t percent, bool apply = false);
    void setKbAutoOn(bool enable) { _kbAutoOn = enable; }
    void setKbAutoOff(bool enable) { _kbAutoOff = enable; }

    enum State { ACTIVE, DIMMED, SCREEN_OFF };
    State state() const { return _state; }
    bool isScreenOn() const { return _state != SCREEN_OFF; }
    bool isDimmed() const { return _state == DIMMED; }

private:
    void setState(State state);
    uint8_t percentToPWM(uint8_t percent) const;

    State _state = ACTIVE;
    uint32_t _lastActivity = 0;
    uint32_t _dimTimeout = 30000;
    uint32_t _offTimeout = 60000;
    uint8_t _brightnessPct = 100;
    uint8_t _batteryModel = 0;
    float _chargeThreshold = 4.0f;
    float _fullBatteryV = 3.9f;
    bool _kbAutoOn = false;
    bool _kbAutoOff = false;
    bool _kbLitBeforeOff = false;
    bool _justWokeFromOff = false;
};
