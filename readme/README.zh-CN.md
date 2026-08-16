# 设备管理器 (Device Manager)

一个类似 Windows 设备管理器的硬件浏览器，基于 **Qt 6 Quick + C++ + xmake** 构建。
UI 复用了 **Material 3** 设计（illogical-impulse 的 "ii" 主题）；
后端从 Linux **sysfs** 枚举真实硬件。

> [English](../README.md) · **简体中文** · [繁體中文](README.zh-TW.md) · [日本語](README.ja.md) · [Русский](README.ru.md)

## 依赖

构建工具：[xmake](https://xmake.io)、C++17 编译器。

Qt 6（≥ 6.8.2）：Core、Gui、Qml、Quick、QuickControls2、Svg、Core5Compat
（外加 Wayland 平台插件）。更高的 6.x 版本（6.9、6.11 等）同样可以构建。

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

## 构建

```sh
xmake            # release 构建（自动探测 Qt；必要时 xmake f --qt=/usr/lib64/qt6）
```

`xmake f -m debug && xmake` 切换调试模式；`xmake f --toolchain=clang && xmake` 用 Clang。

## 运行

```sh
./build/linux/x86_64/release/device-manager                        # 默认 Wayland（不可用时自动回退 X11）
QT_QPA_PLATFORM=xcb ./build/linux/x86_64/release/device-manager    # 强制 X11
```

## 分发二进制文件

xmake 默认配置编译并链接为完全位置无关代码（无 `R_X86_64_COPY` 拷贝重定位、无直接
文本引用），因此产物同样可以在 Qt6Core 带 `GNU_PROPERTY_1_NEEDED_INDIRECT_EXTERN_ACCESS`
保护符号标记的发行版（如 Arch）上运行——包括普通 GCC 构建，已在 Arch 容器中实测。
直接把 `build/linux/x86_64/release/device-manager` 发给对方即可。

目标机器需要 Qt（≥ 构建时的次版本号，如 6.11）以及常见桌面基础库。

## 项目结构

- `src/DeviceManager.*` — sysfs 枚举引擎（设备解析、厂商/驱动名映射表、分组视图）
- `src/DriverHelper.*` — 驱动检测与安装后端（缺失驱动扫描、模块加载/绑定、发行版包搜索与安装、闭源驱动流程）
- `src/Theme.*`, `src/ColorUtils.*` — Material 3 主题层（`Appearance` / `ColorUtils` 上下文属性）
- `qml/Components/` — Material 3 组件库（`Components` QML 模块）
- `qml/fonts/` — 内嵌的 Material Symbols Rounded 变量字体
- `qml/Main.qml` — 主窗口（标题栏 / 工具栏 / 设备树 / 详情面板 / 状态栏）
- `qml/DeviceManager/` — 应用页面：设备列表、详情面板、属性对话框、状态徽章、属性列表

## 说明

- 设备信息来自 `/sys/class`、`/sys/bus` 与 `/proc/cpuinfo`；厂商/驱动名为内置小型映射表，
  未收录的显示原始 ID（如 `0x10ec`）。
- 状态判定：`power/runtime_status`（已暂停）、网卡 `carrier`（未连接）、`rfkill`（已禁用）。
- 主题跟随系统配色方案（`QStyleHints::colorScheme`）；手动切换深/浅色后保持用户选择。
- Wayland 需要 `qt6-wayland`；同时设置 `DISPLAY` 与 `WAYLAND_DISPLAY` 时 Qt 默认选 xcb，
  因此应用自身会设置 `QT_QPA_PLATFORM=wayland;xcb`。

## 许可证

- 项目代码：**GNU GPL v3** —— 参见 [LICENSE](../LICENSE)。
- 第三方组件：
  - **Material Symbols Rounded** 字体 —— [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0)（Google）
  - **M3 形状引擎**（`qml/Components/shapes/`）—— Apache License 2.0（见 `qml/Components/shapes/LICENSE`）
  - **UI 组件** —— 移植自 illogical-impulse 的 quickshell "ii" 主题（[end-4/dots-hyprland](https://github.com/end-4/dots-hyprland)），GNU GPL v3 许可证。
