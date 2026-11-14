# Результат сборки — 2026-11-14

**SUCCESS**, PlatformIO 6.1.18 / espressif32 6.13.0 / Arduino ESP32 2.0.17.
Target `tiko`, board `tiko-cc1101`, C++17, release, 4 jobs.

- Flash (linker): 924497 / 4194304 bytes (22.0%).
- Static RAM: 47420 / 327680 bytes (14.5%). Динамический heap не включён.
- App-image: **924864 bytes**, `dist/firmware.bin`.
- SHA-256: `c105c33f1938c0b4b22b8829be0c665ce549673c8916007081ff705a92275918`.
- esptool 4.11.0: ESP32-S3 image, checksum valid, validation hash valid.
- Host tests: **98 assertions passed**, AddressSanitizer + UndefinedBehaviorSanitizer,
  `-Wall -Wextra -Werror`, без сообщений о проблемах.

Команда успешной финальной сборки (из каталога проекта):

```sh
/workspace/pio-venv/bin/python scripts/cloud_pio.py run -c /workspace/tiko-cloud.ini -j 4
```

Подготовка `/workspace/tiko-cloud.ini` — `scripts/bootstrap_cloud.py`.
Обычный `pio run` был запущен, но получил HTTPClientError: proxy запрещал реестр
PlatformIO. Сборка затем выполнена с локальными package overrides официальных
Espressif release archives и закреплённым TFT_eSPI. Все три архива проверены по
официальным SHA-256, TLS/checksum verification не отключались. Скрипт bootstrap
повторно выполнен и проверен сборкой. Pinned SCons/esptool из PyPI запускаются
через локальные wrappers, без изменения кода PlatformIO/framework.

Первая линковка обнаружила dangerous relocation в inline IRAM encoder handler.
Обработчик вынесен в .cpp и использует стандартный Arduino interrupt registration
без IRAM flag. При flash/NVS операциях interrupt может быть временно задержан;
качество ввода в этот момент проверяется на плате.

Единственное предупреждение финальной сборки — TFT_eSPI TOUCH_CS undefined:
эта плата управляется энкодером, touch не используется. Предупреждение оставлено,
фиктивный TOUCH_CS не задан. Код собственного firmware собран без ошибок.

Отчёты: `dist/build.log`, `dist/image-info.txt`, `dist/native-tests.log`.
Динамический framebuffer требует 108800 B, каталоги/строки и Wi-Fi используют heap.
Измерение свободного heap/PSRAM на реальной плате ещё не проводилось.
Физические GPIO, SD, TFT, Wi-Fi, NVS на ESP32 и Launcher — **Unverified**.
Не выполнены USB upload, erase_flash, прошивка или repartitioning устройства.
