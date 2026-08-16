# 裝置管理員 (Device Manager)

一個類似 Windows 裝置管理員的硬體瀏覽器，基於 **Qt 6 Quick + C++ + xmake** 建置。
UI 沿用 **Material 3** 設計（illogical-impulse 的 "ii" 主題）；
後端從 Linux **sysfs** 列舉真實硬體。

> [English](../README.md) · [简体中文](README.zh-CN.md) · **繁體中文** · [日本語](README.ja.md) · [Русский](README.ru.md)

## 相依套件

建置工具：[xmake](https://xmake.io)、C++17 編譯器。

Qt 6（≥ 6.8.2）：Core、Gui、Qml、Quick、QuickControls2、Svg、Core5Compat
（外加 Wayland 平台外掛程式）。更高的 6.x 版本（6.9、6.11 等）同樣可以建置。

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

## 建置

```sh
xmake            # release 建置（自動探測 Qt；必要時 xmake f --qt=/usr/lib64/qt6）
```

`xmake f -m debug && xmake` 切換除錯模式；`xmake f --toolchain=clang && xmake` 使用 Clang。

## 執行

```sh
./build/linux/x86_64/release/device-manager                        # 預設 Wayland（無法使用時自動回退 X11）
QT_QPA_PLATFORM=xcb ./build/linux/x86_64/release/device-manager    # 強制 X11
```

## 分發二進位檔

xmake 預設配置編譯並連結為完全位置無關程式碼（無 `R_X86_64_COPY` 拷貝重定位、無直接
文字引用），因此產物同樣可以在 Qt6Core 帶 `GNU_PROPERTY_1_NEEDED_INDIRECT_EXTERN_ACCESS`
保護符號標記的發行版（如 Arch）上執行——包括普通 GCC 建置，已在 Arch 容器中實測。
直接把 `build/linux/x86_64/release/device-manager` 傳給對方即可。

目標機器需要 Qt（≥ 建置時的次版本號，如 6.11）以及常見桌面基礎庫。

## 專案結構

- `src/DeviceManager.*` — sysfs 列舉引擎（裝置解析、廠商/驅動程式名稱對應表、分組檢視）
- `src/DriverHelper.*` — 驅動檢測與安裝後端（缺失驅動掃描、模組載入/綁定、發行版套件搜尋與安裝、閉源驅動流程）
- `src/Theme.*`, `src/ColorUtils.*` — Material 3 主題層（`Appearance` / `ColorUtils` 內容屬性）
- `qml/Components/` — Material 3 元件庫（`Components` QML 模組）
- `qml/fonts/` — 內嵌的 Material Symbols Rounded 變數字型
- `qml/Main.qml` — 主視窗（標題列 / 工具列 / 裝置樹 / 詳細資料面板 / 狀態列）
- `qml/DeviceManager/` — 應用程式頁面：裝置清單、詳細資料面板、內容對話方塊、狀態徽章、屬性清單

## 備註

- 裝置資訊來自 `/sys/class`、`/sys/bus` 與 `/proc/cpuinfo`；廠商/驅動程式名為內建小型
  對應表，未收錄的顯示原始 ID（如 `0x10ec`）。
- 狀態判定：`power/runtime_status`（已暫停）、網路卡 `carrier`（未連接）、`rfkill`（已停用）。
- 主題跟隨系統配色方案（`QStyleHints::colorScheme`）；手動切換深/淺色後保留使用者選擇。
- Wayland 需要 `qt6-wayland`；同時設定 `DISPLAY` 與 `WAYLAND_DISPLAY` 時 Qt 預設選 xcb，
  因此應用程式本身會設定 `QT_QPA_PLATFORM=wayland;xcb`。

## 授權條款

- 專案程式碼：**GNU GPL v3** —— 參見 [LICENSE](../LICENSE)。
- 第三方元件：
  - **Material Symbols Rounded** 字型 —— [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0)（Google）
  - **M3 形狀引擎**（`qml/Components/shapes/`）—— Apache License 2.0（見 `qml/Components/shapes/LICENSE`）
  - **UI 元件** —— 移植自 illogical-impulse 的 quickshell "ii" 主題（[end-4/dots-hyprland](https://github.com/end-4/dots-hyprland)），GNU GPL v3 授權。
