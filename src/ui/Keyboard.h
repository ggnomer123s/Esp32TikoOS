#pragma once
#include "../display/DisplayManager.h"
#include "../input/KeyboardModel.h"
class Keyboard {
  public:
    tiko::KeyboardModel model;
    String title, error;
    void begin(const String &name, const String &initial, size_t maxBytes, bool multiline = false,
               bool secret = false) {
        title = name;
        error = "";
        model.begin(initial.c_str(), maxBytes, multiline, secret);
    }
    void render(DisplayManager &d) {
        d.text(title, 5, 0, d.accent);
        d.text(String(model.text.size()) + "/" + String(model.limit), 240, 0, d.fg);
        String preview;
        if (model.secret) {
            for (size_t i = 0; i < std::min<size_t>(model.text.size(), 32); i++)
                preview += '*';
        } else {
            std::string tail = model.text;
            size_t offset = tail.size() > 90 ? tail.size() - 90 : 0;
            while (offset < tail.size() && (uint8_t(tail[offset]) & 0xc0) == 0x80)
                offset++;
            tail = tail.substr(offset);
            for (char &c : tail)
                if (c == '\n')
                    c = ' ';
            preview = tail.c_str();
        }
        auto lines = d.wrapText(preview, 0);
        int first = std::max(0, int(lines.size()) - 2);
        for (int i = first; i < int(lines.size()); i++)
            d.text(lines[i], 6, 22 + (i - first) * 18, d.fg);
        auto keys = model.keys();
        for (int i = 0; i < int(keys.size()); i++) {
            int x = 3 + (i % 12) * 26, y = 65 + (i / 12) * 24;
            bool focus = i == model.selection;
            d.box(x, y, 24, 22, focus);
            d.text(keys[i].c_str(), x + 5, y + 1, focus ? 0 : d.fg);
        }
        const char *labels[] = {"Space", model.multiline ? "Enter" : "Clear", "<-", "EN/RU", "OK",
                                "Cancel"};
        for (int i = 0; i < 6; i++) {
            bool focus = model.selection == int(keys.size()) + i;
            d.box(3 + i * 53, 140, 51, 27, focus);
            d.text(labels[i], 5 + i * 53, 144, focus ? 0 : d.fg);
        }
        if (error.length()) {
            d.box(0, 22, 320, 21, false);
            d.text(error, 5, 23, d.accent);
        }
    }
};
