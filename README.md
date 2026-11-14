# TikoOS v0.3 — ESP-IDF / Furi

TikoOS перенесена на системную основу
[Sor3nt/Flipper-Zero-ESP32-Port](https://github.com/Sor3nt/Flipper-Zero-ESP32-Port),
commit `e6ec11972a8e025d7ab686a6ef3016cf52360834`.
Целевая плата — **LilyGO T-Embed CC1101 / совместимая Plus-ревизия**, ESP32-S3,
16 MB Flash и 8 MB PSRAM. **Для Launcher поставляется только app-image .bin,
без bootloader.bin и partitions.bin.** Сборка выполнена, но работа на физической плате
и установка в конкретный Launcher остаются **Unverified**.

## Скачать

- **[Скачать firmware.bin](https://raw.githubusercontent.com/ggnomer123s/Esp32TikoOS/main/dist/firmware.bin)**
- **[Скачать TikoOS-microSD.zip](https://raw.githubusercontent.com/ggnomer123s/Esp32TikoOS/main/dist/TikoOS-microSD.zip)**
- [Исходники v0.3](https://raw.githubusercontent.com/ggnomer123s/Esp32TikoOS/main/dist/TikoOS-v0.3-source.zip)
- [Контрольная сумма](dist/SHA256SUMS) · [История изменений](aireadme.md)

Пакет карты называется **TikoOS-microSD.zip**. Распакуйте его в корень FAT32-карты,
сохраняя существующие файлы. В архиве — `TikoOS/`, анимационные ресурсы базы
`dolphin/`, отдельный app-image `firmware.bin` и его SHA-256.
Не форматируйте карту ради установки и не перезаписывайте собственные конспекты.

## Что изменилось

Основной проект — **`firmware/`**, ESP-IDF **5.4.1**, Furi/FreeRTOS, штатные
`Gui`, `Input`, `ViewDispatcher`, `Submenu`, `Storage`, `Loader`, `Desktop`,
Bluetooth и Wi-Fi services. Arduino renderer и WebServer больше не являются
основой текущего `firmware.bin`. Старый код сохранён для истории; корневой
`platformio.ini` собирает только прежний Arduino-прототип.

Интерфейс новой базы рисуется как **128×64 mono framebuffer**, масштабируемый
и центрируемый на TFT 320×170. Это интерфейс выбранного порта, а не прежние
цветные карточки. Русские меню, Notes и клавиатура имеют кириллические шрифты.
При запуске открывается приложение TikoOS; Back из него возвращает штатный Desktop.

| Возможность | Реализация v0.3 |
|---|---|
| Меню, движение, кнопки | GUI/Input/ViewDispatcher базы, quadrature T-Embed |
| Notes / Formulas | Перенесённые приложения TikoOS поверх Furi Storage/GUI |
| Favorites | Избранное документов TikoOS, 24 записи, сохранение на SD |
| Files | Штатный Archive базы, без второго файлового менеджера |
| Bluetooth | Штатные BT service/settings, подключение и системное управление |
| Wi-Fi | Штатный service/HAL: worker, connect/reconnect, сохранённая сеть |
| Передача AP / My network | Сервер базы на отдельной HTTP-задаче, политика TikoOS |
| USB | Штатный USB-Storage в Desktop / Lock menu, мягкое размонтирование SD |
| Clock / stopwatch | Штатный Clock, монотонное время и состояние между открытиями |
| Calculator / Monitor | Приложения TikoOS |
| Настройки | Системные Interface/Notification/About + шрифт и энкодер TikoOS |

Полный набор сторонних приложений исходного порта не является профилем TikoOS.
Список включённых приложений — `firmware/fam_config.py`, источники и изменения
базы — [UPSTREAM.md](firmware/UPSTREAM.md) и [MIGRATION-v0.3.md](docs/MIGRATION-v0.3.md).

## Notes, Formulas и клавиатура

Структура карты сохранена:

```text
TikoOS/
  notes/Physics/Heat.txt
  notes/Algebra/Equations.txt
  formulas/Physics/Energy.txt
  formulas/Algebra/Discriminant.txt
  favorites/list.txt
  config/positions/
  assets/
```

Категории — реальные директории. Читатель использует блоки **768 байт** с
корректными границами UTF-8, не загружает весь длинный документ в RAM. Вращение
листит строки, OK открывает меню избранного, размера шрифта и подтверждаемого
удаления. Три размера шрифта, русская азбука с Ё/ё, латиница и математические
символы включены в прошивку; неизвестный glyph заменяется U+FFFD.
Каталоги показываются страницами по 64 записи с переходами вперёд/назад.

`Notes → + Новая заметка` — имя и затем текст до **8192 байт UTF-8**.
Существующий файл не перезаписывается. Кириллическая клавиатура использует те же
события Furi Input: вращение — выбор, OK — символ. `EN` переключает пять раскладок,
`sp` — пробел, `nl` — новая строка, `clr` — очистка однострочного поля,
`<-` — Unicode backspace, `OK` — готово, `X` / Back — отмена. Пароли маскируются.

Позиции чтения и настройки приложения теперь хранятся на SD. Из NVS версии v0.2
читаются прежние Favorites, AP-пароль и позиции документа; старая NVS не стирается.
Разбиение строк изменилось вместе с геометрией интерфейса, поэтому старая
позиция строки приблизительна, хотя блок файла сохраняется. Системные параметры
новой базы и её собственное избранное Archive имеют свои форматы.

## Ввод и инструменты

Короткий click — OK; GPIO6 — Back. Удержание кнопки энкодера с вращением —
Left/Right по правилам базы. Долгое нажатие обрабатывает текущий экран; оно
не заменяет Back во всех приложениях. GPIO0 остаётся BOOT при включении.

В TikoOS отключено подавление клика на 150 ms после вращения и сохранено
выполнение первого действия после сна. Дребезг и сдвиг контактов при нажатии
обрабатываются драйвером базы. `Tools → Encoder diagnostics` показывает переходы,
invalid transitions, шаги и A/B. `Settings` содержит реверс и чувствительность
1–4 detents/step. Изменения сохраняются на SD. Есть подтверждаемый сброс шрифта/
энкодера, который не удаляет документы или избранное.

Calculator: вращением выберите символ, OK вводит; `C` очищает, `<` стирает,
`=` вычисляет слева направо. Деление на ноль и переполнение проверяются.
Clock / stopwatch — приложение базы: OK старт/пауза, Left сбрасывает; на T-Embed
Left получается удержанием кнопки и вращением влево. Секундомер не зависит от
изменения часов и сохраняет состояние до перезагрузки. Разрешение штатного UI — 1 s.

## Wi-Fi / Bluetooth / USB

В `Wi-Fi Transfer` доступны **AP**, **My network**, поиск/подключение, изменение
пароля и выключение Wi-Fi. Сеть AP — `TikoOS`, начальный пароль **`Tigran123456!`**.
Web login: **`tiko`**, пароль совпадает с паролем AP, в том числе в режиме My network.
Пароль роутера вводится отдельно и сохраняется средствами базы. Wi-Fi 2.4 GHz;
для My network телефон/ПК должен находиться в той же локальной сети.

Соединение STA теперь живёт в глобальном Wi-Fi service: сохраняется между
приложениями и восстанавливается при включении. Bluetooth использует штатную
политику владения радио; при включении Wi-Fi база освобождает ресурсы BLE,
управление Bluetooth — через одноимённый пункт/системные Settings.

WebUI встроен в бинарник и работает без HTML-файла на карте. Сервер обслуживает
запросы отдельной задачей, а не UI-loop. Сохранены авторизация, CSRF, проверка
путей, запись только в `/TikoOS`, TXT upload до 2 MiB через временный файл,
запрет перезаписи и подтверждение удаления файлов. Автоматический внешний SNTP
и обновления из upstream-релизов в профиле TikoOS не запускаются.

USB — **штатный `USB-Storage` в Desktop / Lock menu**: выйдите из TikoOS через
Back, откройте меню Desktop и включите USB-Storage с подключённым native USB
data-кабелем. Это **вся microSD**, а не только `/TikoOS`. Безопасно извлеките диск
на компьютере, затем завершите режим штатным интерфейсом. Не работайте с картой
одновременно через ПК и локальные приложения. ROM download через GPIO0 сохраняется;
USB Serial/JTAG и OTG могут переопределять USB-порт.

## Сборка

Нужны Git, Python (проверено на **3.12**), ESP-IDF **v5.4.1**, его инструменты
ESP32-S3, CMake и Ninja. Внешние компоненты закреплены в
`firmware/third_party.lock.json` и включены исходниками, Component Manager не
скачивает плавающие версии. C++-часть приложения собирается как C++17.

Для текущей облачной среды:

```sh
bash scripts/install_native.sh
python3 scripts/build_native.py
bash scripts/test_native.sh
```

Установка размещает SDK/инструменты вне checkout, не создаёт worktree,
не изменяет SDK и ничего не прошивает. `build_native.py` использует 4 jobs
(`TIKO_BUILD_JOBS` переопределяет), пишет только в игнорируемый `firmware/build-tiko`.
На CachyOS/Arch сначала подготовьте Git, Python и C/C++ compiler; SDK можно
расположить в другом месте через `IDF_PATH`, `IDF_TOOLS_PATH`, `IDF_PYTHON_ENV_PATH`.

В обычной установленной ESP-IDF-среде доступен стандартный путь:

```sh
cd firmware
export IDF_COMPONENT_MANAGER=0
idf.py -B build-tiko -DIDF_TARGET=esp32s3 \
  -DFLIPPER_BOARD=lilygo_t_embed_cc1101 \
  '-DSDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.tiko.defaults' app
```

Скрипт собирает target **app**, без сборки bootloader.
Результат: `firmware/build-tiko/TikoOS.bin`. Публикуемый `dist/firmware.bin`
обновляется только после успешной сборки, проверки образа и тестов.
Отчёт — [BUILD.md](BUILD.md), аппаратные сценарии — [TESTING.md](TESTING.md).

## Launcher и восстановление

Поставляется **application image**, не merged/factory dump. Для установки:

1. Сохраните резервные копии SD и Launcher/data.
2. Сверьте свою Plus-ревизию с [hardware notes](docs/HARDWARE.md).
3. Проверьте app-раздел Launcher: он должен вмещать **весь файл** из BUILD.md,
   около 2.7 MB; прежнего слота 1.5 MB недостаточно.
4. `Launcher → SD → firmware.bin → Install`. Не подтверждайте удаление Launcher,
   bootloader или нужных разделов при предложении изменить разметку.

Профиль `TIKO_LAUNCHER_APP` не требует dual-OTA или специальных разделов базы.
Встроенная прошивка других app-слотов отключена: обновление TikoOS выполняет Launcher.
`partitions_launcher_build.csv` служит только для проверки размера (4 MiB) при
сборке; она не включается в app-image и не прошивается. Приложение использует
существующую таблицу и bootloader Launcher; менять их для установки не требуется.
Запуск с конкретной таблицей/версией bootloader на устройстве здесь не проверен.
Совместимость Arduino v0.2 app не доказывает совместимость ESP-IDF v0.3 app.

При неудачном запуске сначала используйте штатный возврат в Launcher. Если он
не отвечает: удерживайте GPIO0/BOOT, нажмите RESET/переподключите питание,
дождитесь USB ROM device и восстановите официальный Launcher для своей платы
по его инструкции. Не шейте весь SDK build поверх Launcher и не выполняйте erase_flash.
Автоматическая USB-прошивка в наших build scripts отсутствует.

Для отката сохранены [v0.2 app](dist/TikoOS-v0.2.bin),
[v0.2 SD package](dist/TikoOS-v0.2-microSD.zip) и [инструкция](docs/LEGACY-v0.2.md).

## Проверки и ограничения

Сборка и 888 host assertions плюс 7 сценариев реального input-драйвера на
моделируемых GPIO выполнены. Это не аппаратные тесты. TFT, SD, Wi-Fi/BLE, USB,
питание, реальная отзывчивость и Launcher на вашей плате — **Unverified**.
Новая база также может содержать собственные дефекты; переход не доказывает,
что все прежние жалобы устранены. Карта FAT32/MBR, без hot swap. Настройки и
Wi-Fi credentials базы сохраняются на SD без аппаратного шифрования.
