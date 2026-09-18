#include "Power.h"
#include "hal/Display.h"
#include "hal/Keyboard.h"
#include <soc/soc.h>
#include <soc/usb_serial_jtag_reg.h>

extern Display display;
extern Keyboard keyboard;

namespace {
struct VoltPoint { float voltage; int percent; };
constexpr VoltPoint LIPO_CURVE[] = {
    {3.90f, 100}, {3.80f, 90}, {3.72f, 80}, {3.65f, 70}, {3.59f, 60},
    {3.53f, 50}, {3.48f, 40}, {3.44f, 30}, {3.40f, 20}, {3.36f, 15},
    {3.30f, 10}, {3.15f, 5}, {3.00f, 0},
};
}

void Power::enablePeripherals() {
    REG_CLR_BIT(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_USB_PAD_ENABLE);

    pinMode(BOARD_POWER_PIN, OUTPUT);
    digitalWrite(BOARD_POWER_PIN, BOARD_POWER_ACTIVE);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    pinMode(GPS_EN, OUTPUT);
    digitalWrite(GPS_EN, LOW);
    pinMode(GPS_RST, OUTPUT);
    digitalWrite(GPS_RST, LOW);
    delay(20);
}

void Power::begin() {
    _lastActivity = millis();
    _state = ACTIVE;
    pinMode(BAT_ADC_PIN, INPUT);
    analogReadResolution(12);
    Serial.println("[POWER] M9 power manager initialized");
}

float Power::batteryVoltage() const {
    static float lastGoodVoltage = 0.0f;
    uint32_t millivolts = 0;
    uint8_t samples = 0;
    for (uint8_t index = 0; index < 8; ++index) {
        const uint32_t sample = analogReadMilliVolts(BAT_ADC_PIN);
        if (sample >= 1250) {
            millivolts += sample;
            ++samples;
        }
    }
    if (samples == 0) return lastGoodVoltage;
    lastGoodVoltage = (millivolts / (float)samples) * 2.0f / 1000.0f;
    return lastGoodVoltage;
}

int Power::batteryPercent() const {
    float voltage = batteryVoltage();
    if (isCharging()) return 100;
    voltage += 3.9f - _fullBatteryV;
    voltage = constrain(voltage, 3.0f, 4.2f);
    if (_batteryModel == 1) return (int)((voltage - 3.0f) / 1.2f * 100.0f);

    constexpr size_t pointCount = sizeof(LIPO_CURVE) / sizeof(LIPO_CURVE[0]);
    for (size_t index = 0; index + 1 < pointCount; ++index) {
        if (voltage >= LIPO_CURVE[index + 1].voltage) {
            const float ratio = (voltage - LIPO_CURVE[index + 1].voltage) /
                (LIPO_CURVE[index].voltage - LIPO_CURVE[index + 1].voltage);
            return (int)(LIPO_CURVE[index + 1].percent + ratio *
                (LIPO_CURVE[index].percent - LIPO_CURVE[index + 1].percent));
        }
    }
    return 0;
}

bool Power::isCharging() const {
    return batteryVoltage() >= _chargeThreshold;
}

void Power::setBatteryModel(uint8_t model) { _batteryModel = model; }
void Power::setChargeThreshold(float voltage) { _chargeThreshold = voltage; }
void Power::setFullBatteryVoltage(float voltage) { _fullBatteryV = voltage; }

uint8_t Power::percentToPWM(uint8_t percent) const {
    if (percent == 0) return 0;
    if (percent >= 100) return 255;
    return (uint8_t)(6 + (uint16_t)(percent - 1) * 249 / 99);
}

void Power::activity() {
    _lastActivity = millis();
    if (_state == SCREEN_OFF) _justWokeFromOff = true;
    if (_state != ACTIVE) setState(ACTIVE);
}

void Power::weakActivity() {
    activity();
}

void Power::forceScreenOff() {
    if (_justWokeFromOff) {
        _justWokeFromOff = false;
        return;
    }
    setState(SCREEN_OFF);
}

void Power::setBrightness(uint8_t percent) {
    _brightnessPct = constrain(percent, 1, 100);
    if (_state == ACTIVE) display.setBrightness(percentToPWM(_brightnessPct));
}

void Power::setKbBrightness(uint8_t percent, bool apply) {
    keyboard.setBacklightBrightness(percent);
    if (percent == 0) keyboard.backlightOff();
    else if (apply) keyboard.backlightOn();
}

void Power::loop() {
    const uint32_t elapsed = millis() - _lastActivity;
    if (_state == ACTIVE && _offTimeout > 0 && elapsed >= _offTimeout) {
        setState(SCREEN_OFF);
    } else if (_state == ACTIVE && _dimTimeout > 0 && elapsed >= _dimTimeout) {
        setState(DIMMED);
    } else if (_state == DIMMED && _offTimeout > 0 && elapsed >= _offTimeout) {
        setState(SCREEN_OFF);
    }
    _justWokeFromOff = false;
}

void Power::setState(State state) {
    if (state == _state) return;
    const State previous = _state;
    _state = state;
    if (state == ACTIVE) {
        display.setBrightness(percentToPWM(_brightnessPct));
        if (previous == SCREEN_OFF) display.wakeup();
        if (_kbAutoOn || (previous == SCREEN_OFF && _kbLitBeforeOff)) keyboard.backlightOn();
    } else if (state == DIMMED) {
        display.setBrightness(40);
        if (_kbAutoOff) keyboard.backlightOff();
    } else {
        display.sleep();
        _kbLitBeforeOff = keyboard.backlightIsLit();
        keyboard.backlightOff();
    }
}
