# Device Manager (设备管理器)

A Windows Device Manager–style hardware browser built with **Qt 6 Quick + C++ + CMake**.
The UI reuses the **Material 3** design (the "ii" theme by illogical-impulse); the
backend enumerates real hardware from the Linux **sysfs** interface.

> **English** · [简体中文](readme/README.zh-CN.md) · [繁體中文](readme/README.zh-TW.md) · [日本語](readme/README.ja.md) · [Русский](readme/README.ru.md)

## Dependencies

Build tools: CMake (≥ 3.16), ninja and a C++17 compiler.

Qt 6 (≥ 6.8.2): Core, Gui, Qml, Quick, QuickControls2, Svg, Core5Compat
(plus the Wayland platform plugin). Any newer 6.x release — 6.9, 6.11, ...
— works as well.

### Fedora

```sh
sudo dnf install cmake ninja-build gcc-c++ \
  qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtsvg-devel qt6-qt5compat-devel qt6-qtwayland
```

### Ubuntu / Debian

```sh
sudo apt install cmake ninja-build g++ \
  qt6-base-dev qt6-declarative-dev qt6-5compat-dev libqt6svg6-dev qt6-wayland
```

### Arch Linux

```sh
sudo pacman -S --needed cmake ninja gcc \
  qt6-base qt6-declarative qt6-svg qt6-5compat qt6-wayland
```

### openSUSE

```sh
sudo zypper install cmake ninja gcc-c++ \
  qt6-base-devel qt6-declarative-devel qt6-svg-devel qt6-qt5compat-devel qt6-wayland
```

### Alpine Linux

```sh
sudo apk add cmake ninja g++ \
  qt6-qtbase-dev qt6-qtdeclarative-dev qt6-qtsvg-dev qt6-qt5compat-dev qt6-qtwayland
```

### Gentoo

```sh
sudo emerge --ask dev-util/cmake dev-util/ninja sys-devel/gcc \
  dev-qt/qtbase:6 dev-qt/qtdeclarative:6 dev-qt/qtsvg:6 dev-qt/qt5compat:6 dev-qt/qtwayland:6
```

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run

```sh
./build/device-manager                        # default: Wayland, falls back to X11
QT_QPA_PLATFORM=xcb ./build/device-manager    # force X11
```

## Layout

- `src/DeviceManager.*` — sysfs enumeration engine (device parsing, vendor/driver name tables, grouped views)
- `src/Translator.*` — lightweight i18n (dictionaries in `qml/i18n/translations.json`, exposed as the `Tr` context property)
- `src/Theme.*`, `src/ColorUtils.*` — Material 3 theme layer (`Appearance` / `ColorUtils` context properties)
- `qml/Components/` — Material 3 widget set (the `Components` QML module)
- `qml/fonts/` — bundled Material Symbols Rounded variable font
- `qml/i18n/translations.json` — UI translations keyed by stable English identifiers (zh-CN / zh-TW / en / ja / ru)
- `qml/Main.qml` — main window (title bar / toolbar / device tree / details pane / status bar)
- `qml/DeviceManager/` — app pages: device list, details pane, properties dialog, status badge, props list

## Notes

- Device information comes from `/sys/class`, `/sys/bus` and `/proc/cpuinfo`. Specific model
  names for PCI/USB devices are looked up in the system's `pci.ids` / `usb.ids` databases
  (`/usr/share/hwdata`, `/usr/share/misc`, ...); without them, small built-in vendor/driver
  tables are used and unknown IDs are shown raw (e.g. `0x10ec`).
- Status detection: `power/runtime_status` (suspended), network `carrier` (unplugged),
  `rfkill` (disabled).
- The theme follows the system color scheme (`QStyleHints::colorScheme`); toggling
  dark/light manually keeps your choice.
- Wayland requires `qt6-wayland`. When both `DISPLAY` and `WAYLAND_DISPLAY` are set,
  Qt would pick xcb by default, so the app sets `QT_QPA_PLATFORM=wayland;xcb` itself.

## License

- Project code: **GNU GPL v3** — see [LICENSE](LICENSE).
- Third-party components:
  - **Material Symbols Rounded** font — [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0) (Google)
  - **M3 shape engine** (`qml/Components/shapes/`) — Apache License 2.0 (see `qml/Components/shapes/LICENSE`)
  - **UI components** ported from the quickshell “ii” theme by illogical-impulse ([end-4/dots-hyprland](https://github.com/end-4/dots-hyprland)) — GNU GPL v3
