#pragma once
#include "../utils/Core.h"
#include <Arduino.h>
class InputManager {
    struct Button {
        int pin;
        bool raw = true, stable = true;
        uint32_t changed = 0, down = 0;
        bool held = false;
        int tick() {
            bool v = digitalRead(pin);
            uint32_t now = millis();
            if (v != raw) {
                raw = v;
                changed = now;
            }
            if (now - changed >= 25 && stable != raw) {
                stable = raw;
                if (!stable) {
                    down = now;
                    held = false;
                } else if (!held)
                    return 1;
            }
            if (!stable && !held && now - down > 650) {
                held = true;
                return 2;
            }
            return 0;
        }
    };
    Button select{0}, back{6};
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    volatile int pending = 0;
    static InputManager *instance;
    static void edge();

  public:
    tiko::Quadrature decoder;
    struct Event {
        int turn, click, back;
    };
    void begin() {
        for (int p : {0, 6, 4, 5})
            pinMode(p, INPUT_PULLUP);
        decoder.state = (digitalRead(4) << 1) | digitalRead(5);
        instance = this;
        attachInterrupt(4, edge, CHANGE);
        attachInterrupt(5, edge, CHANGE);
    }
    Event poll() {
        portENTER_CRITICAL(&mux);
        int v = pending;
        pending = 0;
        portEXIT_CRITICAL(&mux);
        return {v, select.tick(), back.tick()};
    }
};
