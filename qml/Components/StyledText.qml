// Port of modules/common/widgets/StyledText.qml
import QtQuick

Text {
    id: root

    renderType: Text.NativeRendering
    verticalAlignment: Text.AlignVCenter
    font {
        hintingPreference: Font.PreferDefaultHinting
        family: Appearance.font.family.main
        pixelSize: Appearance.font.pixelSize.small
    }
    color: Appearance.m3colors.m3onBackground
    linkColor: Appearance.m3colors.m3primary
}
