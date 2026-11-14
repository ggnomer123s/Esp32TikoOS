#pragma once
#include "../storage/StorageManager.h"
#include <USB.h>
#include <USBMSC.h>
#include <atomic>
#include <freertos/semphr.h>
// Exclusive whole-card export. No UI SPI/VFS access until host eject + restart.
class USBTransferApp {
    USBMSC msc;
    static USBTransferApp *instance;
    SemaphoreHandle_t io = nullptr;
    uint32_t sectors = 0;
    static int32_t read(uint32_t lba, uint32_t offset, void *data, uint32_t length);
    static int32_t write(uint32_t lba, uint32_t offset, uint8_t *data, uint32_t length);
    static bool startStop(uint8_t power, bool start, bool eject);
    int32_t transfer(uint32_t lba, uint32_t offset, uint8_t *data, uint32_t length, bool writing);

  public:
    std::atomic<bool> active{false}, ejected{false};
    void configure();
    bool start(StorageManager &storage);
};
