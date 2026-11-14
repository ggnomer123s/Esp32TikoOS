#pragma once
#include "utils/Core.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <esp_random.h>
#include <storage/storage.h>
#include <string>
#include <vector>
#ifdef malloc
#undef malloc
#endif
namespace tiko_native {
struct Entry {
    std::string path, name;
    bool dir;
    uint64_t size;
};
class Store {
  public:
    Storage *fs = static_cast<Storage *>(furi_record_open(RECORD_STORAGE));
    ~Store() {
        furi_record_close(RECORD_STORAGE);
    }
    static std::string full(const std::string &p) {
        return "/ext" + p;
    }
    bool exists(const std::string &p) {
        return storage_common_exists(fs, full(p).c_str());
    }
    void mkdir(const std::string &p) {
        storage_simply_mkdir(fs, full(p).c_str());
    }
    void folders() {
        for (auto p : {"/TikoOS", "/TikoOS/notes", "/TikoOS/formulas", "/TikoOS/config",
                       "/TikoOS/config/positions", "/TikoOS/favorites"})
            mkdir(p);
    }
    std::string read(const std::string &p, size_t limit, uint32_t offset = 0) {
        File *f = storage_file_alloc(fs);
        std::string out;
        if (storage_file_open(f, full(p).c_str(), FSAM_READ, FSOM_OPEN_EXISTING)) {
            storage_file_seek(f, offset, true);
            out.resize(limit);
            out.resize(storage_file_read(f, out.data(), limit));
        }
        storage_file_close(f);
        storage_file_free(f);
        return out;
    }
    bool write(const std::string &p, const std::string &text, bool replace = false) {
        if (!tiko::safePath(p))
            return false;
        folders();
        std::string temp = "/TikoOS/config/.write-" + std::to_string(esp_random());
        File *f = storage_file_alloc(fs);
        bool ok = storage_file_open(f, full(temp).c_str(), FSAM_WRITE, FSOM_CREATE_NEW);
        if (ok) {
            ok = storage_file_write(f, text.data(), text.size()) == text.size();
            ok = storage_file_sync(f) && ok;
        }
        storage_file_close(f);
        storage_file_free(f);
        if (ok) {
            if (exists(p) && !replace)
                ok = false;
            else
                ok = storage_common_rename(fs, full(temp).c_str(), full(p).c_str()) == FSE_OK;
        }
        if (!ok)
            storage_common_remove(fs, full(temp).c_str());
        return ok;
    }
    bool remove(const std::string &p) {
        return tiko::safePath(p) && storage_common_remove(fs, full(p).c_str()) == FSE_OK;
    }
    std::vector<Entry> list(const std::string &p, size_t skip, bool &more) {
        std::vector<Entry> out;
        File *d = storage_file_alloc(fs);
        more = false;
        if (storage_dir_open(d, full(p).c_str())) {
            FileInfo info;
            char name[256];
            size_t index = 0;
            while (storage_dir_read(d, &info, name, sizeof name)) {
                if (name[0] == '.')
                    continue;
                if (index++ < skip)
                    continue;
                if (out.size() == 64) {
                    more = true;
                    break;
                }
                out.push_back({p + "/" + name, name, file_info_is_dir(&info), info.size});
            }
        }
        storage_dir_close(d);
        storage_file_free(d);
        return out;
    }
    uint64_t size(const std::string &p) {
        FileInfo info{};
        return storage_common_stat(fs, full(p).c_str(), &info) == FSE_OK ? info.size : 0;
    }
    static std::string hash(const std::string &p) {
        uint64_t h = 1469598103934665603ULL;
        for (unsigned char c : p)
            h = (h ^ c) * 1099511628211ULL;
        char s[24];
        snprintf(s, sizeof s, "%llx", (unsigned long long)h);
        return s;
    }
};
} // namespace tiko_native
