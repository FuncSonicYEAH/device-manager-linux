// Port of modules/common/widgets/GroupButton.qml
// Material 3 button with expressive bounciness.
// https://m3.material.io/components/buttons/overview
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

Button {
    id: root

    property bool toggled: false
    property string buttonText
    property real buttonRadius: Appearance.rounding.small
    property real buttonRadiusPressed: Appearance.rounding.verysmall
    property var downAction
    property var releaseAction
    property var altAction
    property var middleClickAction
    property bool bounce: true
    property real baseWidth: contentItem.implicitWidth + horizontalPadding * 2
    property real baseHeight: contentItem.implicitHeight + verticalPadding * 2
    property real clickedWidth: baseWidth + 20
    property real clickedHeight: baseHeight

    implicitWidth: (root.down && bounce) ? clickedWidth : baseWidth
    implicitHeight: (root.down && bounce) ? clickedHeight : baseHeight

    property color colBackground: ColorUtils.transparentize(colBackgroundHover, 1)
    property color colBackgroundHover: Appearance.colors.colLayer1Hover
    property color colBackgroundActive: Appearance.colors.colLayer1Active
    property color colBackgroundToggled: Appearance.colors.colPrimary
    property color colBackgroundToggledHover: Appearance.colors.colPrimaryHover
    property color colBackgroundToggledActive: Appearance.colors.colPrimaryActive

    property real radius: root.down ? root.buttonRadiusPressed : root.buttonRadius
    property color color: root.enabled ? (root.toggled
        ? (root.down ? colBackgroundToggledActive : root.hovered ? colBackgroundToggledHover : colBackgroundToggled)
        : (root.down ? colBackgroundActive : root.hovered ? colBackgroundHover : colBackground))
        : colBackground

    Behavior on implicitWidth {
        enabled: root.bounce
        NumberAnimation {
            duration: Appearance.animation.clickBounce.duration
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Appearance.animation.clickBounce.bezierCurve
        }
    }
    Behavior on implicitHeight {
        enabled: root.bounce
        NumberAnimation {
            duration: Appearance.animation.clickBounce.duration
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Appearance.animation.clickBounce.bezierCurve
        }
    }
    Behavior on radius {
        NumberAnimation {
            duration: Appearance.animation.elementMoveFast.duration
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
        }
    }

    MouseArea {
        id: buttonMouseArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        onPressed: (event) => {
            if (event.button === Qt.RightButton) {
                if (root.altAction) root.altAction();
                return;
            }
            if (event.button === Qt.MiddleButton) {
                if (root.middleClickAction) root.middleClickAction();
                return;
            }
            root.down = true;
            if (root.downAction) root.downAction();
        }
        onReleased: (event) => {
            root.down = false;
            if (event.button !== Qt.LeftButton) return;
            if (root.releaseAction) root.releaseAction();
        }
        onClicked: (event) => {
            if (event.button !== Qt.LeftButton) return;
            root.click();
        }
        onCanceled: {
            root.down = false;
        }
    }

    background: Rectangle {
        id: buttonBackground
        radius: root.radius
        implicitHeight: 50
        color: root.color
        Behavior on color {
            ColorAnimation {
                duration: Appearance.animation.elementMoveFast.duration
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
            }
        }
    }

    contentItem: StyledText {
        text: root.buttonText
        color: root.toggled ? Appearance.colors.colOnPrimary : Appearance.colors.colOnLayer0
    }
}
