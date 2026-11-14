// TikoOS applications on the upstream Furi GUI/Input/Storage and radio services.
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_sd.h>
#include <gui/gui.h>
#include <gui/modules/submenu.h>
#include <gui/view_dispatcher.h>
#include <nvs.h>
#include <string>
#include <vector>
extern "C" {
#include "webfs.h"
#include <wifi.h>
#include <wlan_hal.h>
#include <wlan_passwords.h>
int32_t archive_app(void *);
int32_t bt_settings_app(void *);
int32_t interface_settings_app(void *);
int32_t notification_settings_app(void *);
int32_t about_app(void *);
int32_t clock_app(void *);
}
#ifdef malloc
#undef malloc
#endif
#include "../../targets/lilygo_t_embed_cc1101/tiko_input.h"
#include "NativeFonts.h"
#include "input/KeyboardModel.h"
#include "store.hpp"
namespace {
using tiko_native::Entry;
using tiko_native::Store;
const NativeGlyph *glyph(uint32_t cp, int font) {
    const NativeGlyph *t = font == 0   ? native_glyphs7
                           : font == 1 ? native_glyphs9
                                       : native_glyphs11;
    size_t n = sizeof(native_glyphs7) / sizeof(NativeGlyph);
    for (size_t i = 0; i < n; i++)
        if (t[i].code == cp)
            return t + i;
    return t + n - 1;
}
void drawText(Canvas *c, const char *str, int x, int y, int font = 0, int right = 127) {
    std::string s = str;
    size_t i = 0;
    int h = (font == 0 ? 7 : font == 1 ? 9 : 11) + 3;
    const uint8_t *pixels = font == 0   ? native_pixels7
                            : font == 1 ? native_pixels9
                                        : native_pixels11;
    while (i < s.size()) {
        auto *g = glyph(tiko::decode(s, i), font);
        if (x + g->width > right)
            break;
        for (int yy = 0; yy < h; yy++)
            for (int xx = 0; xx < g->width; xx++) {
                int bit = yy * g->width + xx;
                if (pixels[g->offset + bit / 8] & (0x80 >> (bit % 8)))
                    canvas_draw_dot(c, x + xx, y + yy);
            }
        x += g->width;
    }
}
std::string legacyString(const char *key) {
    nvs_handle_t handle;
    std::string out;
    if (nvs_open("tiko-v1", NVS_READONLY, &handle) == ESP_OK) {
        size_t count = 0;
        if (nvs_get_str(handle, key, nullptr, &count) == ESP_OK && count > 0 && count < 8193) {
            out.resize(count);
            if (nvs_get_str(handle, key, out.data(), &count) == ESP_OK)
                out.resize(count - 1);
            else
                out.clear();
        }
        nvs_close(handle);
    }
    return out;
}
std::string legacyKey(const std::string &path, char prefix) {
    uint32_t h = 2166136261u;
    for (unsigned char c : path)
        h = (h ^ c) * 16777619u;
    char key[16];
    snprintf(key, sizeof key, "%c%lx", prefix, (unsigned long)h);
    return key;
}
struct Render {
    char title[96], lines[5][192], footer[96], keys[42][12];
    int font, selection;
    bool keyboard;
};
class App {
    Store store;
    Gui *gui;
    ViewDispatcher *dispatcher;
    Submenu *menu;
    View *detail;
    enum Mode {
        Home,
        Directory,
        Favorites,
        Document,
        DocMenu,
        Tools,
        Calculator,
        Monitor,
        Network,
        Transfer,
        Connecting,
        Scanning,
        Keyboard,
        Settings,
        Diagnostics,
        Confirm,
        ResetConfirm,
        MissingFavorite
    };
    enum Purpose { SSID, StaPassword, ApPassword, NoteName, NoteBody };
    Mode mode = Home;
    Purpose purpose = SSID;
    std::string directory, root, document, noteFolder, noteName, pendingSSID,
        apPassword = "Tigran123456!", notice;
    std::vector<Entry> entries;
    std::vector<std::string> favorites, lines;
    uint64_t documentSize = 0;
    uint32_t page = 0;
    int row = 0, font = 1;
    bool encoderReverse = false;
    unsigned encoderSensitivity = 1;
    size_t directoryPage = 0;
    bool more = false, fromFavorites = false;
    tiko::KeyboardModel keyboard;
    double calcValue = 0;
    std::string calcInput, calcError;
    char operation = '+';
    int calcKey = 0;
    const std::string calcKeys = "0123456789.+-*/=C<";
    uint32_t connectStarted = 0;
    bool connectSaved = false;
    FuriThread *scanThread = nullptr;
    volatile bool scanCancel = false;
    std::atomic<bool> scanDone{false};
    wifi_ap_record_t *scanResults = nullptr;
    uint16_t scanCount = 0;
    std::vector<std::string> ssids;
    std::string bookmark() {
        return "/TikoOS/config/positions/" + Store::hash(document) + ".txt";
    }
    static std::string leaf(const std::string &p) {
        return p.substr(p.find_last_of('/') + 1);
    }
    void item(const std::string &s, uint32_t id) {
        submenu_add_item(
            menu, s.c_str(), id,
            [](void *p, uint32_t i) {
                auto *a = static_cast<App *>(p);
                view_dispatcher_send_custom_event(a->dispatcher, i);
            },
            this);
    }
    void menuStart(const char *title, Mode next) {
        mode = next;
        submenu_reset(menu);
        submenu_set_header(menu, notice.empty() ? title : notice.c_str());
    }
    void menuShow() {
        view_dispatcher_switch_to_view(dispatcher, 0);
    }
    void render(std::string title, std::vector<std::string> text = {},
                std::string footer = "Back: назад", bool isKeyboard = false) {
        auto *m = static_cast<Render *>(view_get_model(detail));
        memset(m, 0, sizeof *m);
        snprintf(m->title, sizeof m->title, "%s", title.c_str());
        for (size_t i = 0; i < text.size() && i < 5; i++)
            snprintf(m->lines[i], sizeof m->lines[i], "%s", text[i].c_str());
        snprintf(m->footer, sizeof m->footer, "%s", (notice.empty() ? footer : notice).c_str());
        m->font = mode == Document ? font : 0;
        m->keyboard = isKeyboard;
        if (isKeyboard && !notice.empty())
            snprintf(m->title, sizeof m->title, "%s", notice.c_str());
        if (isKeyboard) {
            auto keys = keyboard.keys();
            keys.insert(keys.end(),
                        {"sp", keyboard.multiline ? "nl" : "clr", "<-", "EN", "OK", "X"});
            for (size_t i = 0; i < keys.size(); i++)
                snprintf(m->keys[i], sizeof m->keys[i], "%s", keys[i].c_str());
            m->selection = keyboard.selection;
        }
        view_commit_model(detail, true);
        view_dispatcher_switch_to_view(dispatcher, 1);
    }
    static void draw(Canvas *c, void *data) {
        auto *m = static_cast<Render *>(data);
        canvas_clear(c);
        drawText(c, m->title, 1, 0);
        canvas_draw_line(c, 0, 10, 127, 10);
        if (m->keyboard) {
            drawText(c, m->lines[0], 0, 11);
            for (int i = 0; i < 42; i++) {
                int x = i < 36 ? (i % 12) * 10 : (i - 36) * 21,
                    y = i < 36 ? 23 + (i / 12) * 10 : 54;
                bool selected = i == m->selection;
                if (selected) {
                    canvas_draw_box(c, x, y, i < 36 ? 10 : 21, 10);
                    canvas_set_color(c, ColorWhite);
                }
                drawText(c, m->keys[i], x + 1, y);
                canvas_set_color(c, ColorBlack);
            }
        } else {
            int h = m->font == 0 ? 10 : m->font == 1 ? 12 : 14;
            for (int i = 0; i < 5 && 14 + i * h + h < 56; i++)
                drawText(c, m->lines[i], 1, 13 + i * h, m->font);
            drawText(c, m->footer, 0, 55);
        }
    }
    static bool input(InputEvent *e, void *p) {
        if (e->key == InputKeyBack)
            return false;
        if (e->type == InputTypeShort || e->type == InputTypeRepeat || e->type == InputTypeLong) {
            auto *a = static_cast<App *>(p);
            view_dispatcher_send_custom_event(a->dispatcher,
                                              0x10000 | (uint32_t(e->type) << 8) | e->key);
            return true;
        }
        return false;
    }
    void home() {
        notice = "";
        menuStart("TikoOS v0.3", Home);
        const char *names[] = {"Notes",          "Formulas",  "Favorites", "Files / Archive",
                               "Wi-Fi Transfer", "Bluetooth", "Tools",     "Settings"};
        for (int i = 0; i < 8; i++)
            item(names[i], i);
        menuShow();
    }
    void saveFavorites() {
        std::string s;
        for (auto &p : favorites)
            s += p + "\n";
        if (!store.write("/TikoOS/favorites/list.txt", s, true))
            notice = "Ошибка сохранения";
    }
    void loadFavorites() {
        std::string s = store.read("/TikoOS/favorites/list.txt", 8192);
        if (!store.exists("/TikoOS/favorites/list.txt")) {
            nvs_handle_t handle;
            if (nvs_open("tiko-v1", NVS_READONLY, &handle) == ESP_OK) {
                size_t n = 0;
                if (nvs_get_str(handle, "favorites", nullptr, &n) == ESP_OK && n > 0 && n <= 8192) {
                    s.resize(n);
                    if (nvs_get_str(handle, "favorites", s.data(), &n) == ESP_OK) {
                        s.resize(n - 1);
                        store.write("/TikoOS/favorites/list.txt", s);
                    } else
                        s.clear();
                }
                nvs_close(handle);
            }
        }
        size_t a = 0;
        while (a < s.size()) {
            size_t b = s.find('\n', a);
            if (b == std::string::npos)
                b = s.size();
            auto p = s.substr(a, b - a);
            if (tiko::safePath(p) && favorites.size() < 24)
                favorites.push_back(p);
            a = b + 1;
        }
    }
    void list() {
        menuStart(leaf(directory).c_str(), Directory);
        entries = store.list(directory, directoryPage * 64, more);
        if (root == "/TikoOS/notes")
            item("+ Новая заметка", 2003);
        for (size_t i = 0; i < entries.size(); i++)
            item(std::string(entries[i].dir ? "[+] " : "") + entries[i].name, 1000 + i);
        if (directoryPage)
            item("< Предыдущие 64", 2001);
        if (more)
            item("Следующие 64 >", 2002);
        if (entries.empty())
            item("Нет документов", 2999);
        menuShow();
    }
    void showFavorites() {
        menuStart("Favorites", Favorites);
        for (size_t i = 0; i < favorites.size(); i++)
            item((store.exists(favorites[i]) ? "" : "[нет] ") + leaf(favorites[i]), 1000 + i);
        if (favorites.empty())
            item("Избранное пусто", 2999);
        menuShow();
    }
    void savePosition() {
        if (!document.empty())
            store.write(bookmark(), std::to_string(page) + " " + std::to_string(row), true);
    }
    void loadPage() {
        std::string bytes = store.read(document, 771, page * 768);
        size_t first = 0;
        while (first < bytes.size() && (uint8_t(bytes[first]) & 0xc0) == 0x80)
            first++;
        size_t end = std::min<size_t>(768, bytes.size());
        while (end < bytes.size() && (uint8_t(bytes[end]) & 0xc0) == 0x80)
            end++;
        bytes = bytes.substr(first, end - first);
        lines.clear();
        std::string line;
        size_t i = 0;
        int width = 0;
        while (i < bytes.size()) {
            size_t start = i;
            auto cp = tiko::decode(bytes, i);
            if (cp == '\r')
                continue;
            if (cp == '\n') {
                lines.push_back(line);
                line = "";
                width = 0;
                continue;
            }
            int w = glyph(cp, font)->width;
            if (width + w > 124) {
                lines.push_back(line);
                line = "";
                width = 0;
            }
            line += bytes.substr(start, i - start);
            width += w;
        }
        lines.push_back(line);
        row = 0;
    }
    void readView() {
        mode = Document;
        std::vector<std::string> text;
        int count = font == 0 ? 4 : font == 1 ? 3 : 2;
        for (int i = row; i < int(lines.size()) && int(text.size()) < count; i++)
            text.push_back(lines[i]);
        render(leaf(document), text, "OK: меню | " + std::to_string(page * 768) + " B");
    }
    void open(const std::string &p) {
        if (!store.exists(p)) {
            notice = "Файл удалён";
            return;
        }
        document = p;
        documentSize = store.size(document);
        page = 0;
        row = 0;
        auto saved = store.read(bookmark(), 64);
        unsigned pg = 0;
        int r = 0;
        if (saved.empty()) {
            pg = strtoul(legacyString(legacyKey(document, 'r').c_str()).c_str(), nullptr, 10);
            r = atoi(legacyString(legacyKey(document, 'l').c_str()).c_str());
        } else
            sscanf(saved.c_str(), "%u %d", &pg, &r);
        page = pg;
        if (uint64_t(page) * 768 >= documentSize)
            page = 0;
        loadPage();
        row = std::max(0, std::min(r, int(lines.size()) - 1));
        readView();
    }
    void readerMenu() {
        menuStart("Документ", DocMenu);
        item(std::find(favorites.begin(), favorites.end(), document) != favorites.end()
                 ? "Убрать избранное"
                 : "В избранное",
             0);
        item("Размер шрифта", 1);
        item("Удалить файл...", 2);
        item("Закрыть", 3);
        menuShow();
    }
    void toolsMenu() {
        menuStart("Tools", Tools);
        item("Calculator", 0);
        item("Clock / stopwatch", 1);
        item("System monitor", 2);
        item("Encoder diagnostics", 3);
        menuShow();
    }
    void settingsMenu() {
        menuStart("Settings", Settings);
        item("System interface", 0);
        item("Brightness / timeout", 1);
        item(std::string("Encoder reverse: ") + (encoderReverse ? "on" : "off"), 2);
        item("Detents/step: " + std::to_string(encoderSensitivity), 3);
        item("Reader font: " + std::to_string(font + 1), 4);
        item("About TikoOS", 5);
        item("Reset reader / encoder...", 6);
        menuShow();
    }
    void saveEncoder() {
        tiko_input_configure(encoderReverse, encoderSensitivity);
        if (!store.write("/TikoOS/config/encoder.txt",
                         std::to_string(encoderReverse) + " " + std::to_string(encoderSensitivity),
                         true))
            notice = "Нет SD / не сохранено";
    }
    void networkMenu() {
        menuStart("Wi-Fi / native service", Network);
        item("AP: TikoOS", 0);
        item("My network", 1);
        item("Подключить Wi-Fi", 2);
        item("Пароль AP / Web", 3);
        item("Выключить Wi-Fi", 4);
        menuShow();
    }
    void keyboardStart(Purpose p) {
        purpose = p;
        mode = Keyboard;
        std::string text = p == ApPassword ? apPassword : "";
        keyboard.begin(text,
                       p == NoteBody   ? 8192
                       : p == NoteName ? 80
                       : p == SSID     ? 32
                                       : 63,
                       p == NoteBody, p == StaPassword || p == ApPassword);
        keyboardView();
    }
    void keyboardView() {
        std::string text = keyboard.secret
                               ? std::string(std::min<size_t>(keyboard.text.size(), 24), '*')
                               : keyboard.text;
        size_t offset = text.size() > 24 ? text.size() - 24 : 0;
        while (offset < text.size() && (uint8_t(text[offset]) & 0xc0) == 0x80)
            offset++;
        text = text.substr(offset);
        std::replace(text.begin(), text.end(), '\n', ' ');
        const char *titles[] = {"Wi-Fi SSID", "Wi-Fi пароль", "AP / Web пароль", "Имя заметки",
                                "Текст заметки"};
        render(titles[purpose], {text}, "", true);
    }
    void keyboardDone() {
        const std::string value = keyboard.text;
        if (purpose == ApPassword) {
            if (value.size() < 8) {
                notice = "Минимум 8 байт";
                keyboardView();
                return;
            }
            if (!store.write("/TikoOS/config/ap.password", value, true)) {
                notice = "Нет SD / ошибка записи";
                keyboardView();
                return;
            }
            apPassword = value;
            networkMenu();
        } else if (purpose == SSID) {
            if (value.empty()) {
                notice = "Нужен SSID";
                keyboardView();
                return;
            }
            pendingSSID = value;
            keyboardStart(StaPassword);
        } else if (purpose == StaPassword) {
            if (!value.empty() && value.size() < 8) {
                notice = "Минимум 8 / пустой";
                keyboardView();
                return;
            }
            if (!wlan_password_save(pendingSSID.c_str(), value.c_str()))
                notice = "Пароль не сохранён на SD";
            if (!wlan_hal_start() ||
                !wlan_hal_connect(pendingSSID.c_str(), value.c_str(), nullptr, 0)) {
                notice = "Ошибка подключения";
                networkMenu();
                return;
            }
            connectStarted = furi_get_tick();
            connectSaved = false;
            mode = Connecting;
            render("Wi-Fi", {"Подключение...", pendingSSID});
        } else if (purpose == NoteName) {
            if (!tiko::safeLeaf(value)) {
                notice = "Неверное имя";
                keyboardView();
                return;
            }
            noteName = value;
            if (noteName.size() < 4 || noteName.substr(noteName.size() - 4) != ".txt")
                noteName += ".txt";
            if (store.exists(noteFolder + "/" + noteName)) {
                notice = "Имя уже занято";
                keyboardView();
                return;
            }
            keyboardStart(NoteBody);
        } else {
            if (!store.write(noteFolder + "/" + noteName, value)) {
                notice = "Ошибка SD / имя занято";
                keyboardView();
                return;
            }
            directory = noteFolder;
            list();
        }
    }
    static int32_t scan(void *p) {
        auto *a = static_cast<App *>(p);
        wlan_hal_scan_cancellable(&a->scanResults, &a->scanCount, 32, &a->scanCancel);
        a->scanDone = true;
        return 0;
    }
    void startScan() {
        if (!wlan_hal_start()) {
            notice = "Wi-Fi не запущен";
            networkMenu();
            return;
        }
        mode = Scanning;
        render("Wi-Fi", {"Поиск сетей...", "Back: отменить"});
        scanCancel = false;
        scanDone = false;
        scanThread = furi_thread_alloc_ex("TikoScan", 4096, scan, this);
        furi_thread_start(scanThread);
    }
    void stopScan() {
        if (scanThread) {
            scanCancel = true;
            furi_thread_join(scanThread);
            furi_thread_free(scanThread);
            scanThread = nullptr;
        }
        free(scanResults);
        scanResults = nullptr;
        scanCount = 0;
        scanDone = false;
    }
    void calc(char c) {
        calcError = "";
        if (c == 'C') {
            calcInput = "";
            calcValue = 0;
            operation = '+';
            return;
        }
        if (c == '<') {
            if (!calcInput.empty())
                calcInput.pop_back();
            return;
        }
        if ((c >= '0' && c <= '9') || c == '.') {
            if (calcInput.size() < 16 && (c != '.' || calcInput.find('.') == std::string::npos))
                calcInput += c;
            return;
        }
        if (!calcInput.empty()) {
            double b = atof(calcInput.c_str());
            if (operation == '/' && b == 0) {
                calcError = "Деление на ноль";
                return;
            }
            switch (operation) {
            case '+':
                calcValue += b;
                break;
            case '-':
                calcValue -= b;
                break;
            case '*':
                calcValue *= b;
                break;
            case '/':
                calcValue /= b;
                break;
            }
            calcInput = "";
            if (!std::isfinite(calcValue)) {
                calcValue = 0;
                calcError = "Переполнение";
            }
        }
        operation = c == '=' ? '+' : c;
    }
    void toolView() {
        if (mode == Calculator) {
            char value[64];
            snprintf(value, sizeof value, "%.8g", calcValue);
            render("Calculator",
                   {calcInput.empty() ? value : calcInput,
                    "Клавиша: " + std::string(1, calcKeys[calcKey]), calcError},
                   "Вращение / OK");
        } else if (mode == Diagnostics) {
            auto raw = tiko_input_snapshot();
            render("Encoder raw",
                   {"Transitions: " + std::to_string(raw.transitions),
                    "Invalid: " + std::to_string(raw.invalid),
                    "Steps: " + std::to_string(raw.steps), "AB: " + std::to_string(raw.ab)});
        } else {
            render("System monitor",
                   {"Heap: " + std::to_string(esp_get_free_heap_size()),
                    "PSRAM: " + std::to_string(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)),
                    "Uptime: " + std::to_string(esp_timer_get_time() / 1000000) + " s",
                    "SD: " + std::string(furi_hal_sd_get_card_state() == FuriStatusOk
                                             ? "OK"
                                             : "нет данных")});
        }
    }
    void tick() {
        if (mode == Monitor || mode == Diagnostics)
            toolView();
        if (mode == Connecting) {
            if (wlan_hal_is_connected()) {
                if (!connectSaved) {
                    auto *w = static_cast<Wifi *>(furi_record_open(RECORD_WIFI));
                    wifi_mark_connected(w, pendingSSID.c_str());
                    furi_record_close(RECORD_WIFI);
                    connectSaved = true;
                }
                networkMenu();
            } else if (furi_get_tick() - connectStarted > furi_ms_to_ticks(20000)) {
                notice = "Нет связи / пароль неверен";
                networkMenu();
            }
        }
        if (mode == Scanning && scanDone) {
            ssids.clear();
            for (unsigned i = 0; i < scanCount; i++) {
                std::string s = reinterpret_cast<char *>(scanResults[i].ssid);
                if (!s.empty() && std::find(ssids.begin(), ssids.end(), s) == ssids.end())
                    ssids.push_back(s);
            }
            stopScan();
            menuStart("Выберите Wi-Fi", Scanning);
            item("Ввести SSID", 0);
            for (size_t i = 0; i < ssids.size(); i++)
                item(ssids[i], i + 1);
            menuShow();
        }
    }
    void back() {
        notice = "";
        switch (mode) {
        case Home:
            view_dispatcher_stop(dispatcher);
            break;
        case Document:
            savePosition();
            if (fromFavorites)
                showFavorites();
            else
                list();
            break;
        case DocMenu:
            readView();
            break;
        case Directory:
            if (directory == root)
                home();
            else {
                directory = directory.substr(0, directory.find_last_of('/'));
                directoryPage = 0;
                list();
            }
            break;
        case Transfer:
            tiko_web_stop();
            networkMenu();
            break;
        case Connecting:
            wlan_hal_disconnect();
            networkMenu();
            break;
        case Scanning:
            stopScan();
            networkMenu();
            break;
        case Keyboard:
            if (purpose == NoteName || purpose == NoteBody)
                list();
            else
                networkMenu();
            break;
        case Calculator:
        case Monitor:
        case Diagnostics:
            toolsMenu();
            break;
        case ResetConfirm:
            settingsMenu();
            break;
        case MissingFavorite:
            showFavorites();
            break;
        case Confirm:
            readerMenu();
            break;
        default:
            home();
            break;
        }
    }
    void event(uint32_t e) {
        notice = "";
        if (e & 0x10000) {
            InputKey key = InputKey(e & 255);
            InputType type = InputType((e >> 8) & 255);
            int delta = (key == InputKeyDown || key == InputKeyRight) ? 1
                        : (key == InputKeyUp || key == InputKeyLeft)  ? -1
                                                                      : 0;
            if (mode == Document) {
                if (delta) {
                    row += delta;
                    if (row < 0) {
                        if (page) {
                            page--;
                            loadPage();
                            row = lines.size() - 1;
                        } else
                            row = 0;
                    }
                    if (row >= int(lines.size())) {
                        if (uint64_t(page + 1) * 768 < documentSize) {
                            page++;
                            loadPage();
                        } else
                            row = lines.size() - 1;
                    }
                    readView();
                } else if (key == InputKeyOk && type == InputTypeShort)
                    readerMenu();
            } else if (mode == Keyboard) {
                if (delta)
                    keyboard.turn(delta);
                else if (key == InputKeyOk && type == InputTypeShort) {
                    int result = keyboard.click();
                    if (result == 1) {
                        keyboardDone();
                        return;
                    }
                    if (result < 0)
                        notice = "Достигнут лимит";
                    if (result == 2) {
                        back();
                        return;
                    }
                }
                keyboardView();
            } else if (mode == Calculator) {
                if (delta)
                    calcKey = tiko::wrap(calcKey + delta, calcKeys.size());
                else if (key == InputKeyOk && type == InputTypeShort)
                    calc(calcKeys[calcKey]);
                toolView();
            }
            return;
        }
        switch (mode) {
        case Home:
            if (e < 2) {
                root = e == 0 ? "/TikoOS/notes" : "/TikoOS/formulas";
                directory = root;
                directoryPage = 0;
                fromFavorites = false;
                list();
            } else if (e == 2)
                showFavorites();
            else if (e == 3) {
                delegate = 1;
                view_dispatcher_stop(dispatcher);
            } else if (e == 4)
                networkMenu();
            else if (e == 5) {
                delegate = 2;
                view_dispatcher_stop(dispatcher);
            } else if (e == 6)
                toolsMenu();
            else if (e == 7)
                settingsMenu();
            break;
        case Directory:
            if (e == 2001) {
                directoryPage--;
                list();
            } else if (e == 2002) {
                directoryPage++;
                list();
            } else if (e == 2003) {
                noteFolder = directory;
                keyboardStart(NoteName);
            } else if (e >= 1000 && e < 1000 + entries.size()) {
                auto entry = entries[e - 1000];
                if (entry.dir) {
                    directory = entry.path;
                    directoryPage = 0;
                    list();
                } else if (entry.name.size() >= 4 &&
                           entry.name.substr(entry.name.size() - 4) == ".txt")
                    open(entry.path);
            }
            break;
        case Favorites:
            if (e >= 1000 && e < 1000 + favorites.size()) {
                auto path = favorites[e - 1000];
                if (store.exists(path)) {
                    fromFavorites = true;
                    open(path);
                } else {
                    document = path;
                    menuStart("Файл удалён: убрать ссылку?", MissingFavorite);
                    item("Нет", 0);
                    item("Убрать из Favorites", 1);
                    menuShow();
                }
            }
            break;
        case DocMenu:
            if (e == 0) {
                auto i = std::find(favorites.begin(), favorites.end(), document);
                if (i != favorites.end())
                    favorites.erase(i);
                else if (favorites.size() < 24)
                    favorites.push_back(document);
                else
                    notice = "Лимит: 24 избранных";
                saveFavorites();
                readerMenu();
            } else if (e == 1) {
                font = (font + 1) % 3;
                store.write("/TikoOS/config/font.txt", std::to_string(font), true);
                loadPage();
                readView();
            } else if (e == 2) {
                menuStart("Удалить файл?", Confirm);
                item("Нет", 0);
                item("Да, удалить", 1);
                menuShow();
            } else {
                savePosition();
                if (fromFavorites)
                    showFavorites();
                else
                    list();
            }
            break;
        case Confirm:
            if (e == 1) {
                if (store.remove(document)) {
                    favorites.erase(std::remove(favorites.begin(), favorites.end(), document),
                                    favorites.end());
                    saveFavorites();
                    if (fromFavorites)
                        showFavorites();
                    else
                        list();
                } else {
                    notice = "Ошибка удаления";
                    readerMenu();
                }
            } else
                readView();
            break;
        case Tools:
            if (e == 1) {
                delegate = 6;
                view_dispatcher_stop(dispatcher);
            } else {
                mode = e == 0 ? Calculator : e == 2 ? Monitor : Diagnostics;
                toolView();
            }
            break;
        case ResetConfirm:
            if (e == 1) {
                font = 1;
                encoderReverse = false;
                encoderSensitivity = 1;
                saveEncoder();
                store.write("/TikoOS/config/font.txt", "1", true);
            }
            settingsMenu();
            break;
        case MissingFavorite:
            if (e == 1) {
                favorites.erase(std::remove(favorites.begin(), favorites.end(), document),
                                favorites.end());
                saveFavorites();
            }
            showFavorites();
            break;
        case Settings:
            if (e == 6) {
                menuStart("Сбросить настройки?", ResetConfirm);
                item("Нет", 0);
                item("Да: шрифт / энкодер", 1);
                menuShow();
                break;
            }
            if (e == 0 || e == 1 || e == 5) {
                delegate = e == 0 ? 3 : e == 1 ? 4 : 5;
                view_dispatcher_stop(dispatcher);
            } else {
                if (e == 2)
                    encoderReverse = !encoderReverse;
                else if (e == 3)
                    encoderSensitivity = encoderSensitivity % 4 + 1;
                else if (e == 4) {
                    font = (font + 1) % 3;
                    store.write("/TikoOS/config/font.txt", std::to_string(font), true);
                }
                saveEncoder();
                settingsMenu();
                submenu_set_selected_item(menu, e);
            }
            break;
        case Network:
            if (e < 2) {
                if (e == 1 && !wlan_hal_is_connected()) {
                    notice = "Сначала подключите Wi-Fi";
                    networkMenu();
                    break;
                }
                bool ok = tiko_web_start(e == 0, apPassword.c_str());
                char ip[32] = "";
                tiko_web_ip(ip, sizeof ip);
                mode = Transfer;
                render("Wi-Fi Transfer",
                       {ok ? "http://" + std::string(ip) : "Ошибка сервера", "Login: tiko",
                        apPassword, e == 0 ? "AP: TikoOS" : "My network"});
            } else if (e == 2)
                startScan();
            else if (e == 3)
                keyboardStart(ApPassword);
            else {
                auto *w = static_cast<Wifi *>(furi_record_open(RECORD_WIFI));
                wifi_disable(w);
                furi_record_close(RECORD_WIFI);
                networkMenu();
            }
            break;
        case Scanning:
            if (e == 0)
                keyboardStart(SSID);
            else if (e <= ssids.size()) {
                pendingSSID = ssids[e - 1];
                keyboardStart(StaPassword);
            }
            break;
        default:
            break;
        }
    }

