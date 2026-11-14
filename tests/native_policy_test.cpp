#include "../firmware/components/tiko/NativeFonts.h"
#include "../firmware/components/tiko/policy.h"
#include "../src/utils/Core.h"
#include <cassert>
#include <cstring>
#include <iostream>
int main() {
    unsigned checks = 0;
    auto check = [&](bool b) {
        assert(b);
        checks++;
    };
    for (auto p : {"/ext/TikoOS", "/ext/TikoOS/notes/Физика/Тепло.txt",
                   "/ext/TikoOS/formulas/Geometry/Circle.txt"})
        check(tiko_policy_path(p));
    for (auto p :
         {"/ext", "/ext/TikoOS2", "/ext/TikoOS/../wifi/password.txt", "/ext/TikoOS/a/../../outside",
          "/ext/TikoOS//a", "/ext/TikoOS/.upload-x", "/ext/TikoOS/a.", "/ext/TikoOS/a\\b",
          "/ext/TikoOS/a:b", "/ext/TikoOS/%2e%2e", "/ext/TikoOS/a\nb"})
        check(!tiko_policy_path(p));
    check(tiko_policy_upload("/ext/TikoOS/notes/test.txt", 2 * 1024 * 1024));
    check(!tiko_policy_upload("/ext/TikoOS/notes/test.txt", 2 * 1024 * 1024 + 1));
    check(!tiko_policy_upload("/ext/TikoOS/notes/test.bin", 1));
    char out[256];
    check(tiko_url_decode("/ext/TikoOS/notes/a%20b.txt", out, sizeof out));
    check(strcmp(out, "/ext/TikoOS/notes/a b.txt") == 0);
    for (auto s : {"/ext/TikoOS/%00../evil", "abc%", "abc%f", "abc%xx"})
        check(!tiko_url_decode(s, out, sizeof out));
    check(!tiko_url_decode("long", out, 3));
    check(!tiko_url_decode("", out, 0));
    check(tiko_url_decode("/ext/TikoOS/%2e%2e/wifi", out, sizeof out));
    check(!tiko_policy_path(out));
    check(tiko_url_decode("/ext/TikoOS/%252e%252e/wifi", out, sizeof out));
    check(!tiko_policy_path(out));
    const char *alphabet =
        "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя ±×÷√πΩΔ∞≈≠≤≥°²³αβγ№←→";
    std::string text = alphabet;
    size_t i = 0;
    while (i < text.size()) {
        auto cp = tiko::decode(text, i);
        for (auto table : {native_glyphs7, native_glyphs9, native_glyphs11}) {
            bool found = false;
            for (size_t j = 0; j < sizeof(native_glyphs7) / sizeof(NativeGlyph); j++)
                if (table[j].code == cp) {
                    found = true;
                    check(table[j].width > 0);
                }
            check(found);
        }
    }
    std::cout << checks
              << " assertions passed: native path policy, strict URL decode, upload limits and "
                 "Cyrillic/math glyphs\n";
}
