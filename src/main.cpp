#include "ui/ScreenManager.h"
#include <Arduino.h>
HardwareManager hardware;
DisplayManager display;
InputManager input;
StorageManager storage;
SettingsManager settings;
ScreenManager screens(display, storage, settings, input, hardware);
void setup() {
    Serial.begin(115200);
    hardware.begin();
    display.begin();
    display.boot(1);
    settings.begin();
    display.brightness(settings.value.brightness);
    display.boot(2);
    input.begin();
    display.boot(3);
    storage.begin();
    display.boot(4);
    screens.begin();
}
void loop() {
    screens.loop();
    delay(1);
}
