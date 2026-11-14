# TikoOS v0.3 / Launcher app — сборка 2026-11-14

**SUCCESS**, ESP-IDF **v5.4.1** (`4c2820d377d1375e787bcef612f0c32c1427d183`),
Xtensa GCC 14.2.0 / esp-20241119, ESP32-S3, 4 jobs. Системная база Sor3nt:
`e6ec11972a8e025d7ab686a6ef3016cf52360834`. C++ приложения — C++17.

- Профиль **CONFIG_TIKO_LAUNCHER_APP=y**, build target **app**.
- App-image: **2693024 bytes**, `dist/firmware.bin`.
- SHA-256: `050de51aa6f16d70fc23398300ca4a65bcc69cb37c19942bf83b450a244e7b75`.
- Сборочный лимит app: 4194304 bytes (4 MiB), проверка размера прошла.
- Link-map DIRAM: 196715 / 341760 bytes, свободно 145045.
- Измерения динамического heap/PSRAM на устройстве не выполнялись.

## Контракт образа

Это результат `TikoOS.bin` app target, без merge-bin, padding до flash-offset,
bootloader.bin или partitions.bin. Проект и версия в ESP app descriptor —
TikoOS / 0.3.0. esptool проверяет ESP32-S3 image checksum/hash; отчёт —
`dist/image-info.txt`. Наличие magic 0xE9 само по себе не является единственным
доказательством типа образа: проверяются app descriptor и источник сборки.

Новая firmware не требует таблицы dual-OTA базы, второго OTA-слота или u2fkeys.
Для сборочной проверки есть `partitions_launcher_build.csv`, которая не
включается в app-image и не прошивается. Используются существующие ресурсы
Launcher: его bootloader, таблица и обычная NVS. In-app OTA отключено даже при
наличии чужих app-слотов. Проверка nm подтвердила: OTA-объект не импортирует
esp_ota_begin/write/end/set_boot_partition. `dist/launcher-check.txt` фиксирует это.

Фактический Launcher-раздел должен вмещать весь файл. Слот 1.5 MB не подходит.
Физическая таблица и запуск приложения на вашей плате здесь не проверялись.
Изменение bootloader/partition table, USB upload и erase_flash не выполнялись.

## Выполненные команды

```sh
bash scripts/install_native.sh
python3 scripts/build_native.py
bash scripts/test_native.sh
```

Скрипт установки повторно выполнен на текущей машине; pinned Python dependencies,
SDK tools и build/app workflow проверены. SDK checkout остался без tracked changes.
В облаке недоступны Espressif constraints/Component Registry: используется snapshot
Python packages `scripts/requirements-idf.txt` и vendored components с Git revisions.
Проверка TLS/архивов инструментов не отключалась. Сборка использует настоящий
ESP-IDF toolchain/CMake/Ninja, не эмуляцию компилятора.

## Результаты проверок

- **331 + 557 = 888 host assertions**, ASan/UBSan и warnings-as-errors.
- **7 групп сценариев реального target_input.c** с моделируемыми GPIO/тиками:
  click, post-scroll click, shaft jostle, reverse, sensitivity, invalid AB, Back.
- Проверены UTF-8, EN/RU keyboard, glyphs трёх шрифтов, sandbox, strict URL decode,
  upload limits и сериализация. WebUI JavaScript прошёл `node --check`.
- ESP image checksum/hash, содержимое SD/source ZIP и SHA-256 проверяются упаковкой.

В логах ESP-IDF 5.4.1 присутствуют deprecation notes нового Kconfig/esptool Python
инструментария (legacy bool/hex defaults и имена CLI-опций). Они не скрывались;
финальная компиляция/линковка и size checks завершились успешно.
Отчёты: `dist/build.log`, `dist/native-tests.log`, `dist/size.json`.

TFT, SD, Wi-Fi/Bluetooth, USB, питание, реальный input latency и Launcher остаются
**Unverified**. Хост-тесты и сборка не доказывают отсутствие всех runtime-багов.
