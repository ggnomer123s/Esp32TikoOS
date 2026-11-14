#pragma once
#include "../utils/Core.h"
#include <vector>
namespace tiko {
class KeyboardModel {
  public:
    std::string text;
    size_t limit = 4096;
    int selection = 0, layout = 0;
    bool multiline = true, secret = false;
    void begin(const std::string &initial, size_t maxBytes, bool lines, bool masked) {
        text = initial;
        limit = maxBytes;
        multiline = lines;
        secret = masked;
        selection = 0;
        layout = 0;
    }
    std::vector<std::string> keys() const {
        static const char *layouts[] = {
            "abcdefghijklmnopqrstuvwxyz0123456789", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
            "абвгдеёжзийклмнопрстуфхцчшщъыьэюя012", "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ012",
            "!@#$%^&*()-_=+[]{};:'\",.<>/?\\|`~0123"};
        std::string s = layouts[layout];
        std::vector<std::string> out;
        size_t i = 0;
        while (i < s.size()) {
            size_t start = i;
            decode(s, i);
            out.push_back(s.substr(start, i - start));
        }
        return out;
    }
    void turn(int d) {
        selection = wrap(selection + d, keys().size() + 6);
    }
    // 0 = changed, 1 = done, 2 = cancel, -1 = limit or unsupported action.
    int click() {
        auto k = keys();
        int n = k.size();
        std::string add;
        if (selection < n)
            add = k[selection];
        else
            switch (selection - n) {
            case 0:
                add = " ";
                break;
            case 1:
                if (!multiline) {
                    text.clear();
                    return 0;
                }
                add = "\n";
                break;
            case 2:
                popUtf8(text);
                return 0;
            case 3:
                layout = (layout + 1) % 5;
                selection = 0;
                return 0;
            case 4:
                return 1;
            case 5:
                return 2;
            }
        if (text.size() + add.size() > limit)
            return -1;
        text += add;
        return 0;
    }
};
} // namespace tiko
