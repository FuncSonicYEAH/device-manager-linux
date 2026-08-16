set_project("device-manager")
set_version("0.1.0")
set_languages("c++17")
set_encodings("utf-8")

add_rules("mode.debug", "mode.release")

target("device-manager")
    set_kind("binary")
    add_rules("qt.quickapp")

    add_files("src/*.cpp")
    -- headers go through moc; plain ones (DeviceEntry.h) are skipped
    add_files("src/*.h")
    -- QML, fonts and translations are baked in from qml.qrc via rcc
    add_files("qml/qml.qrc")

    add_frameworks("QtQuick", "QtQml", "QtQuickControls2", "QtSvg", "QtCore5Compat")

    -- version macro for the About dialog; keep in sync with set_version()
    add_defines('APP_VERSION="0.1.0"')
