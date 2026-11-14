#pragma once
#include "../storage/StorageManager.h"
#include <WebServer.h>
#include <WiFi.h>
class WiFiTransferApp {
    WebServer server{80};
    StorageManager &storage;
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
    explicit WiFiTransferApp(StorageManager &s) : storage(s) {}
    void begin();
    void stop();
    void loop() {
        if (active)
            server.handleClient();
    }
};
