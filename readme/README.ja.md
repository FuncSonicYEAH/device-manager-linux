# デバイスマネージャー (Device Manager)

Windows のデバイスマネージャー風のハードウェアブラウザです。**Qt 6 Quick + C++ + CMake**
で構築されています。UI は **Material 3** デザイン
(illogical-impulse 氏の "ii" テーマ) を再利用し、バックエンドは Linux の **sysfs**
から実際のハードウェアを列挙します。

> [English](../README.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · **日本語** · [Русский](README.ru.md)

## 依存関係

ビルドツール: CMake（≥ 3.16）、ninja、C++17 コンパイラー。

Qt 6（≥ 6.8.2）: Core、Gui、Qml、Quick、QuickControls2、Svg、Core5Compat
（さらに Wayland プラットフォームプラグイン）。これより新しい 6.x リリース
（6.9、6.11 など）でもビルドできます。

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

## ビルド

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## 実行

```sh
./build/device-manager                        # 既定: Wayland（不可なら X11 にフォールバック）
QT_QPA_PLATFORM=xcb ./build/device-manager    # X11 を強制
```

## バイナリの配布

`GNU_PROPERTY_1_NEEDED_INDIRECT_EXTERN_ACCESS` ノートを持たない Qt6Core（例: Fedora）に対してリンクしたバイナリには `R_X86_64_COPY` リロケーションが含まれ、このノートを持つ Qt を採用するディストリビューション（例: Arch）では起動に失敗します:

```
error due to GNU_PROPERTY_1_NEEDED_INDIRECT_EXTERN_ACCESS
```

どちらの環境でも動くバイナリを作るには Clang を使ってください。CMake が `-fno-direct-access-external-data` と `-z nocopyreloc` を自動的に追加します（GCC に同等のオプションはありません）:

```sh
cmake -S . -B build-clang -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build build-clang
./build-clang/device-manager    # R_X86_64_COPY リロケーション・ゼロ
```

実行側のマシンには Qt 6.8.2 以上のランタイム（QtCore、QtGui、QtQml、QtQuick、QtQuickControls2、QtSvg、QtCore5Compat）が必要です。

## 構成

- `src/DeviceManager.*` — sysfs 列挙エンジン（デバイス解析、ベンダー/ドライバー名テーブル、グループビュー）
- `src/DriverHelper.*` — ドライバー検出・インストールのバックエンド（ドライバー不足スキャン、モジュールのロード/バインド、ディストリパッケージの検索とインストール、プロプライエタリドライバー処理）
- `src/Theme.*`, `src/ColorUtils.*` — Material 3 テーマ層（`Appearance` / `ColorUtils` コンテキストプロパティ）
- `qml/Components/` — Material 3 ウィジェット集（`Components` QML モジュール）
- `qml/fonts/` — 同梱の Material Symbols Rounded 可変フォント
- `qml/Main.qml` — メインウィンドウ（タイトルバー / ツールバー / デバイスツリー / 詳細ペイン / ステータスバー）
- `qml/DeviceManager/` — アプリのページ: デバイス一覧、詳細ペイン、プロパティダイアログ、ステータスバッジ、プロパティ一覧

## 備考

- デバイス情報は `/sys/class`、`/sys/bus`、`/proc/cpuinfo` から取得。ベンダー/ドライバー名は
  内蔵の小規模な対応表を使用し、未収録の ID はそのまま表示します（例: `0x10ec`）。
- 状態判定: `power/runtime_status`（一時停止）、ネットワークの `carrier`（未接続）、
  `rfkill`（無効）。
- テーマはシステムの配色スキーム（`QStyleHints::colorScheme`）に追従します。手動で
  ダーク/ライトを切り替えると、その選択が維持されます。
- Wayland には `qt6-wayland` が必要です。`DISPLAY` と `WAYLAND_DISPLAY` の両方が設定
  されている場合、Qt は既定で xcb を選ぶため、アプリ自身が
  `QT_QPA_PLATFORM=wayland;xcb` を設定します。

## ライセンス

- プロジェクトコード: **GNU GPL v3** — [LICENSE](../LICENSE) を参照。
- サードパーティコンポーネント:
  - **Material Symbols Rounded** フォント — [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0)（Google）
  - **M3 シェイプエンジン**（`qml/Components/shapes/`）— Apache License 2.0（`qml/Components/shapes/LICENSE` を参照）
  - **UI コンポーネント** — illogical-impulse 氏の quickshell "ii" テーマ（[end-4/dots-hyprland](https://github.com/end-4/dots-hyprland)）からの移植、GNU GPL v3 ライセンス。