  public:
    int delegate = 0;
    App() {
        gui = static_cast<Gui *>(furi_record_open(RECORD_GUI));
        dispatcher = view_dispatcher_alloc();
        menu = submenu_alloc();
        detail = view_alloc();
        view_set_context(detail, this);
        view_allocate_model(detail, ViewModelTypeLocking, sizeof(Render));
        view_set_draw_callback(detail, draw);
        view_set_input_callback(detail, input);
        view_dispatcher_add_view(dispatcher, 0, submenu_get_view(menu));
        view_dispatcher_add_view(dispatcher, 1, detail);
        view_dispatcher_set_event_callback_context(dispatcher, this);
        view_dispatcher_set_custom_event_callback(dispatcher, [](void *p, uint32_t e) {
            static_cast<App *>(p)->event(e);
            return true;
        });
        view_dispatcher_set_navigation_event_callback(dispatcher, [](void *p) {
            static_cast<App *>(p)->back();
            return true;
        });
        view_dispatcher_set_tick_event_callback(
            dispatcher, [](void *p) { static_cast<App *>(p)->tick(); }, 250);
        view_dispatcher_attach_to_gui(dispatcher, gui, ViewDispatcherTypeFullscreen);
        store.folders();
        loadFavorites();
        auto pass = store.read("/TikoOS/config/ap.password", 64);
        if (pass.empty()) {
            pass = legacyString("ap-pass");
            if (pass.size() >= 8 && pass.size() <= 63)
                store.write("/TikoOS/config/ap.password", pass);
        }
        if (pass.size() >= 8 && pass.size() <= 63)
            apPassword = pass;
        auto fontStr = store.read("/TikoOS/config/font.txt", 8);
        if (!fontStr.empty())
            font = std::max(0, std::min(2, atoi(fontStr.c_str())));
        auto encoderSaved = store.read("/TikoOS/config/encoder.txt", 32);
        unsigned reverse = 0, sense = 1;
        sscanf(encoderSaved.c_str(), "%u %u", &reverse, &sense);
        encoderReverse = reverse != 0;
        encoderSensitivity = std::max(1u, std::min(4u, sense));
        tiko_input_configure(encoderReverse, encoderSensitivity);
        home();
    }
    void run() {
        view_dispatcher_run(dispatcher);
    }
    ~App() {
        stopScan();
        tiko_web_stop();
        view_dispatcher_remove_view(dispatcher, 0);
        view_dispatcher_remove_view(dispatcher, 1);
        submenu_free(menu);
        view_free(detail);
        view_dispatcher_free(dispatcher);
        furi_record_close(RECORD_GUI);
    }
};
} // namespace
extern "C" int32_t tiko_app(void *) {
    int next;
    do {
        auto *a = new App;
        a->run();
        next = a->delegate;
        delete a;
        if (next == 1)
            archive_app(nullptr);
        else if (next == 2)
            bt_settings_app(nullptr);
        else if (next == 3)
            interface_settings_app(nullptr);
        else if (next == 4)
            notification_settings_app(nullptr);
        else if (next == 5)
            about_app(nullptr);
        else if (next == 6)
            clock_app(nullptr);
    } while (next);
    return 0;
}
