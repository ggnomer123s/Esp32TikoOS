#pragma once
#include "../display/DisplayManager.h"
#include "../storage/StorageManager.h"
class NotesApp {
    File file;
    SettingsManager &settings;
    DisplayManager &display;
    uint32_t offset = 0, next = 0;
    uint32_t page = 0;
    std::vector<String> lines;
    int row = 0;
    void load() {
        offset = page * 768;
        file.seek(offset);
        while (file.available() && (file.peek() & 0xc0) == 0x80) {
            file.read();
            offset++;
        }
        String text;
        while (file.available() && file.position() < (page + 1) * 768)
            text += char(file.read());
        while (file.available() && (file.peek() & 0xc0) == 0x80)
            text += char(file.read());
        next = (page + 1) * 768;
        lines = display.wrapText(text, settings.value.font);
        row = 0;
    }

  public:
    String path;
    bool opened = false;
    NotesApp(SettingsManager &s, DisplayManager &d) : settings(s), display(d) {}
    bool open(const String &p) {
        close();
        file = SD.open(p);
        if (!file || file.isDirectory())
            return false;
        path = p;
        opened = true;
        page = settings.get("r" + settings.key(p)).toInt();
        if (page > file.size() / 768)
            page = 0;
        load();
        row = constrain(settings.get("l" + settings.key(p)).toInt(), 0, int(lines.size()) - 1);
        return true;
    }
    void close() {
        if (opened) {
            settings.put("r" + settings.key(path), String(page));
            settings.put("l" + settings.key(path), String(row));
            file.close();
        }
        opened = false;
    }
    void turn(int d) {
        if (!opened)
            return;
        int step = d > 0 ? 1 : -1;
        for (int n = 0; n < abs(d) && n < 16; n++) {
            row += step;
            if (row >= int(lines.size())) {
                if (next < file.size()) {
                    page++;
                    load();
                } else
                    row = lines.size() - 1;
            }
            if (row < 0) {
                if (page) {
                    page--;
                    load();
                    row = lines.size() - 1;
                } else
                    row = 0;
            }
        }
    }
    void resize() {
        load();
    }
    void render() {
        int size = settings.value.font, h = (size == 0 ? 14 : size == 1 ? 18 : 22) + 5;
        for (int j = row, y = 27; j < int(lines.size()) && y + h < 149; j++, y += h)
            display.text(lines[j], 9, y, display.fg, size);
        display.text(String(offset) + " / " + String(file.size()) + " B", 8, 150, display.accent);
    }
};
class FormulasApp {
  public:
    static constexpr const char *root = "/TikoOS/formulas";
};
class FileManagerApp {
  public:
    static constexpr const char *root = "/";
};
