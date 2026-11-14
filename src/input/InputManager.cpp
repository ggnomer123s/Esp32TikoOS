#include "InputManager.h"
InputManager *InputManager::instance = nullptr;
void InputManager::edge() {
    auto *p = instance;
    portENTER_CRITICAL_ISR(&p->mux);
    p->pending += p->decoder.feed((digitalRead(4) << 1) | digitalRead(5));
    portEXIT_CRITICAL_ISR(&p->mux);
}
void InputManager::begin() {
    for (int p : {0, 6, 4, 5})
        pinMode(p, INPUT_PULLUP);
    decoder.state = (digitalRead(4) << 1) | digitalRead(5);
    instance = this;
    events = xQueueCreate(64, sizeof(Event));
    if (!events)
        abort();
    attachInterrupt(4, edge, CHANGE);
    attachInterrupt(5, edge, CHANGE);
    if (xTaskCreatePinnedToCore(task, "tiko-input", 3072, this, 3, nullptr, 1) != pdPASS)
        abort();
}
void InputManager::task(void *context) {
    auto *p = static_cast<InputManager *>(context);
    TickType_t next = xTaskGetTickCount();
    for (;;) {
        portENTER_CRITICAL(&p->mux);
        int turn = p->pending;
        p->pending = 0;
        portEXIT_CRITICAL(&p->mux);
        Event e{turn, p->select.sample(digitalRead(0), millis()),
                p->back.sample(digitalRead(6), millis())};
        if (e.turn || e.click || e.back) {
            if (xQueueSend(p->events, &e, 0) != pdTRUE) {
                p->overflows++; // Preserve button events rather than silently dropping a click.
                if (e.click || e.back)
                    xQueueSend(p->events, &e, portMAX_DELAY);
            }
        }
        vTaskDelayUntil(&next, pdMS_TO_TICKS(1));
    }
}
