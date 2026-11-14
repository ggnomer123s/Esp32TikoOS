#pragma once
#include "../utils/Core.h"
#include <TFT_eSPI.h>
#include <vector>
class DisplayManager {
    TFT_eSPI tft;
    TFT_eSprite canvas{&tft};
    uint32_t previous[170]{};
    bool firstFrame = true;

  public:
    uint16_t bg = 0x1083, fg = 0xffff, accent = 0x07ff;
    int font = 0;
    void begin();
    void brightness(int v) {
        ledcWrite(0, v);
    }
    void clear() {
        canvas.fillSprite(bg);
    }
    void text(const String &s, int x, int y, uint16_t color, int size = 0, int right = 318);
    int width(uint32_t c, int size);
    void card(int y, bool selected) {
        canvas.fillRoundRect(4, y, 312, 28, 7, selected ? accent : 0x2105);
    }
    void marker(int y, int width) {
        canvas.drawFastHLine(5, y, width, fg);
    }
    void line(int y) {
        canvas.drawFastHLine(8, y, 304, 0x3186);
    }
    void show();
    void box(int x, int y, int w, int h, bool active) {
        canvas.fillRoundRect(x, y, w, h, 3, active ? accent : 0x2105);
    }
    std::vector<String> wrapText(const String &s, int size);
    void boot(int stage);
};
