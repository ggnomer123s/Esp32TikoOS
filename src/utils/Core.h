#pragma once
#include <cstdint>
#include <string>
namespace tiko {
inline uint32_t decode(const std::string &s, size_t &i) {
    if (i >= s.size())
        return 0;
    uint8_t b = s[i++];
    if (b < 128)
        return b;
    int n = b >= 0xc2 && b <= 0xdf   ? 1
            : b >= 0xe0 && b <= 0xef ? 2
            : b >= 0xf0 && b <= 0xf4 ? 3
                                     : 0;
    if (!n)
        return 0xfffd;
    uint32_t c = b & ((1 << (6 - n)) - 1);
    size_t start = i;
    for (int k = 0; k < n; k++) {
        if (i >= s.size() || (uint8_t(s[i]) & 0xc0) != 0x80) {
            i = start;
            return 0xfffd;
        }
        c = (c << 6) | (s[i++] & 63);
    }
    if (c < (n == 1 ? 128 : n == 2 ? 2048 : 65536) || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
        return 0xfffd;
    return c;
}
inline bool validUtf8(const std::string &s) {
    size_t i = 0;
    while (i < s.size())
        if (decode(s, i) == 0xfffd)
            return false;
    return true;
}
inline bool safePath(const std::string &p) {
    if (p != "/TikoOS" && p.rfind("/TikoOS/", 0) != 0)
        return false;
    if (p.size() > 220 || !validUtf8(p))
        return false;
    size_t a = 1;
    for (size_t i = 0; i <= p.size(); i++) {
        if (i < p.size() &&
            (uint8_t(p[i]) < 32 || p[i] == '\\' || p[i] == '%' || p[i] == '?' || p[i] == '#'))
            return false;
        if (i == p.size() || p[i] == '/') {
            auto part = p.substr(a, i > a ? i - a : 0);
            if (part == "." || part == "..")
                return false;
            if (i > 1 && i == a)
                return false;
            a = i + 1;
        }
    }
    return p.back() != '/';
}
inline int wrap(int x, int count) {
    return count ? ((x % count) + count) % count : 0;
}
struct Quadrature {
    uint8_t state = 0;
    int sum = 0;
    uint32_t transitions = 0, invalid = 0;
    int steps = 0, last = 0;
    int feed(uint8_t s) {
        static const int8_t t[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
        if (s == state)
            return 0;
        transitions++;
        if ((s ^ state) == 3) {
            invalid++;
            sum = 0;
            state = s;
            return 0;
        }
        sum += t[(state << 2) | s];
        state = s;
        int out = 0;
        if (s == 3) {
            if (sum >= 4)
                out = 1;
            else if (sum <= -4)
                out = -1;
            sum = 0;
        }
        if (out) {
            steps += out;
            last = out;
        }
        return out;
    }
};
} // namespace tiko
