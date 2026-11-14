#include "../src/settings/Config.h"
#include "../src/utils/Core.h"
#include <cassert>
#include <iostream>
int main() {
    int tests = 0;
    auto check = [&](bool v) {
        assert(v);
        ++tests;
    };
    std::string ru = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя";
    check(tiko::validUtf8(ru));
    size_t i = 0;
    unsigned n = 0;
    while (i < ru.size()) {
        check(tiko::decode(ru, i) >= 0x400);
        n++;
    }
    check(n == 66);
    for (std::string bad : {"\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe2\x82", "\x80"})
        check(!tiko::validUtf8(bad));
    for (std::string p : {"/TikoOS", "/TikoOS/notes/Русский.txt", "/TikoOS/a.b/c.txt"})
        check(tiko::safePath(p));
    for (std::string p :
         {"/TikoOS2/a", "/TikoOS/../other", "/TikoOS/./a", "/TikoOS//a", "/TikoOS/a\\b",
          "/TikoOS/%2e%2e/a", "/etc/passwd", "/TikoOS/a\nb", "/TikoOS/"})
        check(!tiko::safePath(p));
    check(tiko::wrap(-1, 8) == 7);
    check(tiko::wrap(8, 8) == 0);
    check(tiko::wrap(5, 0) == 0);
    tiko::Quadrature q;
    q.state = 3;
    for (int s : {1, 0, 2, 3})
        q.feed(s);
    check(q.steps == 1);
    for (int s : {2, 0, 1, 3})
        q.feed(s);
    check(q.steps == 0);
    for (int s : {1, 3, 1, 0, 2, 3})
        q.feed(s);
    check(q.steps == 1);
    q.feed(0);
    check(q.invalid == 1);
    check(q.steps == 1);
    Settings a;
    a.font = 2;
    a.reverse = true;
    a.timeout = 120;
    Settings b;
    check(Settings::decode(a.encode(), b));
    check(b.encode() == a.encode());
    auto damaged = a.encode();
    damaged[2] = 8;
    check(!Settings::decode(damaged, b));
    check(!Settings::decode("", b));
    damaged = a.encode();
    damaged[0] = 2;
    check(!Settings::decode(damaged, b));
    std::cout << tests
              << " assertions passed: UTF-8, paths, quadrature, navigation, settings codec\n";
}
