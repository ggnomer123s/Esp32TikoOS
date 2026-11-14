#include "policy.h"
#include "utils/Core.h"
#include <cstring>
#include <string>
bool tiko_policy_path(const char *path) {
    if (!path || strncmp(path, "/ext", 4) != 0)
        return false;
    std::string p = path + 4;
    if (!tiko::safePath(p))
        return false;
    size_t a = 1;
    while (a < p.size()) {
        auto b = p.find('/', a);
        if (b == std::string::npos)
            b = p.size();
        if (!tiko::safeLeaf(p.substr(a, b - a)))
            return false;
        a = b + 1;
    }
    return true;
}
bool tiko_policy_upload(const char *path, uint64_t bytes) {
    if (!tiko_policy_path(path) || bytes > 2 * 1024 * 1024)
        return false;
    std::string p = path;
    return p.size() >= 4 && p.substr(p.size() - 4) == ".txt";
}

bool tiko_url_decode(const char *raw, char *out, size_t size) {
    if (!raw || !out || !size)
        return false;
    size_t o = 0;
    for (size_t i = 0; raw[i];) {
        unsigned char c = raw[i++];
        if (c == '%') {
            auto hex = [](char x) -> int {
                return x >= '0' && x <= '9'   ? x - '0'
                       : x >= 'a' && x <= 'f' ? x - 'a' + 10
                       : x >= 'A' && x <= 'F' ? x - 'A' + 10
                                              : -1;
            };
            if (!raw[i] || !raw[i + 1])
                return false;
            int a = hex(raw[i]), b = hex(raw[i + 1]);
            if (a < 0 || b < 0)
                return false;
            c = (a << 4) | b;
            i += 2;
        }
        if (!c || o + 1 >= size)
            return false;
        out[o++] = c;
    }
    out[o] = 0;
    return true;
}
