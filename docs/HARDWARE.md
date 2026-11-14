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
