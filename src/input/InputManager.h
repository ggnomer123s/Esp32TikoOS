#pragma once
#include "../utils/Core.h"
#include <Arduino.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
class InputManager {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    volatile int pending = 0;
    static InputManager *instance;
    static void edge();
    static void task(void *context);
    QueueHandle_t events = nullptr;
    tiko::DebouncedButton select, back;

  public:
    tiko::Quadrature decoder;
    struct Event {
        int turn = 0, click = 0, back = 0;
    };
    std::atomic<uint32_t> overflows{0};
    tiko::Quadrature snapshot() {
        portENTER_CRITICAL(&mux);
        auto result = decoder;
        portEXIT_CRITICAL(&mux);
        return result;
    }
    void begin();
    Event poll() {
        Event e;
        if (events)
            xQueueReceive(events, &e, 0);
        return e;
    }
};
