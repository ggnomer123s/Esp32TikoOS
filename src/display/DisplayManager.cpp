#include "DisplayManager.h"
#include "Fonts.h"
static const Glyph *table(int size) {
    return size == 0 ? glyphs14 : size == 1 ? glyphs18 : glyphs22;
}
static const uint8_t *pixels(int size) {
    return size == 0 ? pixels14 : size == 1 ? pixels18 : pixels22;
}
static Glyph glyph(uint32_t c, int size) {
    const Glyph *t = table(size);
    int lo = 0, hi = sizeof(glyphs14) / sizeof(Glyph) - 1;
    while (lo <= hi) {
        int m = (lo + hi) / 2;
        Glyph g;
        memcpy_P(&g, t + m, sizeof g);
        if (g.code == c)
            return g;
        if (g.code < c)
            lo = m + 1;
        else
            hi = m - 1;
    }
    Glyph g;
    memcpy_P(&g, t + sizeof(glyphs14) / sizeof(Glyph) - 1, sizeof g);
    return g;
}
void DisplayManager::begin() {
    tft.init();
    tft.setRotation(3);
    canvas.setColorDepth(16);
    if (!canvas.createSprite(320, 170)) {
        Serial.println("Display allocation failed");
        abort();
    }
    ledcSetup(0, 5000, 8);
    ledcAttachPin(21, 0);
    brightness(160);
}
int DisplayManager::width(uint32_t c, int size) {
    return glyph(c, size).width;
}
void DisplayManager::text(const String &s, int x, int y, uint16_t color, int size, int right) {
    std::string str(s.c_str());
    size_t i = 0;
    int h = (size == 0 ? 14 : size == 1 ? 18 : 22) + 5;
    while (i < str.size()) {
        uint32_t cp = tiko::decode(str, i);
        Glyph g = glyph(cp, size);
        if (x + g.width > right)
            break;
        const auto *data = pixels(size) + g.offset;
        for (int yy = 0; yy < h; yy++) {
            int start = -1;
            for (int xx = 0; xx <= g.width; xx++) {
                int bit = yy * g.width + xx;
                bool on = xx < g.width && (pgm_read_byte(data + bit / 8) & (0x80 >> (bit % 8)));
                if (on && start < 0)
                    start = xx;
                if (!on && start >= 0) {
                    canvas.drawFastHLine(x + start, y + yy, xx - start, color);
                    start = -1;
                }
            }
        }
        x += g.width;
    }
}
std::vector<String> DisplayManager::wrapText(const String &s, int size) {
    std::vector<String> lines;
    String line;
    int w = 0;
    std::string str(s.c_str());
    size_t i = 0;
    while (i < str.size()) {
        size_t begin = i;
        auto c = tiko::decode(str, i);
        if (c == '\r')
            continue;
        if (c == '\n') {
            lines.push_back(line);
            line = "";
            w = 0;
            continue;
        }
        int cw = width(c, size);
        if (w + cw > 300) {
            lines.push_back(line);
            line = "";
            w = 0;
        }
        line += String(str.substr(begin, i - begin).c_str());
        w += cw;
    }
    lines.push_back(line);
    return lines;
}
void DisplayManager::boot(int stage) {
    clear();
    text("TikoOS", 85, 35, accent, 2);
    text("v0.2", 135, 75, fg);
    for (int i = 0; i < stage; i++)
        canvas.fillRoundRect(30 + i * 65, 118, 52, 4, 2, accent);
    show();
}

void DisplayManager::show() {
    // Only changed row bands cross the shared SPI bus; sprite stores byte-swapped RGB565.
    auto *data = static_cast<uint16_t *>(canvas.getPointer());
    int begin = -1;
    tft.startWrite();
    for (int y = 0; y <= 170; y++) {
        bool changed = false;
        if (y < 170) {
            uint32_t hash = 2166136261u;
            auto *row = data + y * 320;
            for (int x = 0; x < 320; x++)
                hash = (hash ^ row[x]) * 16777619u;
            changed = firstFrame || hash != previous[y];
            previous[y] = hash;
        }
        if (changed && begin < 0)
            begin = y;
        if (!changed && begin >= 0) {
            tft.pushImage(0, begin, 320, y - begin, data + begin * 320);
            begin = -1;
        }
    }
    tft.endWrite();
    firstFrame = false;
}
