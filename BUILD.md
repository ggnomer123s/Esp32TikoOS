# Результат сборки TikoOS v0.2 — 2026-11-14

**SUCCESS**, PlatformIO 6.1.18 / espressif32 6.13.0 / Arduino ESP32 2.0.17.
Target `tiko`, board `tiko-cc1101`, C++17, release, 4 jobs, USB OTG (mode 0).

- Flash (linker): **968793 / 4194304 bytes**.
- Static RAM: **60348 / 327680 bytes**. Динамический heap/framebuffer не включён.
- App-image: **969152 bytes**, `dist/firmware.bin`.
- SHA-256: `7f44dd9b6fafe09e22ba4ccd93800fa5f62d262db822f9d4a16def4be91396b6`.
- Host tests: **331 assertions passed**, AddressSanitizer + UndefinedBehaviorSanitizer,
  `-Wall -Wextra -Werror`, без ошибок.

```sh
/workspace/pio-venv/bin/python scripts/bootstrap_cloud.py
/workspace/pio-venv/bin/python scripts/cloud_pio.py run -c /workspace/tiko-cloud.ini -j 4
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined tests/core_test.cpp -o /tmp/tiko-tests
/tmp/tiko-tests
```

Используется локальная установка официальных архивов Espressif с проверкой SHA-256:
реестр PlatformIO в этой облачной среде блокировался proxy (403). SCons/esptool
закреплены через PyPI. Сборка выполняется настоящим PlatformIO builder;
TLS/checksum проверки и проверки компилятора не отключены. Инструкции — README.

Что проверяют host tests: UTF-8/кириллица, sandbox/имена файлов, quadrature,
короткое/долгое нажатие и дребезг, переполнение millis, пять раскладок клавиатуры,
Unicode backspace, лимиты ввода, сериализация настроек и USB sector bounds.
Они не исполняют WebServer/TinyUSB на ESP32 и не измеряют задержки TFT/энкодера.

Единственное предупреждение финальной сборки — TFT_eSPI TOUCH_CS undefined:
сенсорный ввод на этой плате не используется, фиктивный GPIO не задан.
После сборки esptool проверяет checksum/hash ESP32-S3 app-image; отчёт —
`dist/image-info.txt`. Остальные отчёты — `dist/build.log`, `dist/native-tests.log`.

В v0.2 удалён PWM fade при переходах и отбрасывание первого события после сна.
Кнопки опрашиваются отдельной задачей, очередь сохраняет порядок движения/click,
отрисовка объединяет события и передаёт только изменившиеся строки framebuffer.
Поиск Wi-Fi/STA connection выполняются без ожидания в UI-loop. Перед USB raw export
локальный UI/SD и сеть останавливаются; Eject приводит к restart без старого FAT-кэша.
Функциональность USB/STA/AP, реальная скорость UI и Eject на конкретной ОС требуют
проверки на плате. Период кадра 16 ms в коде — ограничитель, не измеренный FPS.

USB upload, erase_flash и изменение разделов устройства не выполнялись.
Конкретная CC1101 Plus ревизия и Launcher на реальной плате — **Unverified**.
