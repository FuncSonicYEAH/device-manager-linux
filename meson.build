project('device-manager', 'cpp',
  version: '0.1.0',
  meson_version: '>=1.1.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=2',
    'buildtype=release',
  ],
)

qt6 = import('qt6')
qt6_dep = dependency('qt6', modules: ['Core', 'Gui', 'Qml', 'Quick', 'QuickControls2', 'Svg', 'Core5Compat'])

# Bake the project version into the build so the About dialog can show it.
add_project_arguments('-DAPP_VERSION="' + meson.project_version() + '"', language: 'cpp')

sources = files(
  'src/main.cpp',
  'src/ColorUtils.cpp',
  'src/Theme.cpp',
  'src/DeviceManager.cpp',
  'src/DeviceActions.cpp',
  'src/SmartReader.cpp',
  'src/GraphicsProbe.cpp',
  'src/AboutInfo.cpp',
  'src/Translator.cpp',
)

moc = qt6.preprocess(
  moc_headers: files(
    'src/ColorUtils.h',
    'src/Theme.h',
    'src/DeviceManager.h',
    'src/DeviceActions.h',
    'src/SmartReader.h',
    'src/GraphicsProbe.h',
    'src/AboutInfo.h',
    'src/Translator.h',
  ),
)

resources = qt6.compile_resources(
  sources: files('qml/qml.qrc'),
)

executable('device-manager',
  sources,
  moc,
  resources,
  dependencies: qt6_dep,
  install: true,
)
