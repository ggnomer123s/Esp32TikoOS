#pragma once
#include "../apps/Reader.h"
#include "../apps/ToolsApp.h"
#include "../hardware/HardwareManager.h"
#include "../input/InputManager.h"
#include "../network/WiFiTransferApp.h"
class ScreenManager {
    DisplayManager &display;
    StorageManager &storage;
    SettingsManager &settings;
    InputManager &input;
    HardwareManager &hardware;
    FavoritesApp favorites;
    NotesApp reader;
    WiFiTransferApp wifi;
    ToolsApp tools;
    enum Screen {
        Home,
        Browser,
        Reader,
        ReaderMenu,
        Favorites,
        Transfer,
        Tools,
        Calculator,
        Stopwatch,
        Monitor,
        Settings,
        Info,
        Diagnostics,
        FileActions,
        Confirm
    };
    Screen screen = Home, confirmReturn = Browser;
    int selected = 0, calcKey = 0, substeps = 0;
    String directory = "/", browseRoot = "/", message, deletePath;
    std::vector<Entry> entries;
    uint32_t lastInput = 0, lastDraw = 0;
    bool dirty = true, asleep = false;
    int scrollOffset = 0;
    void menu(const String &title, const std::vector<String> &items);
    void browse(const String &p);
    void activate();
    void back();
    void render();
    void setting();

  public:
    ScreenManager(DisplayManager &d, StorageManager &s, SettingsManager &v, InputManager &i,
                  HardwareManager &h)
        : display(d), storage(s), settings(v), input(i), hardware(h), favorites(v), reader(v, d),
          wifi(s) {}
    void begin() {
        favorites.begin();
        lastInput = millis();
    }
    void loop();
};
