// Port of modules/common/widgets/StyledProgressBar.qml
// Material 3 progress bar. https://m3.material.io/components/progress-indicators/overview
import QtQuick
import QtQuick.Controls

ProgressBar {
    id: root

    property real valueBarWidth: 120
    property real valueBarHeight: 4
    property real valueBarGap: 4
    property color highlightColor: Appearance.colors.colPrimary
    property color trackColor: Appearance.m3colors.m3secondaryContainer

    Behavior on value {
        NumberAnimation {
            duration: Appearance.animation.elementMoveEnter.duration
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Appearance.animation.elementMoveEnter.bezierCurve
        }
    }

    background: Item {
        implicitHeight: root.valueBarHeight
        implicitWidth: root.valueBarWidth
    }

    contentItem: Item {
        id: contentItem
        anchors.fill: parent

        Rectangle {
            anchors.left: parent.left
            width: contentItem.width * root.visualPosition
            height: contentItem.height
            radius: height / 2
            color: root.highlightColor
        }

        Rectangle {
            anchors.right: parent.right
            width: (1 - root.visualPosition) * parent.width - root.valueBarGap
            height: parent.height
            radius: height / 2
            color: root.trackColor
        }

        Rectangle {
            anchors.right: parent.right
            width: root.valueBarGap
            height: root.valueBarGap
            radius: height / 2
            color: root.highlightColor
        }
    }
}
