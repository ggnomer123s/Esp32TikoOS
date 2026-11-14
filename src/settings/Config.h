#pragma once
#include <cstdint>
#include <string>
struct Settings {
    uint8_t brightness = 160, font = 0, accent = 0, sensitivity = 1;
    bool reverse = false, animations = true;
    uint16_t timeout = 60;
    std::string encode() const {
        return std::string({char(1), char(brightness), char(font), char(accent), char(sensitivity),
                            char(reverse), char(animations), char(timeout & 255),
                            char(timeout >> 8)});
    }
    static bool decode(const std::string &s, Settings &v) {
        if (s.size() != 9 || uint8_t(s[0]) != 1 || uint8_t(s[1]) < 16 || uint8_t(s[2]) > 2 ||
            uint8_t(s[3]) > 2 || uint8_t(s[4]) < 1 || uint8_t(s[4]) > 4 || uint8_t(s[5]) > 1 ||
            uint8_t(s[6]) > 1)
            return false;
        unsigned t = uint8_t(s[7]) + (uint8_t(s[8]) << 8);
        if (t > 600)
            return false;
        v.brightness = uint8_t(s[1]);
        v.font = s[2];
        v.accent = s[3];
        v.sensitivity = s[4];
        v.reverse = s[5];
        v.animations = s[6];
        v.timeout = t;
        return true;
    }
};
