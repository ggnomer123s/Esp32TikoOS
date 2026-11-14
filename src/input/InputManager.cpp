#include "InputManager.h"
InputManager *InputManager::instance = nullptr;

void InputManager::edge() {
    auto *p = instance;
    portENTER_CRITICAL_ISR(&p->mux);
    p->pending += p->decoder.feed((digitalRead(4) << 1) | digitalRead(5));
    portEXIT_CRITICAL_ISR(&p->mux);
}
