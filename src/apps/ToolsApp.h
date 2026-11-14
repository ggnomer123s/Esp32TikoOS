#pragma once
#include <Arduino.h>
#include <cmath>
class ToolsApp {
  public:
    double value = 0;
    String input = "";
    char op = '+';
    String error;
    bool running = false;
    uint32_t started = 0, elapsed = 0;
    String keys = "0123456789.+-*/=C<";
    void key(char c) {
        error = "";
        if (c == 'C') {
            value = 0;
            input = "";
            op = '+';
            return;
        }
        if (c == '<') {
            if (input.length())
                input.remove(input.length() - 1);
            return;
        }
        if ((c >= '0' && c <= '9') || c == '.') {
            if (input.length() < 16 && (c != '.' || input.indexOf('.') < 0))
                input += c;
            return;
        }
        if (input.length()) {
            double b = input.toDouble();
            if (op == '/' && b == 0) {
                error = "Деление на ноль";
                return;
            }
            switch (op) {
            case '+':
                value += b;
                break;
            case '-':
                value -= b;
                break;
            case '*':
                value *= b;
                break;
            case '/':
                value /= b;
                break;
            }
            if (!std::isfinite(value)) {
                error = "Переполнение";
                value = 0;
            }
            input = "";
        }
        op = c == '=' ? '+' : c;
    }
    uint32_t time() {
        return elapsed + (running ? millis() - started : 0);
    }
    void toggle() {
        if (running)
            elapsed += millis() - started;
        else
            started = millis();
        running = !running;
    }
    void reset() {
        elapsed = 0;
        started = millis();
    }
};
