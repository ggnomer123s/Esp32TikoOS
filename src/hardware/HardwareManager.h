#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
class HardwareManager {
  public:
    bool pn532 = false, gauge = false, charger = false;
    void begin() {
        pinMode(15, OUTPUT);
        digitalWrite(15, HIGH);
        for (int p : {41, 13, 12, 44}) {
            pinMode(p, OUTPUT);
            digitalWrite(p, HIGH);
        }
        pinMode(43, OUTPUT);
        digitalWrite(43, LOW);
        Wire.begin(8, 18);
        Wire.setTimeOut(20);
        pn532 = probe(0x24);
        gauge = probe(0x55);
        charger = probe(0x6b);
    }
    bool probe(uint8_t a) {
        Wire.beginTransmission(a);
        return Wire.endTransmission() == 0;
    }
    // Read-only I2C address probes. No RF transmission or speculative register writes.
};
