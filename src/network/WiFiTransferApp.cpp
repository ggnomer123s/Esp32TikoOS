#include "WiFiTransferApp.h"
#include <esp_system.h>
static String randomHex() {
    String s;
    for (int i = 0; i < 4; i++) {
        char b[9];
        snprintf(b, sizeof b, "%08lx", (unsigned long)esp_random());
        s += b;
    }
    return s;
}
static String html(const String &s) {
    String o;
    for (unsigned i = 0; i < s.length(); i++) {
        char c = s[i];
        switch (c) {
        case '&':
            o += "&amp;";
            break;
        case '<':
            o += "&lt;";
            break;
        case '>':
            o += "&gt;";
            break;
        case '\"':
            o += "&quot;";
            break;
        case '\'':
            o += "&#39;";
            break;
        default:
            o += c;
        }
    }
    return o;
}
static String url(const String &s) {
    String o;
    for (unsigned i = 0; i < s.length(); i++) {
        unsigned char c = s[i];
        if (isalnum(c) || c == '/' || c == '-' || c == '_' || c == '.')
            o += char(c);
        else {
            char b[4];
            snprintf(b, 4, "%%%02X", c);
            o += b;
        }
    }
    return o;
}
bool WiFiTransferApp::auth() {
    if (server.authenticate("tiko", password.c_str()))
        return true;
    server.requestAuthentication();
    return false;
}
bool WiFiTransferApp::authorize() {
    if (!auth())
        return false;
    if (server.header("X-Tiko-CSRF") != token) {
        server.send(403, "text/plain", "CSRF");
        return false;
    }
    return true;
}
String WiFiTransferApp::path() {
    String p = server.arg("path");
    if (p.isEmpty())
        p = "/TikoOS";
    return tiko::safePath(p.c_str()) ? p : String();
}
void WiFiTransferApp::begin(bool useNetwork) {
    WiFi.persistent(false);
    stop();
    station = useNetwork;
    password = settings.get("ap-pass");
    if (password.length() < 8)
        password = "Tigran123456!";
    WiFi.mode(station ? WIFI_STA : WIFI_AP);
    token = randomHex();
    if (station) {
        String ssid = settings.get("sta-ssid");
        if (ssid.isEmpty()) {
            status = "Настройте Wi-Fi";
            return;
        }
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        WiFi.begin(ssid.c_str(), settings.get("sta-pass").c_str());
        active = true;
        connectStarted = millis();
        status = "Подключение: " + ssid;
    } else {
        WiFi.mode(WIFI_AP);
        active = WiFi.softAP("TikoOS", password.c_str(), 1, 0, 1);
        status = active ? "http://" + WiFi.softAPIP().toString() : "AP error";
        if (active)
            startServer();
    }
}
void WiFiTransferApp::startServer() {
    if (!configured) {
        const char *headers[] = {"X-Tiko-CSRF"};
        server.collectHeaders(headers, 1);
        server.on("/", HTTP_GET, [this] {
            if (auth())
                page();
        });
        server.on("/view", HTTP_GET, [this] {
            if (!auth())
                return;
            String p = path();
            File f = storage.ready && p.length() ? SD.open(p) : File();
            if (!f || f.isDirectory() || !p.endsWith(".txt")) {
                server.send(404);
                return;
            }
            server.sendHeader("X-Content-Type-Options", "nosniff");
            server.streamFile(f, "text/plain; charset=utf-8");
        });
        server.on("/mkdir", HTTP_POST, [this] {
            if (!authorize())
                return;
            String p = path();
            bool ok = storage.ready && p.length() && SD.mkdir(p);
            server.send(ok ? 200 : 400, "text/plain", ok ? "OK" : "Cannot create");
        });
        server.on("/delete", HTTP_POST, [this] {
            if (!authorize())
                return;
            String p = path();
            bool ok =
                server.arg("confirm") == "yes" && p.length() && p != "/TikoOS" && storage.remove(p);
            server.send(ok ? 200 : 400, "text/plain", ok ? "OK" : "Cannot delete");
        });
        server.on(
            "/upload", HTTP_POST,
            [this] {
                if (!authorize())
                    return;
                server.send(uploadOK ? 200 : 400, "text/plain",
                            uploadOK ? "OK" : "Upload rejected (TXT, 2 MiB max, no overwrite)");
            },
            [this] { onUpload(); });
        server.onNotFound([this] { server.send(404); });
        configured = true;
    }
    server.begin();
    serving = true;
}
void WiFiTransferApp::stop() {
    if (upload)
        upload.close();
    if (temp.length() && storage.ready)
        SD.remove(temp);
    temp = "";
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    active = false;
    serving = false;
    WiFi.scanDelete();
}
void WiFiTransferApp::onUpload() {
    auto &u = server.upload();
    if (u.status == UPLOAD_FILE_START) {
        uploadOK = false;
        bytes = 0;
        target = "";
        if (!server.authenticate("tiko", password.c_str()) ||
            server.header("X-Tiko-CSRF") != token || !storage.ready)
            return;
        String dir = path(), name = u.filename;
        target = dir + "/" + name;
        if (dir.isEmpty() || name.indexOf('/') >= 0 || name.indexOf('\\') >= 0 ||
            !tiko::safePath(target.c_str()) || !name.endsWith(".txt") || SD.exists(target))
            return;
        temp = dir + "/.upload-" + randomHex();
        upload = SD.open(temp, FILE_WRITE);
        uploadOK = bool(upload);
    } else if (u.status == UPLOAD_FILE_WRITE) {
        bytes += u.currentSize;
        if (bytes > 2 * 1024 * 1024)
            uploadOK = false;
        if (uploadOK && upload.write(u.buf, u.currentSize) != u.currentSize)
            uploadOK = false;
    } else if (u.status == UPLOAD_FILE_END || u.status == UPLOAD_FILE_ABORTED) {
        if (upload)
            upload.close();
        if (u.status == UPLOAD_FILE_ABORTED)
            uploadOK = false;
        if (uploadOK)
            uploadOK = SD.rename(temp, target);
        if (!uploadOK && temp.length())
            SD.remove(temp);
        temp = "";
    }
}
void WiFiTransferApp::page() {
    String p = path();
    if (p.isEmpty()) {
        server.send(400);
        return;
    }
    String s =
        R"HTML(<!doctype html><html lang="ru"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>TikoOS</title><style>body{background:#10151d;color:white;font:16px system-ui;max-width:700px;margin:auto;padding:20px}a{color:#48dcff}article,button,input{background:#222c38;color:white;border:1px solid #415368;border-radius:10px;padding:12px;margin:5px}article{display:flex;align-items:center;justify-content:space-between}button{cursor:pointer}input{max-width:85%}</style><h1>TikoOS · Files</h1>)HTML";
    s += "<p>" + html(storage.freeSpace()) + "</p><h3>" + html(p) +
         "</h3><a href='/?path=/TikoOS'>TikoOS</a>";
    if (p != "/TikoOS")
        s += " · <a href='/?path=" + url(p.substring(0, p.lastIndexOf('/'))) + "'>Назад</a>";
    for (auto &e : storage.list(p)) {
        s += "<article><a href='" + String(e.directory ? "/?path=" : "/view?path=") + url(e.path) +
             "'>" + html(e.name) + "</a> " + String(e.size) + " B";
        if (!e.directory)
            s += "<button data-path='" + html(e.path) +
                 "' onclick='del(this.dataset.path)'>Удалить</button>";
        s += "</article>";
    }
    s += "<p><input id='folder' placeholder='Новая папка'><button "
         "onclick='mkdir()'>Создать</button></p><input id='file' type='file' accept='.txt'><button "
         "onclick='upload()'>Загрузить TXT</button><p id='result'></p>";
    // Paths are passed through URI encoding rather than inserted as executable JavaScript.
    s += "<script>const token='" + token + "',base=decodeURIComponent('" + url(p) +
         "');async function send(route,body){let r=await "
         "fetch(route,{method:'POST',headers:{'X-Tiko-CSRF':token},body});let t=await "
         "r.text();document.getElementById('result').textContent=t;if(r.ok)location.reload()}"
         "function del(p){if(confirm('Удалить '+p+'?'))send('/delete',new "
         "URLSearchParams({path:p,confirm:'yes'}))}function mkdir(){send('/mkdir',new "
         "URLSearchParams({path:base+'/'+document.getElementById('folder').value}))}function "
         "upload(){let f=document.getElementById('file').files[0];if(!f)return;let b=new "
         "FormData();b.append('file',f);send('/upload?path='+encodeURIComponent(base),b)}</"
         "script></html>";
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("X-Frame-Options", "DENY");
    server.send(200, "text/html; charset=utf-8", s);
}

void WiFiTransferApp::loop() {
    if (!active)
        return;
    if (station) {
        if (WiFi.status() == WL_CONNECTED) {
            status = "http://" + WiFi.localIP().toString();
            if (!serving)
                startServer();
        } else {
            if (serving) {
                server.stop();
                serving = false;
                if (upload)
                    upload.close();
                if (temp.length())
                    SD.remove(temp);
                temp = "";
                uploadOK = false;
                connectStarted = millis();
            }
            if (millis() - connectStarted > 20000)
                status = "Нет связи: Back / повтор";
        }
    }
    if (serving)
        server.handleClient();
}
