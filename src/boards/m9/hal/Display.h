#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "config/BoardConfig.h"

class LGFX_M9 : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel;
    lgfx::Bus_SPI _bus;
    lgfx::Light_PWM _light;

public:
    LGFX_M9() {
        auto busConfig = _bus.config();
        busConfig.spi_host = SPI2_HOST;
        busConfig.spi_mode = 0;
        busConfig.freq_write = TFT_SPI_FREQ;
        busConfig.freq_read = 16000000;
        busConfig.pin_sclk = SPI_SCK;
        busConfig.pin_miso = SPI_MISO;
        busConfig.pin_mosi = SPI_MOSI;
        busConfig.pin_dc = TFT_DC;
        _bus.config(busConfig);
        _panel.setBus(&_bus);

        auto panelConfig = _panel.config();
        panelConfig.pin_cs = TFT_CS;
        panelConfig.pin_rst = TFT_RST;
        panelConfig.panel_width = 240;
        panelConfig.panel_height = 320;
        panelConfig.memory_width = 240;
        panelConfig.memory_height = 320;
        panelConfig.offset_x = 0;
        panelConfig.offset_y = 0;
        panelConfig.invert = true;
        panelConfig.rgb_order = false;
        _panel.config(panelConfig);

        auto lightConfig = _light.config();
        lightConfig.pin_bl = TFT_BL;
        lightConfig.invert = true;
        lightConfig.freq = 12000;
        lightConfig.pwm_channel = 0;
        _light.config(lightConfig);
        _panel.setLight(&_light);

        setPanel(&_panel);
    }
};

class Display {
public:
    bool begin();
    bool beginLVGL();
    void setBrightness(uint8_t level);
    void sleep();
    void wakeup();

    LGFX_M9& gfx() { return _gfx; }

private:
    LGFX_M9 _gfx;
};
