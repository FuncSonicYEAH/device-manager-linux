# Device Manager (设备管理器)

A Windows Device Manager–style hardware browser built with **Qt 6 Quick + C++ + xmake**.
The UI reuses the **Material 3** design (the "ii" theme by illogical-impulse); the
backend enumerates real hardware from the Linux **sysfs** interface.

> **English** · [简体中文](readme/README.zh-CN.md) · [繁體中文](readme/README.zh-TW.md) · [日本語](readme/README.ja.md) · [Русский](readme/README.ru.md)

## Dependencies

Build tools: [xmake](https://xmake.io) and a C++17 compiler.

Qt 6 (≥ 6.8.2): Core, Gui, Qml, Quick, QuickControls2, Svg, Core5Compat
(plus the Wayland platform plugin). Any newer 6.x release — 6.9, 6.11, ...
— works as well.

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

## Build

```sh
xmake            # release build (Qt auto-detected; if needed: xmake f --qt=/usr/lib64/qt6)
```

`xmake f -m debug && xmake` switches to a debug build; `xmake f --toolchain=clang && xmake` builds with Clang.

## Run

```sh
./build/linux/x86_64/release/device-manager                        # default: Wayland, falls back to X11
QT_QPA_PLATFORM=xcb ./build/linux/x86_64/release/device-manager    # force X11
```

## Distributing the binary

The default xmake configuration compiles and links fully position-independent
(no `R_X86_64_COPY` relocations, no direct text references), so the resulting
binary also runs on distros whose Qt6Core marks its symbols protected with
`GNU_PROPERTY_1_NEEDED_INDIRECT_EXTERN_ACCESS` (e.g. Arch) — plain GCC builds
included, verified in an Arch container. Just hand over
`build/linux/x86_64/release/device-manager`.

The target machine needs Qt (≥ the minor version used at build time, e.g.
6.11) and the usual desktop libraries.


Qt symbol versions are backwards compatible, so for the widest reach build on
the oldest distribution you want to support: a binary built on Debian trixie
(Qt 6.8.2) also runs on Fedora/Arch with Qt 6.11, while the reverse fails with
`version Qt_6.11 not found` (verified in distrobox containers).

## Layout

- `src/DeviceManager.*` — sysfs enumeration engine (device parsing, vendor/driver name tables, grouped views)
- `src/DriverHelper.*` — driver detection & install backend (missing-driver scan, module load/bind, distro package search & install, proprietary driver flows)
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
