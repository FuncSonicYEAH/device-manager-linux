# Диспетчер устройств (Device Manager)

Браузер оборудования в стиле диспетчера устройств Windows, написанный на
**Qt 6 Quick + C++ + Meson**. Интерфейс использует дизайн **Material 3**
(тема "ii" от illogical-impulse); бэкенд
перечисляет реальное оборудование через интерфейс Linux **sysfs**.

> [English](../README.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · [日本語](README.ja.md) · **Русский**

## Зависимости

Инструменты сборки: meson, ninja, компилятор C++17 и pkg-config.

Модули Qt 6: Core, Gui, Qml, Quick, QuickControls2, Svg, Core5Compat
(плюс плагин платформы Wayland).

### Fedora

```sh
sudo dnf install meson ninja-build gcc-c++ \
  qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtsvg-devel qt6-qt5compat-devel qt6-qtwayland
```

### Ubuntu / Debian

```sh
sudo apt install meson ninja-build g++ pkg-config \
  qt6-base-dev qt6-declarative-dev qt6-5compat-dev libqt6svg6-dev qt6-wayland
```

### Arch Linux

```sh
sudo pacman -S --needed meson ninja gcc \
  qt6-base qt6-declarative qt6-svg qt6-5compat qt6-wayland
```

### openSUSE

```sh
sudo zypper install meson ninja gcc-c++ \
  qt6-base-devel qt6-declarative-devel qt6-svg-devel qt6-qt5compat-devel qt6-wayland
```

### Alpine Linux

```sh
sudo apk add meson ninja g++ pkgconfig \
  qt6-qtbase-dev qt6-qtdeclarative-dev qt6-qtsvg-dev qt6-qt5compat-dev qt6-qtwayland
```

### Gentoo

```sh
sudo emerge --ask dev-util/meson dev-util/ninja sys-devel/gcc \
  dev-qt/qtbase:6 dev-qt/qtdeclarative:6 dev-qt/qtsvg:6 dev-qt/qt5compat:6 dev-qt/qtwayland:6
```

## Сборка

```sh
meson setup build
ninja -C build
```

## Запуск

```sh
./build/device-manager                        # по умолчанию: Wayland, при недоступности — X11
QT_QPA_PLATFORM=xcb ./build/device-manager    # принудительно X11
```

## Структура проекта

- `src/DeviceManager.*` — движок перечисления sysfs (разбор устройств, таблицы имён производителей/драйверов, группировка)
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
