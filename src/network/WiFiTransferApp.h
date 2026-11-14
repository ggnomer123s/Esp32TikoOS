#pragma once
#include "../storage/StorageManager.h"
#include <WebServer.h>
#include <WiFi.h>
class WiFiTransferApp {
    WebServer server{80};
    StorageManager &storage;
    SettingsManager &settings;
    bool configured = false, serving = false, station = false;
    uint32_t connectStarted = 0;
    void startServer();
    File upload;
    String temp, target;
    bool uploadOK = false;
    size_t bytes = 0;
    String token;
    bool auth();
    bool authorize();
    String path();
    void page();
    void onUpload();

  public:
    bool active = false;
    String password, status;
    explicit WiFiTransferApp(StorageManager &s, SettingsManager &v) : storage(s), settings(v) {}
    void begin(bool useNetwork = false);
    void scan() {
        WiFi.persistent(false);
        stop();
        WiFi.mode(WIFI_STA);
        WiFi.scanNetworks(true);
    }
    bool connected() const {
        return station && WiFi.status() == WL_CONNECTED;
    }
    void stop();
    void loop();
};
