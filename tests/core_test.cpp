#include "../src/input/KeyboardModel.h"
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

    // Button samples continue during display/network work; bounce must not duplicate clicks.
    tiko::DebouncedButton button;
    check(button.sample(false, 100) == 0);
    check(button.sample(true, 105) == 0);
    check(button.sample(false, 108) == 0);
    check(button.sample(false, 129) == 0);
    check(button.sample(true, 190) == 0);
    check(button.sample(true, 211) == 1);
    check(button.sample(true, 220) == 0);
    check(button.sample(false, 300) == 0);
    check(button.sample(false, 321) == 0);
    check(button.sample(false, 971) == 2);
    check(button.sample(true, 990) == 0);
    check(button.sample(true, 1011) == 0);
    // Overflow of millis must not lose release debounce.
    tiko::DebouncedButton wrapButton;
    check(wrapButton.sample(false, 0xfffffff0u) == 0);
    check(wrapButton.sample(false, 8) == 0);
    check(wrapButton.sample(true, 20) == 0);
    check(wrapButton.sample(true, 41) == 1);
    tiko::KeyboardModel keyboard;
    for (int layout = 0; layout < 5; layout++) {
        keyboard.layout = layout;
        check(keyboard.keys().size() == 36);
        for (auto &key : keyboard.keys())
            check(tiko::validUtf8(key));
    }
    keyboard.layout = 0;
    keyboard.begin("", 8, false, true);
    keyboard.selection = 0;
    check(keyboard.click() == 0);
    check(keyboard.text == "a");
    keyboard.selection = keyboard.keys().size() + 3;
    check(keyboard.click() == 0);
    check(keyboard.layout == 1);
    keyboard.layout = 2;
    keyboard.selection = 0;
    check(keyboard.click() == 0);
    check(keyboard.text == "aа");
    keyboard.selection = keyboard.keys().size() + 2;
    keyboard.click();
    check(keyboard.text == "a");
    keyboard.selection = keyboard.keys().size() + 1;
    keyboard.click();
    check(keyboard.text.empty());
    keyboard.begin("ё", 2, false, false);
    keyboard.selection = 0;
    check(keyboard.click() == -1);
    check(keyboard.text == "ё");
    keyboard.selection = keyboard.keys().size() + 2;
    keyboard.click();
    check(keyboard.text.empty());
    keyboard.begin("текст", 8192, true, false);
    keyboard.selection = keyboard.keys().size() + 1;
    keyboard.click();
    check(keyboard.text == "текст\n");
    keyboard.selection = keyboard.keys().size() + 4;
    check(keyboard.click() == 1);
    keyboard.selection = keyboard.keys().size() + 5;
    check(keyboard.click() == 2);
    keyboard.turn(100);
    check(keyboard.selection >= 0 && keyboard.selection < int(keyboard.keys().size() + 6));
    check(tiko::safeLeaf("Конспект 1.txt"));
    for (const std::string name :
         {"../abc", "a/b", ".hidden", "x.", "x ", "a:b", "a*b", "a|b", "a\\b"})
        check(!tiko::safeLeaf(name));
    check(tiko::sectorRange(0, 0, 512, 1));
    check(tiko::sectorRange(0, 128, 384, 1));
    check(tiko::sectorRange(1, 0, 512, 2));
    check(!tiko::sectorRange(1, 0, 513, 2));
    check(!tiko::sectorRange(0, 512, 1, 1));
    check(!tiko::sectorRange(0xffffffffu, 0, 512, 1));
    check(tiko::sectorRange(0, 0, 1024, 2));
    std::cout << tests
              << " assertions passed: UTF-8, paths, quadrature, buttons, keyboard, USB bounds, "
                 "navigation, settings codec\n";
}
