# Источники и аппаратные предположения

Проверено 2026-11-14 по исходникам:

- LilyGO T-Embed-CC1101 commit `c36826e0f91311cb591c4545fccf91e254fc55dc`:
  https://github.com/Xinyuan-LilyGO/T-Embed-CC1101/tree/c36826e0f91311cb591c4545fccf91e254fc55dc
  `examples/utilities.h`, `boards/T_Embed_PN532.json`, `platformio.ini`,
  `examples/test_sd_card/test_sd_card.ino`, `examples/encode_test/encode_test.ino`,
  `lib/TFT_eSPI/User_Setups/Setup214_LilyGo_T_Embed_PN532.h`, factory startup.
- Launcher commit `6b432e4e185ceaebad14fb902d750d699b380bbd`:
  https://github.com/bmorcelli/Launcher/tree/6b432e4e185ceaebad14fb902d750d699b380bbd
  `boards/lilygo-t-embed-cc1101/platformio.ini`, `interface.cpp`.
- https://github.com/bmorcelli/Launcher/wiki/Obtaining-binaries-to-launch
- https://github.com/bmorcelli/Launcher/wiki/Explaining-the-project

| Функция | GPIO / параметры |
|---|---|
| Power enable | 15 HIGH |
| ST7789 | native 170×320, rotation 3 → 320×170, inversion ON |
| SPI | SCK 11 / MOSI 9 / MISO 10, HSPI |
| TFT | CS 41 / DC 16 / RST -1 / BL 21, 40 MHz |
| SD | CS 13, тот же `TFT_eSPI::getSPIinstance()`, 4 MHz |
| CC1101 | CS 12 HIGH (неактивен) |
| nRF24 | CS 44 HIGH / CE 43 LOW (неактивен) |
| Encoder | A 4 / B 5 / click 0 (BOOT) |
| Back | 6 |
| I2C | SDA 8 / SCL 18 |
| PN532 / fuel gauge / charger | read-only ACK на 0x24 / 0x55 / 0x6B |

Определение платы скопировано из официального T_Embed_PN532.json. Пины обычного
T-Embed (например LCD CS10, rotary 2/1) не использованы. Все SPI CS отключаются
до display/SD init, CE радио удерживается LOW. Зарядные регистры не меняются.
Официальный header содержит также legacy I2C aliases с переставленными 8/18;
здесь выбраны актуальные BOARD_I2C_SDA/SCL и параметры Launcher.

README LilyGO называет hardware v1.0-241103, но не предоставляет отдельное имя
board `CC1101 Plus`. nRF24 описан как расширение. Нельзя утверждать, что любая
плата с маркетинговым названием Plus имеет ту же разводку. Перед установкой
сверьте свою ревизию/схему с таблицей. Отклонения требуют другого build target.

RAM из linker report не включает динамический framebuffer (108800 B), сетевые
буферы, heap строк/каталогов. Sprite allocation проверяется при старте, иначе
загрузка прерывается с сообщением Serial. Размер PSRAM и качество питания нужно
проверить на плате. Наличие ACK не доказывает работоспособность радио/NFC.

## USB в v0.2

USB MSC реализован через Arduino ESP32 2.0.17 `cores/esp32/USBMSC.h/.cpp` и
`SDFS::readRAW/writeRAW/numSectors/sectorSize`, по штатным TinyUSB API. В board flags
удалён дублирующий USB_MODE=1; единственный флаг проекта — USB_MODE=0 (OTG).
USBMSC конструируется глобально, интерфейс регистрируется до `USB.begin` core.
Medium отсутствует до явного выбора USB Transfer.

Перед экспортом все локальные File закрыты, Wi-Fi остановлен, карточка помечена
недоступной StorageManager. После последнего кадра USB-экрана main loop не выполняет
ни TFT SPI, ни VFS/SD операции; raw callbacks защищены mutex и проверяют границы,
включая частичные секторы. FatFS остаётся зарегистрированным, но не используется
во время экспорта. После host Eject происходит перезапуск, чтобы отбросить старый
локальный FAT-кэш; одновременно монтировать карту на ПК и пользоваться ею в UI нельзя.
На карте требуется сектор 512 байт. Форматирования/erase/repartition в коде нет.
Аппаратная передача USB/события eject и конкретная Plus-ревизия не проверены.

## v0.3 — системная база Sor3nt

Активный board header теперь находится в
`firmware/components/furi_hal/boards/board_lilygo_t_embed_cc1101.h`.
Пины SPI/SD/энкодера совпадают с проверенной выше LilyGO-схемой. В upstream LCD
reset был назначен GPIO40, совпадающий с I2S WCLK; для TikoOS он изменён на -1,
как в официальном примере LilyGO. Panel init использует SWRESET; invalid reset
GPIO не участвует в sleep/reset tables. SD recovery включён независимо от BW16.
Используется stock ESP-IDF esp_lcd и общий SPI mutex базы.

Загрузка использует USB Serial/JTAG; USB OTG composite устанавливается по запросу
штатного USB-Storage. В отличие от v0.2, в новом образе нет Arduino USBMSC.
Служба Storage размонтирует FAT перед raw-доступом; последующее монтирование
выполняет база. Одновременное использование VFS и USB запрещено.

Основная прошивка теперь использует ESP-IDF v5.4.1, OPI PSRAM 8 MB, flash 16 MB, dual-core.
Аппаратные результаты v0.2 нельзя автоматически переносить на v0.3. Наши тесты
целевого input driver моделируют GPIO и тики, не электрические характеристики
энкодера и не реальные задержки/питание.
