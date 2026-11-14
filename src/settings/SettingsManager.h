#pragma once
#include "Config.h"
#include <Preferences.h>
class SettingsManager {
    Preferences prefs;

  public:
    Settings value;
    void begin() {
        prefs.begin("tiko-v1", false);
        char data[9];
        if (prefs.getBytesLength("settings") == sizeof data) {
            prefs.getBytes("settings", data, sizeof data);
            Settings::decode(std::string(data, sizeof data), value);
        }
    }
    void save() {
        auto data = value.encode();
        prefs.putBytes("settings", data.data(), data.size());
    }
    void reset() {
        value = Settings{};
        save();
    }
    String get(const String &key) {
        return prefs.getString(key.c_str(), "");
    }
    void put(const String &key, const String &v) {
        prefs.putString(key.c_str(), v);
    }
    String key(const String &path) {
        uint32_t hash = 2166136261u;
        for (unsigned i = 0; i < path.length(); i++)
            hash = (hash ^ uint8_t(path[i])) * 16777619u;
        return String(hash, 16);
    }
};
