// Port of modules/common/widgets/StyledRadioButton.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

RadioButton {
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
            id: radio
            Layout.alignment: Qt.AlignVCenter
            width: 20
            height: 20
            radius: Appearance.rounding.full
            border.color: checked ? root.activeColor : root.inactiveColor
            border.width: 2
            color: "transparent"

            Rectangle {
                anchors.centerIn: parent
                width: checked ? 10 : 4
                height: checked ? 10 : 4
                radius: Appearance.rounding.full
                color: Appearance.colors.colPrimary
                opacity: checked ? 1 : 0

                Behavior on opacity {
                    NumberAnimation {
                        duration: Appearance.animation.elementMoveFast.duration
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
                    }
                }
                Behavior on width {
                    NumberAnimation {
                        duration: Appearance.animation.elementMove.duration
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Appearance.animation.elementMove.bezierCurve
                    }
                }
                Behavior on height {
                    NumberAnimation {
                        duration: Appearance.animation.elementMove.duration
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Appearance.animation.elementMove.bezierCurve
                    }
                }
            }

            Rectangle {
                anchors.centerIn: parent
                width: root.hovered ? 40 : 20
                height: root.hovered ? 40 : 20
                radius: Appearance.rounding.full
                color: Appearance.m3colors.m3onSurface
                opacity: root.hovered ? 0.1 : 0

                Behavior on opacity {
                    NumberAnimation {
                        duration: Appearance.animation.elementMoveFast.duration
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
                    }
                }
                Behavior on width {
                    NumberAnimation {
                        duration: Appearance.animation.elementMove.duration
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Appearance.animation.elementMove.bezierCurve
                    }
                }
                Behavior on height {
                    NumberAnimation {
                        duration: Appearance.animation.elementMove.duration
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Appearance.animation.elementMove.bezierCurve
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
