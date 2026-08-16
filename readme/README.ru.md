# Диспетчер устройств (Device Manager)

Браузер оборудования в стиле диспетчера устройств Windows, написанный на
**Qt 6 Quick + C++ + xmake**. Интерфейс использует дизайн **Material 3**
(тема "ii" от illogical-impulse); бэкенд
перечисляет реальное оборудование через интерфейс Linux **sysfs**.

> [English](../README.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · [日本語](README.ja.md) · **Русский**

## Зависимости

Инструменты сборки: [xmake](https://xmake.io) и компилятор C++17.

Qt 6 (≥ 6.8.2): Core, Gui, Qml, Quick, QuickControls2, Svg, Core5Compat
(плюс плагин платформы Wayland). Более новые выпуски 6.x (6.9, 6.11 и т. д.)
также собираются без изменений.

### Fedora

```sh
sudo dnf install gcc-c++ \
  qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtsvg-devel qt6-qt5compat-devel qt6-qtwayland
```

### Ubuntu / Debian

```sh
sudo apt install g++ \
  qt6-base-dev qt6-declarative-dev qt6-5compat-dev libqt6svg6-dev qt6-wayland
```

### Arch Linux

```sh
sudo pacman -S --needed gcc \
  qt6-base qt6-declarative qt6-svg qt6-5compat qt6-wayland
```

### openSUSE

```sh
sudo zypper install gcc-c++ \
  qt6-base-devel qt6-declarative-devel qt6-svg-devel qt6-qt5compat-devel qt6-wayland
```

### Alpine Linux

```sh
sudo apk add g++ \
  qt6-qtbase-dev qt6-qtdeclarative-dev qt6-qtsvg-dev qt6-qt5compat-dev qt6-qtwayland
```

### Gentoo

```sh
sudo emerge --ask sys-devel/gcc \
  dev-qt/qtbase:6 dev-qt/qtdeclarative:6 dev-qt/qtsvg:6 dev-qt/qt5compat:6 dev-qt/qtwayland:6
```

## Сборка

```sh
xmake            # release-сборка (Qt определяется автоматически; при необходимости xmake f --qt=/usr/lib64/qt6)
```

`xmake f -m debug && xmake` — отладочная сборка; `xmake f --toolchain=clang && xmake` — Clang.

## Запуск

```sh
./build/linux/x86_64/release/device-manager                        # по умолчанию: Wayland, при недоступности — X11
QT_QPA_PLATFORM=xcb ./build/linux/x86_64/release/device-manager    # принудительно X11
```

## Распространение бинарного файла

Конфигурация xmake по умолчанию собирает полностью позиционно-независимый код
(без перемещений `R_X86_64_COPY` и прямых ссылок в тексте), поэтому бинарный
файл работает и в дистрибутивах, где Qt6Core помечает символы protected с
`GNU_PROPERTY_1_NEEDED_INDIRECT_EXTERN_ACCESS` (например, Arch) — включая
обычную сборку GCC, проверено в контейнере Arch. Просто передайте
`build/linux/x86_64/release/device-manager`.

На целевой машине нужны Qt (≥ минорной версии сборки, например 6.11) и
стандартные библиотеки рабочего стола.


Версии символов Qt обратно совместимы, поэтому для максимального охвата
собирайте на самом старом дистрибутиве из поддерживаемых: бинарный файл,
собранный на Debian trixie (Qt 6.8.2), работает и на Fedora/Arch с Qt 6.11,
а в обратную сторону — ошибка `version Qt_6.11 not found`
(проверено в контейнерах distrobox).

## Структура проекта

- `src/DeviceManager.*` — движок перечисления sysfs (разбор устройств, таблицы имён производителей/драйверов, группировка)
- `src/DriverHelper.*` — движок поиска и установки драйверов (скан устройств без драйверов, загрузка/привязка модулей, поиск и установка пакетов дистрибутива, проприетарные драйверы)
- `src/Theme.*`, `src/ColorUtils.*` — слой темы Material 3 (контекстные свойства `Appearance` / `ColorUtils`)
- `qml/Components/` — набор виджетов Material 3 (QML-модуль `Components`)
- `qml/fonts/` — встроенный вариативный шрифт Material Symbols Rounded
- `qml/Main.qml` — главное окно (заголовок / панель инструментов / дерево устройств / панель сведений / строка состояния)
- `qml/DeviceManager/` — страницы приложения: список устройств, панель сведений, диалог свойств, значок состояния, список свойств

## Примечания

- Сведения об устройствах берутся из `/sys/class`, `/sys/bus` и `/proc/cpuinfo`;
  имена производителей/драйверов — из небольших встроенных таблиц, неизвестные
  идентификаторы показываются как есть (например, `0x10ec`).
- Определение состояния: `power/runtime_status` (приостановлено), `carrier` сетевого
  интерфейса (не подключено), `rfkill` (отключено).
- Тема следует системной цветовой схеме (`QStyleHints::colorScheme`); ручное
  переключение тёмной/светлой темы сохраняет ваш выбор.
- Для Wayland требуется `qt6-wayland`. Когда заданы и `DISPLAY`, и `WAYLAND_DISPLAY`,
  Qt по умолчанию выбирает xcb, поэтому приложение само устанавливает
  `QT_QPA_PLATFORM=wayland;xcb`.

## Лицензия

- Код проекта: **GNU GPL v3** — см. [LICENSE](../LICENSE).
- Сторонние компоненты:
  - Шрифт **Material Symbols Rounded** — [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0) (Google)
  - **Движок фигур M3** (`qml/Components/shapes/`) — Apache License 2.0 (см. `qml/Components/shapes/LICENSE`)
  - **UI-компоненты** — портированы из темы quickshell "ii" от illogical-impulse
    ([end-4/dots-hyprland](https://github.com/end-4/dots-hyprland)), лицензия GNU GPL v3.
