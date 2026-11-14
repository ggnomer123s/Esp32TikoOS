#include "ScreenManager.h"
static String filename(const String &p) {
    return p.substring(p.lastIndexOf('/') + 1);
}
void ScreenManager::menu(const String &title, const std::vector<String> &items) {
    display.text(title, 8, 3, display.accent);
    int start = selected / 4 * 4;
    for (int i = start; i < int(items.size()) && i < start + 4; i++) {
        int y = 27 + (i - start) * 29;
        display.card(y, i == selected);
        display.text(items[i], 12, y + 4, i == selected ? 0x0000 : display.fg);
    }
    display.text(String(selected + 1) + "/" + String(items.size()) + "   Rotate · Click · Back", 8,
                 150, display.fg);
}
void ScreenManager::browse(const String &p) {
    directory = p;
    entries = storage.list(p);
    selected = 0;
    screen = Browser;
}
void ScreenManager::back() {
    message = "";
    switch (screen) {
    case ReaderMenu:
        screen = Reader;
        break;
    case Reader:
        selected = 0;
        reader.close();
        if (browseRoot == "favorites")
            screen = Favorites;
        else
            browse(directory);
        break;
    case Browser:
        if (directory == browseRoot || directory == "/") {
            screen = Home;
            selected = 0;
        } else {
            String p = directory.substring(0, directory.lastIndexOf('/'));
            browse(p.length() ? p : "/");
        }
        break;
    case FileActions:
        screen = Browser;
        selected = 0;
        break;
    case Transfer:
        wifi.stop();
        screen = Home;
        selected = 0;
        break;
    case Calculator:
    case Stopwatch:
    case Monitor:
    case Diagnostics:
        screen = Tools;
        selected = 0;
        break;
    case Confirm:
        screen = confirmReturn;
        selected = 0;
        break;
    default:
        screen = Home;
        selected = 0;
        break;
    }
}
void ScreenManager::setting() {
    auto &v = settings.value;
    switch (selected) {
    case 0:
        v.brightness = v.brightness >= 240 ? 32 : v.brightness + 32;
        break;
    case 1:
        v.font = (v.font + 1) % 3;
        break;
    case 2:
        v.accent = (v.accent + 1) % 3;
        break;
    case 3:
        v.reverse = !v.reverse;
        break;
    case 4:
        v.sensitivity = v.sensitivity % 4 + 1;
        break;
    case 5:
        v.timeout = v.timeout == 0 ? 30 : v.timeout == 30 ? 60 : v.timeout == 60 ? 120 : 0;
        break;
    case 6:
        v.animations = !v.animations;
        break;
    case 7:
        wifi.begin();
        screen = Transfer;
        break;
    case 8:
        screen = Info;
        break;
    case 9:
        deletePath = "";
        confirmReturn = Settings;
        screen = Confirm;
        selected = 0;
        return;
    }
    settings.save();
    display.brightness(v.brightness);
}
void ScreenManager::activate() {
    message = "";
    switch (screen) {
    case Home:
        switch (selected) {
        case 0:
            browseRoot = "/TikoOS/notes";
            browse(browseRoot);
            break;
        case 1:
            browseRoot = FormulasApp::root;
            browse(browseRoot);
            break;
        case 2:
            screen = Favorites;
            selected = 0;
            break;
        case 3:
            browseRoot = FileManagerApp::root;
            browse("/");
            break;
        case 4:
            wifi.begin();
            screen = Transfer;
            break;
        case 5:
            screen = Tools;
            selected = 0;
            break;
        case 6:
            screen = Settings;
            selected = 0;
            break;
        case 7:
            screen = Info;
            break;
        }
        break;
    case Browser:
        if (entries.empty())
            break;
        if (entries[selected].directory)
            browse(entries[selected].path);
        else if (entries[selected].path.endsWith(".txt")) {
            if (reader.open(entries[selected].path)) {
                screen = Reader;
                selected = 0;
            } else
                message = "Ошибка чтения";
        } else {
            deletePath = entries[selected].path;
            screen = FileActions;
            selected = 0;
        }
        break;
    case FileActions:
        if (selected == 0) {
            if (tiko::safePath(deletePath.c_str())) {
                confirmReturn = Browser;
                screen = Confirm;
                selected = 0;
            } else
                message = "Удаление только /TikoOS";
        } else {
            screen = Browser;
            selected = 0;
        }
        break;
    case Favorites:
        if (favorites.paths.empty())
            break;
        if (!SD.exists(favorites.paths[selected])) {
            deletePath = favorites.paths[selected];
            confirmReturn = Favorites;
            screen = Confirm;
            selected = 0;
        } else if (reader.open(favorites.paths[selected])) {
            browseRoot = "favorites";
            screen = Reader;
            selected = 0;
        }
        break;
    case Reader:
        screen = ReaderMenu;
        selected = 0;
        break;
    case ReaderMenu:
        switch (selected) {
        case 0:
            if (!favorites.toggle(reader.path))
                message = "Лимит: 24 избранных";
            break;
        case 1:
            settings.value.font = (settings.value.font + 1) % 3;
            settings.save();
            reader.resize();
            screen = Reader;
            break;
        case 2:
            if (tiko::safePath(reader.path.c_str())) {
                deletePath = reader.path;
                confirmReturn = Reader;
                screen = Confirm;
                selected = 0;
            } else
                message = "Удаление только /TikoOS";
            break;
        case 3:
            screen = Reader;
            back();
            break;
        }
        break;
    case Tools:
        screen = selected == 0   ? Calculator
                 : selected == 1 ? Stopwatch
                 : selected == 2 ? Monitor
                                 : Diagnostics;
        selected = 0;
        break;
    case Calculator:
        tools.key(tools.keys[calcKey]);
        break;
    case Stopwatch:
        if (selected == 0)
            tools.toggle();
        else
            tools.reset();
        break;
    case Settings:
        setting();
        break;
    case Confirm:
        if (selected == 1) {
            if (deletePath.isEmpty()) {
                settings.reset();
                display.brightness(settings.value.brightness);
            } else if (confirmReturn == Favorites) {
                favorites.toggle(deletePath);
            } else {
                reader.close();
                bool ok = storage.remove(deletePath);
                if (ok && favorites.contains(deletePath))
                    favorites.toggle(deletePath);
                message = ok ? "Удалено" : "Ошибка удаления";
                browse(directory);
                return;
            }
        }
        screen = confirmReturn;
        selected = 0;
        break;
    default:
        back();
        break;
    }
}
void ScreenManager::render() {
    auto &v = settings.value;
    display.accent = v.accent == 0 ? 0x07ff : v.accent == 1 ? 0x459f : 0xb81f;
    display.clear();
    switch (screen) {
    case Home:
        menu("TikoOS · SD " + String(storage.ready ? "OK" : "--") + " · BAT ?",
             {"Notes", "Formulas", "Favorites", "Files", "Wi-Fi Transfer", "Tools", "Settings",
              "Device Info"});
        break;
    case Browser: {
        std::vector<String> names;
        for (auto &e : entries)
            names.push_back(String(e.directory ? "[+] " : "") + e.name +
                            String(e.directory ? "" : "  " + String(e.size) + " B"));
        if (names.empty())
            names.push_back(storage.ready ? "Нет документов / Back" : "SD отсутствует / Back");
        menu(filename(directory).length() ? filename(directory) : "microSD", names);
        if (storage.truncated)
            display.text("Лимит списка: 128 файлов", 8, 150, display.accent);
        break;
    }
    case Favorites: {
        std::vector<String> names;
        for (auto &p : favorites.paths)
            names.push_back(String(SD.exists(p) ? "" : "[нет] ") + filename(p));
        if (names.empty())
            names.push_back("Избранное пусто / Back");
        menu("Favorites", names);
        break;
    }
    case Reader:
        display.text(filename(reader.path), 8, 3, display.accent);
        reader.render();
        break;
    case ReaderMenu:
        menu("Документ", {favorites.contains(reader.path) ? "Убрать из избранного" : "В избранное",
                          "Размер шрифта", "Удалить файл...", "Закрыть документ"});
        break;
    case Transfer:
        display.text("Wi-Fi Transfer", 8, 3, display.accent);
        display.text(wifi.active ? "AP: TikoOS" : "AP не запущен", 8, 30, display.fg);
        display.text("Пароль: " + wifi.password, 8, 53, display.fg);
        display.text(wifi.status, 8, 76, display.accent);
        display.text("Web login: tiko / пароль AP", 8, 101, display.fg);
        display.text("Back: выключить Wi-Fi", 8, 135, display.fg);
        break;
    case Tools:
        menu("Tools", {"Calculator", "Stopwatch", "System Monitor", "Encoder diagnostics"});
        break;
    case Calculator:
        display.text("Calculator", 8, 3, display.accent);
        display.text(tools.input.length() ? tools.input : String(tools.value, 6), 8, 35, display.fg,
                     1);
        display.text("Op: " + String(tools.op), 8, 68, display.accent);
        display.card(99, true);
        display.text("Выбрать: " + String(tools.keys[calcKey]), 12, 103, 0);
        display.text(tools.error.length() ? tools.error : "Rotate: символ · Click: ввод", 8, 145,
                     display.fg);
        break;
    case Stopwatch: {
        uint32_t ms = tools.time();
        display.text("Stopwatch", 8, 3, display.accent);
        char b[40];
        snprintf(b, sizeof b, "%02lu:%02lu.%02lu", (unsigned long)(ms / 60000),
                 (unsigned long)(ms / 1000 % 60), (unsigned long)(ms / 10 % 100));
        display.text(b, 40, 40, display.fg, 2);
        display.card(90, selected == 0);
        display.text(tools.running ? "Пауза" : "Старт", 12, 94, selected == 0 ? 0 : display.fg);
        display.card(121, selected == 1);
        display.text("Сброс", 12, 125, selected == 1 ? 0 : display.fg);
        break;
    }
    case Monitor:
    case Info: {
        std::vector<String> info = {"Heap: " + String(ESP.getFreeHeap()) + " B",
                                    "PSRAM: " + String(ESP.getFreePsram()) + " B",
                                    "Flash: " + String(ESP.getFlashChipSize() / 1048576) + " MB",
                                    "CPU: " + String(ESP.getCpuFreqMHz()) + " MHz / " +
                                        ESP.getChipModel(),
                                    "Uptime: " + String(millis() / 1000) + " s",
                                    storage.capacity(),
                                    "Battery: Unknown",
                                    String("PN532 I2C: ") + (hardware.pn532 ? "ACK" : "no ACK"),
                                    String("BQ27220: ") + (hardware.gauge ? "ACK" : "no ACK"),
                                    String("BQ25896: ") + (hardware.charger ? "ACK" : "no ACK"),
                                    "CC1101 / nRF24: not probed",
                                    "IR / RGB: inactive",
                                    "TikoOS v0.1 · About"};
        menu(screen == Info ? "Device Info" : "System Monitor", info);
        break;
    }
    case Diagnostics: {
        menu("Encoder raw",
             {"Transitions: " + String(input.decoder.transitions),
              "Invalid: " + String(input.decoder.invalid), "Steps: " + String(input.decoder.steps),
              "Direction: " + String(input.decoder.last), "GPIO0 click / GPIO6 back"});
        break;
    }
    case Settings:
        menu("Settings",
             {"Brightness: " + String(v.brightness), "Font: " + String(v.font + 1),
              "Accent: " + String(v.accent + 1), String("Reverse: ") + (v.reverse ? "on" : "off"),
              "Detents/step: " + String(v.sensitivity), "Timeout: " + String(v.timeout) + " s",
              String("Animations: ") + (v.animations ? "on" : "off"), "Wi-Fi: новый пароль / AP",
              "About TikoOS", "Reset settings..."});
        break;
    case FileActions:
        menu(filename(deletePath), {"Удалить файл...", "Назад"});
        break;
    case Confirm:
        menu(deletePath.length() ? "Удалить? " + filename(deletePath) : "Сбросить настройки?",
             {"Нет / отмена", confirmReturn == Favorites ? "Убрать ссылку" : "Да, подтверждаю"});
        break;
    }
    if (message.length()) {
        display.card(120, false);
        display.text(message, 8, 124, display.accent);
    }
    display.show();
    lastDraw = millis();
    dirty = false;
}
void ScreenManager::loop() {
    wifi.loop();
    auto e = input.poll();
    bool activity = e.turn || e.click || e.back;
    if (activity) {
        lastInput = millis();
        if (asleep) {
            asleep = false;
            display.brightness(settings.value.brightness);
            return;
        }
    }
    Screen old = screen;
    if (e.back)
        back();
    else if (e.click == 2) {
        if (screen == Browser && !entries.empty() && !entries[selected].directory) {
            deletePath = entries[selected].path;
            screen = FileActions;
            selected = 0;
        } else
            back();
    } else if (e.click == 1)
        activate();
    if (e.turn) {
        int t = settings.value.reverse ? -e.turn : e.turn;
        substeps += t;
        t = substeps / settings.value.sensitivity;
        substeps %= settings.value.sensitivity;
        if (screen == Reader)
            reader.turn(t);
        else if (screen == Calculator)
            calcKey = tiko::wrap(calcKey + t, tools.keys.length());
        else {
            int count = 1;
            switch (screen) {
            case Home:
                count = 8;
                break;
            case Browser:
                count = entries.size();
                break;
            case Favorites:
                count = favorites.paths.size();
                break;
            case ReaderMenu:
                count = 4;
                break;
            case Tools:
                count = 4;
                break;
            case Stopwatch:
            case Confirm:
            case FileActions:
                count = 2;
                break;
            case Settings:
                count = 10;
                break;
            case Monitor:
            case Info:
                count = 13;
                break;
            case Diagnostics:
                count = 5;
                break;
            default:
                break;
            }
            selected = tiko::wrap(selected + t, count);
        }
    }
    if (old != screen) {
        substeps = 0;
        if (settings.value.animations) { /* Brief fade uses PWM; never delays event processing. */
            scrollOffset = millis();
        }
    }
    if (settings.value.animations && scrollOffset) {
        uint32_t age = millis() - uint32_t(scrollOffset);
        if (age < 160)
            display.brightness(settings.value.brightness * (80 + age) / 240);
        else {
            scrollOffset = 0;
            display.brightness(settings.value.brightness);
        }
    }
    if (activity)
        dirty = true;
    if (screen == Stopwatch || screen == Monitor || screen == Diagnostics)
        if (millis() - lastDraw > 150)
            dirty = true;
    if (settings.value.timeout && millis() - lastInput > settings.value.timeout * 1000UL) {
        if (!asleep) {
            display.brightness(0);
            asleep = true;
        }
    }
    if (dirty && !asleep)
        render();
}
