#include "Display.h"
#include "hal/SharedSPIBus.h"
#include "runtime/RuntimeMetrics.h"
#include <lvgl.h>

static lv_color_t* s_buffer1 = nullptr;
static lv_color_t* s_buffer2 = nullptr;
static LGFX_M9* s_display = nullptr;

static void lvglFlush(lv_disp_drv_t* driver, const lv_area_t* area, lv_color_t* colors) {
    const uint32_t width = area->x2 - area->x1 + 1;
    const uint32_t height = area->y2 - area->y1 + 1;
    SharedSPILock bus;
    configASSERT(bus.locked());
    if (!bus.locked()) return;
    s_display->startWrite();
    s_display->setAddrWindow(area->x1, area->y1, width, height);
    s_display->pushPixels((lgfx::swap565_t*)&colors->full, width * height);
    s_display->endWrite();
    handheld::displayFlushed(millis());
    lv_disp_flush_ready(driver);
}

bool Display::begin() {
    if (!initializeSharedSPIBus()) return false;
    SharedSPILock bus;
    if (!bus.locked()) return false;
    _gfx.init();
    _gfx.setRotation(3);
    _gfx.setBrightness(0);
    _gfx.fillScreen(TFT_BLACK);
    Serial.printf("[DISPLAY] M9 ST7789 initialized: %dx%d\n", _gfx.width(), _gfx.height());
    return true;
}

bool Display::beginLVGL() {
    s_display = &_gfx;
    lv_init();

    constexpr uint32_t bufferPixels = TFT_WIDTH * 20;
    s_buffer1 = (lv_color_t*)heap_caps_malloc(
        bufferPixels * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    s_buffer2 = (lv_color_t*)heap_caps_malloc(
        bufferPixels * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!s_buffer1) s_buffer1 = (lv_color_t*)ps_malloc(bufferPixels * sizeof(lv_color_t));
    if (!s_buffer2) s_buffer2 = (lv_color_t*)ps_malloc(bufferPixels * sizeof(lv_color_t));
    if (!s_buffer1 || !s_buffer2) return false;

    static lv_disp_draw_buf_t drawBuffer;
    lv_disp_draw_buf_init(&drawBuffer, s_buffer1, s_buffer2, bufferPixels);

    static lv_disp_drv_t displayDriver;
    lv_disp_drv_init(&displayDriver);
    displayDriver.hor_res = TFT_WIDTH;
    displayDriver.ver_res = TFT_HEIGHT;
    displayDriver.flush_cb = lvglFlush;
    displayDriver.draw_buf = &drawBuffer;
    lv_disp_drv_register(&displayDriver);
    return true;
}

void Display::setBrightness(uint8_t level) {
    _gfx.setBrightness(level);
}

void Display::sleep() {
    SharedSPILock bus;
    if (bus.locked()) _gfx.sleep();
}

void Display::wakeup() {
    SharedSPILock bus;
    if (bus.locked()) _gfx.wakeup();
}
