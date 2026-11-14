#pragma once
#include "../settings/SettingsManager.h"
#include "../utils/Core.h"
#include <SD.h>
#include <TFT_eSPI.h>
#include <algorithm>
#include <vector>
struct Entry {
    String path, name;
    bool directory;
    size_t size;
};
class StorageManager {

  public:
    bool ready = false;
    bool truncated = false;
    String cachedCapacity;
    void begin() {
        ready = SD.begin(13, TFT_eSPI::getSPIinstance(), 4000000) && SD.cardType() != CARD_NONE;
    }
    std::vector<Entry> list(const String &path) {
        std::vector<Entry> out;
        truncated = false;
        if (!ready)
            return out;
        File dir = SD.open(path);
        if (!dir || !dir.isDirectory())
            return out;
        for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
            String n = f.name();
            int pos = n.lastIndexOf('/');
            if (pos >= 0)
                n = n.substring(pos + 1);
            if (n.startsWith("."))
                continue;
            if (out.size() == 128) {
                truncated = true;
                break;
            }
            out.push_back({path == "/" ? "/" + n : path + "/" + n, n, f.isDirectory(), f.size()});
        }
        std::sort(out.begin(), out.end(), [](const Entry &a, const Entry &b) {
            return a.directory != b.directory ? a.directory : a.name < b.name;
        });
        return out;
    }
    bool remove(const String &p) {
        return ready && tiko::safePath(p.c_str()) && SD.remove(p);
    }
    String capacity() {
        if (cachedCapacity.length())
            return cachedCapacity;
        if (!ready)
            return "SD отсутствует";
        return String(SD.cardSize() / 1048576) + " MB SD";
    }
    String freeSpace() {
        return cachedCapacity = ready
                                    ? String((SD.totalBytes() - SD.usedBytes()) / 1048576) +
                                          " MB free / " + String(SD.totalBytes() / 1048576) + " MB"
                                    : "SD отсутствует";
    }
};
class FavoritesApp {
    SettingsManager &settings;

  public:
    std::vector<String> paths;
    explicit FavoritesApp(SettingsManager &s) : settings(s) {}
    void begin() {
        String s = settings.get("favorites");
        int a = 0;
        for (unsigned i = 0; i <= s.length(); i++)
            if (i == s.length() || s[i] == '\n') {
                String p = s.substring(a, i);
                if (tiko::safePath(p.c_str()) && paths.size() < 24)
                    paths.push_back(p);
                a = i + 1;
            }
    }
    bool contains(const String &p) {
        return std::find(paths.begin(), paths.end(), p) != paths.end();
    }
    bool toggle(const String &p) {
        auto i = std::find(paths.begin(), paths.end(), p);
        if (i != paths.end())
            paths.erase(i);
        else {
            if (paths.size() >= 24)
                return false;
            paths.push_back(p);
        }
        String s;
        for (auto &p : paths)
            s += p + "\n";
        settings.put("favorites", s);
        return true;
    }
};
