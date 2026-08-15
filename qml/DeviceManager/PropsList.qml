// Key/value property list used by the details pane and the properties dialog.
import QtQuick
import QtQuick.Layouts
import Components

ColumnLayout {
    id: root

    property var props: []
    property bool showDividers: false
    property int nameWidth: 140

    spacing: showDividers ? 0 : 6

    Repeater {
        model: root.props
        delegate: ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Rectangle {
                visible: root.showDividers
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Appearance.colors.colOutlineVariant
                opacity: 0.35
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: root.showDividers ? 7 : 0
                Layout.bottomMargin: root.showDividers ? 7 : 0
                spacing: 16

                StyledText {
                    text: modelData.name
                    Layout.preferredWidth: root.nameWidth
                    Layout.alignment: Qt.AlignTop
                    font.pixelSize: Appearance.font.pixelSize.smallie
                    font.weight: Font.Medium
                    color: Appearance.m3colors.m3onSurface
                    wrapMode: Text.Wrap
                }
                StyledText {
                    text: modelData.value
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    font.pixelSize: Appearance.font.pixelSize.smallie
                    color: Appearance.m3colors.m3onSurface
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
            }
        }
    }
}
