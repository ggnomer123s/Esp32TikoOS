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
        if (i == selected && settings.value.animations)
            display.marker(y + 26, std::min<uint32_t>(120, millis() - lastInput) * 310 / 120);
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
        if (browseRoot == "favorites") {
            refreshFavorites();
            screen = Favorites;
        } else
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
    case NetworkMenu:
    case WiFiList:
        wifi.stop();
        scanning = false;
        screen = Home;
        selected = 0;
        break;
    case KeyboardScreen:
        screen = (purpose == NoteName || purpose == NoteBody) ? Browser : NetworkMenu;
        selected = 0;
        break;
    case USBConfirm:
        screen = Home;
        selected = 0;
        break;
    case Transfer:
        wifi.stop();
        screen = NetworkMenu;
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
        screen = NetworkMenu;
        selected = 0;
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
            refreshFavorites();
            screen = Favorites;
            selected = 0;
            break;
        case 3:
            browseRoot = FileManagerApp::root;
            browse("/");
            break;
        case 4:
            screen = NetworkMenu;
            selected = 0;
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
            selected = 0;
            break;
        case 8:
            screen = USBConfirm;
            selected = 0;
            break;
        }
        break;
    case Browser: {
        if (noteCreation() && selected == 0) {
            noteFolder = directory;
            startKeyboard(NoteName);
            break;
        }
        int index = selected - (noteCreation() ? 1 : 0);
        if (index < 0 || index >= int(entries.size()))
            break;
        auto entry = entries[index];
        if (entry.directory)
            browse(entry.path);
        else if (entry.path.endsWith(".txt")) {
            if (reader.open(entry.path)) {
                screen = Reader;
                selected = 0;
            } else
                message = "Ошибка чтения";
        } else {
            deletePath = entry.path;
            screen = FileActions;
            selected = 0;
        }
        break;
    }
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
    case NetworkMenu:
        if (selected == 0 || selected == 1) {
            wifi.begin(selected == 1);
            screen = Transfer;
            selected = 0;
        } else if (selected == 2) {
            wifi.scan();
            networks.clear();
            scanning = true;
            scanStarted = millis();
            screen = WiFiList;
            selected = 0;
        } else
            startKeyboard(APPassword);
        break;
    case WiFiList:
        if (selected == 0)
            startKeyboard(SSID);
        else if (selected <= int(networks.size())) {
            pendingSSID = networks[selected - 1];
            startKeyboard(STAPassword);
        }
        break;
    case KeyboardScreen: {
        int result = keyboard.model.click();
        keyboard.error = "";
        if (result == 1)
            finishKeyboard();
        else if (result == 2)
            back();
        else if (result < 0)
            keyboard.error = "Достигнут лимит";
        break;
    }
    case USBConfirm:
        if (selected == 1) {
            reader.close();
            wifi.stop();
            screen = USBTransfer;
            usbPending = true;
        } else
            back();
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
              "Device Info", "USB Transfer"});
        break;
    case Browser: {
        std::vector<String> names;
        if (noteCreation())
            names.push_back("+ Новая заметка");
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
        auto names = favoriteLabels;
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
    case NetworkMenu:
        menu("Wi-Fi Transfer", {"AP: точка TikoOS", "My network: " + settings.get("sta-ssid"),
                                "Подключить Wi-Fi...", "Пароль AP / WebUI..."});
        break;
    case WiFiList: {
        auto names = networks;
        names.insert(names.begin(), "Ввести SSID вручную...");
        menu(scanning ? "Поиск Wi-Fi..." : "Выберите сеть", names);
        break;
    }
    case KeyboardScreen:
        keyboard.render(display);
        break;
    case USBConfirm:
        menu("USB: доступ ко всей карте", {"Отмена", "Подключить USB-диск"});
        display.text("Сначала закройте файлы на ПК", 8, 113, display.fg);
        break;
    case USBTransfer:
        display.text("USB microSD", 8, 3, display.accent);
        display.text("Карта доступна компьютеру", 8, 35, display.fg);
        display.text("Файлы открывайте на ПК", 8, 60, display.fg);
        display.text("Безопасно извлеките диск", 8, 90, display.accent);
        display.text("После Eject: перезагрузка", 8, 118, display.fg);
        display.text("Не отключайте при записи", 8, 147, display.fg);
        break;
    case Transfer:
        display.text("Wi-Fi Transfer", 8, 3, display.accent);
        display.text(wifi.connected() ? "My network" : "AP / подключение", 8, 30, display.fg);
        display.text("Пароль Web: " + wifi.password, 8, 53, display.fg);
        display.text(wifi.status, 8, 76, display.accent);
        display.text("Web login: tiko", 8, 101, display.fg);
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
                                    "TikoOS v0.2 · About"};
        menu(screen == Info ? "Device Info" : "System Monitor", info);
        break;
    }
    case Diagnostics: {
        auto raw = input.snapshot();
        menu("Encoder raw",
             {"Transitions: " + String(raw.transitions), "Invalid: " + String(raw.invalid),
              "Steps: " + String(raw.steps), "Direction: " + String(raw.last),
              "Queue drops: " + String(input.overflows.load()), "GPIO0 click / GPIO6 back"});
        break;
    }
    case Settings:
        menu("Settings",
             {"Brightness: " + String(v.brightness), "Font: " + String(v.font + 1),
              "Accent: " + String(v.accent + 1), String("Reverse: ") + (v.reverse ? "on" : "off"),
              "Detents/step: " + String(v.sensitivity), "Timeout: " + String(v.timeout) + " s",
              String("Animations: ") + (v.animations ? "on" : "off"), "Wi-Fi / подключение...",
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
void ScreenManager::rotate(int amount) {
    int t = settings.value.reverse ? -amount : amount;
    substeps += t;
    t = substeps / settings.value.sensitivity;
    substeps %= settings.value.sensitivity;
    if (!t)
        return;
    if (screen == Reader)
        reader.turn(t);
    else if (screen == Calculator)
        calcKey = tiko::wrap(calcKey + t, tools.keys.length());
    else if (screen == KeyboardScreen)
        keyboard.model.turn(t);
    else {
        int count = 1;
        switch (screen) {
        case Home:
            count = 9;
            break;
        case Browser:
            count = entries.size() + (noteCreation() ? 1 : 0);
            break;
        case Favorites:
            count = favorites.paths.size();
            break;
        case ReaderMenu:
        case Tools:
        case NetworkMenu:
            count = 4;
            break;
        case Stopwatch:
        case Confirm:
        case FileActions:
        case USBConfirm:
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
            count = 6;
            break;
        case WiFiList:
            count = networks.size() + 1;
            break;
        default:
            break;
        }
        selected = tiko::wrap(selected + t, count);
    }
}
void ScreenManager::event(InputManager::Event e) {
    lastInput = millis();
    if (asleep) {
        asleep = false;
        display.brightness(settings.value.brightness);
    }
    Screen old = screen;
    // Apply queued movement BEFORE click, never to the screen opened by that click.
    rotate(e.turn);
    if (e.back)
        back();
    else if (e.click == 2) {
        int index = selected - (noteCreation() ? 1 : 0);
        if (screen == Browser && index >= 0 && index < int(entries.size()) &&
            !entries[index].directory) {
            deletePath = entries[index].path;
            screen = FileActions;
            selected = 0;
        } else
            back();
    } else if (e.click == 1)
        activate();
    if (old != screen)
        substeps = 0;
    dirty = true;
}
void ScreenManager::loop() {
    if (usb.active || usb.ejected) {
        while (true) {
            auto e = input.poll();
            if (!e.turn && !e.click && !e.back)
                break;
        }
        if (usb.ejected) {
            delay(100);
            ESP.restart();
        }
        return;
    }
    wifi.loop();
    if (scanning) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
            networks.clear();
            for (int i = 0; i < n && networks.size() < 24; i++) {
                String ssid = WiFi.SSID(i);
                if (ssid.length() &&
                    std::find(networks.begin(), networks.end(), ssid) == networks.end())
                    networks.push_back(ssid);
            }
            WiFi.scanDelete();
            scanning = false;
            selected = 0;
            dirty = true;
        } else if (millis() - scanStarted > 15000) {
            scanning = false;
            message = "Поиск завершён: SSID вручную";
            dirty = true;
        }
    }
    for (int i = 0; i < 32; i++) {
        auto e = input.poll();
        if (!e.turn && !e.click && !e.back)
            break;
        event(e);
        if (usbPending)
            break;
    }
    if ((screen == Stopwatch || screen == Monitor || screen == Diagnostics || screen == Transfer) &&
        millis() - lastDraw >= 250)
        dirty = true;
    if (settings.value.timeout && screen != Transfer && screen != KeyboardScreen &&
        screen != USBTransfer && millis() - lastInput > settings.value.timeout * 1000UL) {
        if (!asleep) {
            display.brightness(0);
            asleep = true;
        }
    }
    if (settings.value.animations && millis() - lastInput < 120)
        dirty = true;
    // At most one frame every 16 ms, independent of how many events have accumulated.
    if (dirty && !asleep && millis() - lastDraw >= 16)
        render();
    if (usbPending && !dirty) {
        usbPending = false;
        if (!usb.start(storage)) {
            screen = Home;
            selected = 0;
            message = "USB: нужна SD, сектор 512";
            dirty = true;
        }
    }
}
void ScreenManager::startKeyboard(Purpose p) {
    scanning = false;
    purpose = p;
    screen = KeyboardScreen;
    selected = 0;
    switch (p) {
    case SSID:
        keyboard.begin("Имя Wi-Fi (SSID)", settings.get("sta-ssid"), 32);
        break;
    case STAPassword:
        keyboard.begin("Пароль Wi-Fi", "", 63, false, true);
        break;
    case APPassword: {
        String value = settings.get("ap-pass");
        keyboard.begin("Пароль AP / Web", value.length() ? value : "Tigran123456!", 63, false,
                       true);
        break;
    }
    case NoteName:
        keyboard.begin("Название заметки", "", 80);
        break;
    case NoteBody:
        keyboard.begin("Текст заметки", "", 8192, true);
        break;
    }
}
void ScreenManager::finishKeyboard() {
    String value = keyboard.model.text.c_str();
    switch (purpose) {
    case SSID:
        if (value.isEmpty()) {
            keyboard.error = "Введите SSID";
            return;
        }
        pendingSSID = value;
        startKeyboard(STAPassword);
        break;
    case STAPassword:
        if (value.length() && value.length() < 8) {
            keyboard.error = "Пароль: 8–63 байта / пусто";
            return;
        }
        settings.put("sta-ssid", pendingSSID);
        settings.put("sta-pass", value);
        wifi.begin(true);
        screen = Transfer;
        selected = 0;
        break;
    case APPassword:
        if (value.length() < 8) {
            keyboard.error = "Пароль: от 8 байт";
            return;
        }
        settings.put("ap-pass", value);
        screen = NetworkMenu;
        selected = 0;
        message = "Пароль AP / Web сохранён";
        break;
    case NoteName: {
        if (!tiko::safeLeaf(value.c_str())) {
            keyboard.error = "Недопустимое имя файла";
            return;
        }
        if (!value.endsWith(".txt"))
            value += ".txt";
        String path = noteFolder + "/" + value;
        if (!tiko::safePath(path.c_str()) || SD.exists(path)) {
            keyboard.error = "Имя занято / слишком длинно";
            return;
        }
        noteName = value;
        startKeyboard(NoteBody);
        break;
    }
    case NoteBody: {
        if (!storage.ready) {
            keyboard.error = "SD недоступна";
            return;
        }
        String path = noteFolder + "/" + noteName;
        if (!tiko::safePath(path.c_str()) || SD.exists(path)) {
            keyboard.error = "Файл уже существует";
            return;
        }
        if (!SD.exists("/TikoOS"))
            SD.mkdir("/TikoOS");
        if (!SD.exists("/TikoOS/notes"))
            SD.mkdir("/TikoOS/notes");
        String temp = noteFolder + "/.note-" + String(esp_random(), 16);
        File f = SD.open(temp, FILE_WRITE);
        if (!f) {
            keyboard.error = "Не удалось создать файл";
            return;
        }
        bool ok = f.write(reinterpret_cast<const uint8_t *>(value.c_str()), value.length()) ==
                  value.length();
        f.flush();
        f.close();
        if (ok)
            ok = SD.rename(temp, path);
        if (!ok) {
            SD.remove(temp);
            keyboard.error = "Ошибка записи / SD заполнена";
            return;
        }
        browse(noteFolder);
        message = "Заметка сохранена";
        break;
    }
    }
}
