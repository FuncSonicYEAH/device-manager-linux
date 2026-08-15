// Port of modules/common/widgets/StyledSlider.qml
// Material 3 slider. https://m3.material.io/components/sliders/overview
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

Slider {
    id: root

    property real trackWidth: 6
    property color highlightColor: Appearance.colors.colPrimary
    property color trackColor: Appearance.colors.colSecondaryContainer
    property color handleColor: Appearance.colors.colPrimary
    property color dotColor: Appearance.m3colors.m3onSecondaryContainer
    property color dotColorHighlighted: Appearance.m3colors.m3onPrimary
    property real unsharpenRadius: Appearance.rounding.unsharpen
    property real trackRadius: trackWidth >= 72 ? 21 : trackWidth >= 42 ? 12 : trackWidth >= 30 ? 9 : trackWidth >= 18 ? 6 : 4
    property real handleHeight: Math.max(33, trackWidth + 9)
    property real handleWidth: root.pressed ? 1.5 : 3
    property real handleMargins: 4
    property real trackDotSize: 3
    property bool usePercentTooltip: true
    property string tooltipContent: usePercentTooltip ? `${Math.round(((value - from) / (to - from)) * 100)}%` : `${Math.round(value)}`

    leftPadding: handleMargins
    rightPadding: handleMargins
    property real effectiveDraggingWidth: width - leftPadding - rightPadding
    from: 0
    to: 1

    Behavior on value {
        SmoothedAnimation {
            velocity: Appearance.animation.elementMoveFast.velocity
        }
    }

    component TrackDot: Rectangle {
        required property real value
        property real normalizedValue: (value - root.from) / (root.to - root.from)
        anchors.verticalCenter: parent.verticalCenter
        x: root.handleMargins + (normalizedValue * root.effectiveDraggingWidth) - (root.trackDotSize / 2)
        width: root.trackDotSize
        height: root.trackDotSize
        radius: Appearance.rounding.full
        color: normalizedValue > root.visualPosition ? root.dotColor : root.dotColorHighlighted

        Behavior on color {
            ColorAnimation {
                duration: Appearance.animation.elementMoveFast.duration
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
            }
        }
    }

    background: Item {
        id: background
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        width: root.width
        implicitHeight: root.trackWidth

        Rectangle {
            id: trackLeft
            anchors {
                left: parent.left
                leftMargin: root.leftPadding
                verticalCenter: parent.verticalCenter
            }
            width: root.visualPosition * root.effectiveDraggingWidth - root.handleWidth / 2
            height: root.trackWidth
            color: root.highlightColor
            topLeftRadius: root.trackRadius
            bottomLeftRadius: root.trackRadius
        }

        Rectangle {
            id: trackRight
            anchors {
                right: parent.right
                rightMargin: root.rightPadding
                verticalCenter: parent.verticalCenter
            }
            width: (1 - root.visualPosition) * root.effectiveDraggingWidth - root.handleWidth / 2
            height: root.trackWidth
            color: root.trackColor
            topRightRadius: root.trackRadius
            bottomRightRadius: root.trackRadius
        }
    }

    handle: Rectangle {
        id: handle

        implicitWidth: root.handleWidth
        implicitHeight: root.handleHeight
        x: root.leftPadding + (root.visualPosition * root.effectiveDraggingWidth) - (root.handleWidth / 2)
        anchors.verticalCenter: parent.verticalCenter
        radius: Appearance.rounding.full
        color: root.handleColor

        Behavior on implicitWidth {
            NumberAnimation {
                duration: Appearance.animation.elementMoveFast.duration
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
            }
        }

        StyledToolTip {
            extraVisibleCondition: root.pressed
            text: root.tooltipContent
            font.pixelSize: Appearance.font.pixelSize.smaller
        }
    }
}
