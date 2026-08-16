cmake_minimum_required(VERSION 3.16)

project(device-manager
    VERSION 0.1.0
    DESCRIPTION "A Windows Device Manager-style hardware browser"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt 6.8.2 is the minimum supported version; newer 6.x releases (6.11, ...)
# are accepted too, so the same tree builds against either Qt line.
find_package(Qt6 6.8.2 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Svg
    Core5Compat
)

qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

qt_add_executable(device-manager
    src/main.cpp
    src/AboutInfo.cpp     src/AboutInfo.h
    src/ColorUtils.cpp    src/ColorUtils.h
    src/DeviceActions.cpp src/DeviceActions.h
    src/DeviceManager.cpp src/DeviceManager.h
    src/DeviceEntry.h
    src/DriverHelper.cpp   src/DriverHelper.h
    src/GraphicsProbe.cpp src/GraphicsProbe.h
    src/MonitorProbe.cpp  src/MonitorProbe.h
    src/SmartReader.cpp   src/SmartReader.h
    src/TemperatureProbe.cpp src/TemperatureProbe.h
    src/Theme.cpp         src/Theme.h
    src/Translator.cpp    src/Translator.h
    qml/qml.qrc
)

# Bake the project version into the build so the About dialog can show it.
target_compile_definitions(device-manager PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(device-manager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    Qt6::Core5Compat
)

if(MSVC)
    target_compile_options(device-manager PRIVATE /W4)
else()
    target_compile_options(device-manager PRIVATE -Wall -Wextra)
endif()

install(TARGETS device-manager
    RUNTIME DESTINATION bin
)