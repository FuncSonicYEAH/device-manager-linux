// Material 3 range slider (simple port adaptation)
import QtQuick
import QtQuick.Controls
import Components

RangeSlider {
    id: root

    property real trackWidth: 6
    property color highlightColor: Appearance.colors.colPrimary
    property color trackColor: Appearance.colors.colSecondaryContainer
    property color handleColor: Appearance.colors.colPrimary
    property real handleHeight: Math.max(33, trackWidth + 9)
    property real handleWidth: root.pressed ? 1.5 : 3
    property real handleMargins: 4

    leftPadding: handleMargins
    rightPadding: handleMargins
    property real effectiveDraggingWidth: width - leftPadding - rightPadding
    from: 0
    to: 1

    background: Item {
        id: background
        anchors.verticalCenter: parent.verticalCenter
        width: root.width
        implicitHeight: root.trackWidth

        Rectangle {
            id: trackFill
            anchors.verticalCenter: parent.verticalCenter
            x: root.leftPadding + (root.first.visualPosition * root.effectiveDraggingWidth)
            width: (root.second.visualPosition - root.first.visualPosition) * root.effectiveDraggingWidth
            height: root.trackWidth
            color: root.highlightColor
            radius: root.trackWidth / 2
        }

        Rectangle {
            id: trackBackground
            anchors.verticalCenter: parent.verticalCenter
            x: root.leftPadding
            width: root.effectiveDraggingWidth
            height: root.trackWidth
            color: root.trackColor
            radius: root.trackWidth / 2
        }
    }

    first.handle: Rectangle {
        implicitWidth: root.handleWidth
        implicitHeight: root.handleHeight
        x: root.leftPadding + (root.first.visualPosition * root.effectiveDraggingWidth) - (root.handleWidth / 2)
        anchors.verticalCenter: parent.verticalCenter
        radius: Appearance.rounding.full
        color: root.handleColor
        z: 2
    }

    second.handle: Rectangle {
        implicitWidth: root.handleWidth
        implicitHeight: root.handleHeight
        x: root.leftPadding + (root.second.visualPosition * root.effectiveDraggingWidth) - (root.handleWidth / 2)
        anchors.verticalCenter: parent.verticalCenter
        radius: Appearance.rounding.full
        color: root.handleColor
        z: 2
    }
}
