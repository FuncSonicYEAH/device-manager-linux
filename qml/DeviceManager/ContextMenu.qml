// Material-styled popup context menu used by the device list (right-click).
// `menuItems` is a list of { icon, text, enabled, action } objects; entries
// with an empty `text` render as separators. itemActivated() carries the
// index of the clicked entry.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

Popup {
    id: root

    property var menuItems: []

    signal itemActivated(int index)

    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape
    padding: 6
    width: 250

    background: Rectangle {
        radius: Appearance.rounding.normal
        color: Appearance.m3colors.m3surfaceContainerLow
        border.color: Appearance.colors.colOutlineVariant
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 2

        Repeater {
            model: root.menuItems

            delegate: Item {
                required property int index
                required property var modelData

                Layout.fillWidth: true
                Layout.preferredHeight: modelData.text === "" ? 10 : 36

                Rectangle {
                    visible: modelData.text === ""
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    height: 1
                    color: Appearance.colors.colOutlineVariant
                    opacity: 0.4
                }

                RippleButton {
                    id: menuBtn
                    visible: modelData.text !== ""
                    anchors.fill: parent
                    enabled: modelData.enabled !== false
                    buttonRadius: Appearance.rounding.small
                    rippleEnabled: false
                    onClicked: {
                        root.close()
                        root.itemActivated(index)
                    }

                    contentItem: RowLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 10

                        MaterialSymbol {
                            text: modelData.icon
                            iconSize: 18
                            color: menuBtn.enabled
                                ? Appearance.colors.colOnLayer1
                                : Appearance.colors.colOnLayer1Inactive
                        }
                        StyledText {
                            text: modelData.text
                            Layout.fillWidth: true
                            font.pixelSize: Appearance.font.pixelSize.smallie
                            color: menuBtn.enabled
                                ? Appearance.colors.colOnLayer1
                                : Appearance.colors.colOnLayer1Inactive
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
