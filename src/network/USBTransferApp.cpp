#include "USBTransferApp.h"
USBTransferApp *USBTransferApp::instance = nullptr;
void USBTransferApp::configure() {
    instance = this;
    io = xSemaphoreCreateMutex();
    if (!io)
        abort();
    msc.vendorID("TikoOS");
    msc.productID("microSD");
    msc.productRevision("0101");
    msc.onRead(read);
    msc.onWrite(write);
    msc.onStartStop(startStop);
    msc.mediaPresent(false);
}
bool USBTransferApp::start(StorageManager &storage) {
    if (!storage.ready || SD.sectorSize() != 512)
        return false;
    sectors = SD.numSectors();
    if (!sectors || !msc.begin(sectors, 512))
        return false;
    // Every application File is closed by caller. No further local FAT/SD or display calls.
    storage.ready = false;
    ejected = false;
    active = true;
    msc.mediaPresent(true);
    return true;
}
int32_t USBTransferApp::read(uint32_t lba, uint32_t offset, void *data, uint32_t length) {
    return instance->transfer(lba, offset, static_cast<uint8_t *>(data), length, false);
}
int32_t USBTransferApp::write(uint32_t lba, uint32_t offset, uint8_t *data, uint32_t length) {
    return instance->transfer(lba, offset, data, length, true);
}
int32_t USBTransferApp::transfer(uint32_t lba, uint32_t offset, uint8_t *data, uint32_t length,
                                 bool writing) {
    if (!active.load() || !io || !tiko::sectorRange(lba, offset, length, sectors))
        return -1;
    xSemaphoreTake(io, portMAX_DELAY);
    if (!active) {
        xSemaphoreGive(io);
        return -1;
    }
    uint8_t sector[512];
    uint32_t done = 0;
    bool ok = true;
    while (done < length && ok) {
        uint32_t n = std::min<uint32_t>(512 - offset, length - done);
        if (writing) {
            if (offset || n != 512) {
                ok = SD.readRAW(sector, lba);
                if (ok) {
                    memcpy(sector + offset, data + done, n);
                    ok = SD.writeRAW(sector, lba);
                }
            } else
                ok = SD.writeRAW(data + done, lba);
        } else {
            ok = SD.readRAW(sector, lba);
            if (ok)
                memcpy(data + done, sector + offset, n);
        }
        done += n;
        lba++;
        offset = 0;
    }
    xSemaphoreGive(io);
    return ok ? int32_t(done) : -1;
}
bool USBTransferApp::startStop(uint8_t, bool start, bool eject) {
    auto *p = instance;
    if (eject && !start && p->active) {
        xSemaphoreTake(p->io, portMAX_DELAY);
        p->msc.mediaPresent(false);
        p->ejected = true;
        p->active = false;
        xSemaphoreGive(p->io);
    }
    return true;
}
