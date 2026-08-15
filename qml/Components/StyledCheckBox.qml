// Port-style of modules/common/widgets/StyledCheckBox.qml (Material 3 checkbox)
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

CheckBox {
    id: root

    padding: 4
    implicitHeight: contentItem.implicitHeight + padding * 2
    property string description
    property color activeColor: Appearance.colors.colPrimary
    property color inactiveColor: Appearance.m3colors.m3onSurfaceVariant

    indicator: Item {}

    contentItem: RowLayout {
        id: contentItem
        Layout.fillWidth: true
        spacing: 12

        Rectangle {
            id: box
            Layout.alignment: Qt.AlignVCenter
            width: 20
            height: 20
            radius: 5
            border.color: checked ? root.activeColor : root.inactiveColor
            border.width: 2
            color: checked ? root.activeColor : "transparent"

            Behavior on color {
                ColorAnimation {
                    duration: Appearance.animation.elementMoveFast.duration
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
                }
            }
            Behavior on border.color {
                ColorAnimation {
                    duration: Appearance.animation.elementMoveFast.duration
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
                }
            }

            MaterialSymbol {
                anchors.centerIn: parent
                width: 18
                height: 18
                iconSize: 18
                text: root.checkState === Qt.CheckState.Checked ? "check"
                    : root.checkState === Qt.CheckState.PartiallyChecked ? "remove" : ""
                color: Appearance.m3colors.m3onPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                opacity: root.checkState === Qt.CheckState.Unchecked ? 0 : 1

                Behavior on opacity {
                    NumberAnimation {
                        duration: Appearance.animation.elementMoveFast.duration
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
                    }
                }
            }
        }

        StyledText {
            text: root.description
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Appearance.m3colors.m3onSurface
        }
    }
}
