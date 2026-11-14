# Миграция на Furi / ESP-IDF

Исходная база: Sor3nt/Flipper-Zero-ESP32-Port,
`e6ec11972a8e025d7ab686a6ef3016cf52360834`.
Полные URL/commits SDK и внешних компонентов — `firmware/third_party.lock.json`.

## Интеграция

В `firmware/` включены исходники системной базы. Профиль `fam_config.py` выбирает
GUI/Input, Storage, Desktop/Loader, Notification/Power, Bluetooth, Wi-Fi, Archive,
Clock и настройки. Приложения TikoOS добавлены компонентом `components/tiko`.
Генератор FAM регистрирует TikoOS и запускает его после старта служб.

Файловый менеджер, USB Storage, Bluetooth и Clock/секундомер используются из базы.
Повторная реализация этих подсистем Arduino не включена в новый бинарник.
Учебные приложения, UTF-8 keyboard model, проверка путей, Calculator/Monitor
сохранены/адаптированы. В C++ части нет собственного poll-loop для GPIO или TFT
драйвера: она получает InputEvent через ViewDispatcher и рисует через Canvas.
Чтение файлов и новые заметки проходят через сервис Storage.

Wi-Fi connect/reconnect, scan worker, владение радио и BT реализует upstream HAL.
HTTP lifecycle взят из upstream Web-Filesystem, интерфейс страницы адаптирован
и встроен в flash. Добавлены Basic auth, CSRF, строгий URL decode, sandbox
`/ext/TikoOS`, ограничение TXT/2 MiB, временный upload и запрет перезаписи.
HTTP работает отдельной задачей. Большие операции по общей SPI могут всё ещё
конкурировать с дисплеем; реальные задержки нужно замерить.

## Изменения базы для TikoOS

- Branding/app descriptor: TikoOS 0.3.0; авторство исходной базы сохранено.
- Board: LCD RST -1 по LilyGO; GPIO40 оставлен I2S, сброс панели программный.
  Исправлены manual reset и sleep pin tables для отсутствующего reset GPIO.
- Shared-SPI SD recovery включён через board define без обязательного BW16.
- Нет SDK raw-Wi-Fi patch и WIFITE adapter; драйвер ESP-IDF на диске не менялся.
  Невключённые WIFITE exports удалены из FAP table, функции не заменены заглушками.
- Нет автоматического стирания NVS при init error и внешнего SNTP по умолчанию.
- Кириллица в Canvas secondary/primary font и проверенный DejaVu subset читателя.
- Исправлен include path компонента Locale, который скрывал libc `locale.h` от C++.
- GPIO драйвер базы сохранён с shaft-jostle/debounce/modifier логикой. Для TikoOS
  снят 150 ms post-rotation click veto; физическая кнопка читается до обработки
  quadrature, чтобы jostle при нажатии не передвигал выбор. Реверс/чувствительность
  и счётчики переходов добавлены через небольшой API целевой платы.
- Первое действие после сна передаётся приложению и службе пробуждения.
- Clock использует монотонное время секундомера и сохраняет его состояние в RAM
  между открытиями. Календарное время/дата остаются штатными.

## Данные и установка

Физическая структура `TikoOS/notes`, `formulas` и пользовательские TXT не меняется.
`/ext/TikoOS` — имя того же каталога в Furi Storage, а не новая папка `ext` на карте.
Новая конфигурация приложения — `/TikoOS/config`, Favorites —
`/TikoOS/favorites/list.txt`, позиции — `/TikoOS/config/positions`.
Favorites/AP-пароль/позиции v0.2 читаются из NVS read-only при отсутствии нового
файла. Строки читателя после смены геометрии не обязаны совпадать. Системные UI
настройки базы не копируются побайтно из Arduino Preferences.

Не копируйте пустой favorites/list.txt поверх своего списка: пакет его не содержит.
Пакет называется `TikoOS-microSD.zip`, содержит отдельный app binary и ресурсы
Desktop `dolphin/`. Не путать с полным SD-пакетом upstream или его factory image.
`TikoOS-v0.2.bin` и исторический пакет сохранены для отката.

Фактический размер/хеш и проверки — BUILD.md. Профиль TIKO_LAUNCHER_APP собирает только app target: dual-OTA и специальная
таблица базы не требуются. In-app OTA отключено, чтобы не выбирать чужой app-слот.
Сборочная partitions_launcher_build.csv задаёт только лимит 4 MiB и не прошивается.
Размер 1.5 MB недостаточен.
Разметка/bootloader на устройстве не проверены; автоматической прошивки не было.

## Уточнение: app-image без отдельного bootloader

По требованию пользователя в поставке нет bootloader.bin/partitions.bin/merged
factory image. build_native.py собирает target app; package_native.py копирует
ровно TikoOS.bin, а не результат merge-bin. Отключены in-app OTA записи, rollback
зависимость и boot-миграция U2F partition: наличие особых разделов базы не требуется.
Обычные системные ресурсы ESP32/Launcher (существующий bootloader, таблица, NVS)
по-прежнему используются; аппаратный boot flow ESP32 нельзя устранить из app.
