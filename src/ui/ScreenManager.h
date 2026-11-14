#pragma once
#include "../apps/Reader.h"
#include "../apps/ToolsApp.h"
#include "../hardware/HardwareManager.h"
#include "../input/InputManager.h"
#include "../network/USBTransferApp.h"
#include "../network/WiFiTransferApp.h"
#include "Keyboard.h"
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
    USBTransferApp usb;
    Keyboard keyboard;
    enum Purpose { SSID, STAPassword, APPassword, NoteName, NoteBody };
    Purpose purpose = SSID;
    String pendingSSID, noteName, noteFolder;
    std::vector<String> networks;
    bool scanning = false, usbPending = false;
    uint32_t scanStarted = 0;
    void startKeyboard(Purpose p);
    void finishKeyboard();
    bool noteCreation() const {
        return storage.ready && browseRoot == "/TikoOS/notes";
    }
    void rotate(int amount);
    void event(InputManager::Event e);
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
        Confirm,
        NetworkMenu,
        WiFiList,
        KeyboardScreen,
        USBConfirm,
        USBTransfer
    };
    Screen screen = Home, confirmReturn = Browser;
    int selected = 0, calcKey = 0, substeps = 0;
    String directory = "/", browseRoot = "/", message, deletePath;
    std::vector<Entry> entries;
    uint32_t lastInput = 0, lastDraw = 0;
    bool dirty = true, asleep = false;
    std::vector<String> favoriteLabels;
    void refreshFavorites() {
        favoriteLabels.clear();
        for (auto &p : favorites.paths)
            favoriteLabels.push_back(String(storage.ready && SD.exists(p) ? "" : "[нет] ") +
                                     p.substring(p.lastIndexOf('/') + 1));
    }
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
          wifi(s, v) {}
    void begin() {
        usb.configure();
        favorites.begin();
        refreshFavorites();
        lastInput = millis();
    }
    void loop();
};
