// Port of modules/common/widgets/MaterialTextField.qml
// Material 3 outlined text field with floating label (custom-drawn).
// Drawn entirely with the gallery's tokens instead of the Qt Quick Material
// style, whose unfocused outline color (Material.hintTextColor) is read-only
// and does not follow the palette (white border on dark schemes). The outline
// is a native Rectangle (crisp corners) with a page-colored patch behind the
// floating label that opens a gap in the top edge, like the M3 spec.
// https://m3.material.io/components/text-fields/overview
import QtQuick
import QtQuick.Templates as T
import Components

T.TextField {
    id: root

    implicitWidth: Math.max(200, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(48, contentHeight + topPadding + bottomPadding)

    leftPadding: 16
    rightPadding: 16
    topPadding: 14
    bottomPadding: 14

    color: root.enabled ? Appearance.m3colors.m3onSurface : Appearance.m3colors.m3outlineVariant
    selectionColor: Appearance.colors.colSecondaryContainer
    selectedTextColor: Appearance.m3colors.m3onSecondaryContainer
    placeholderTextColor: root.enabled ? Appearance.m3colors.m3outline : Appearance.m3colors.m3outlineVariant

    font {
        family: Appearance.font.family.main
        pixelSize: Appearance.font.pixelSize.small
        hintingPreference: Font.PreferFullHinting
    }

    wrapMode: TextEdit.Wrap
    selectByMouse: true
    verticalAlignment: Text.AlignVCenter

    // Color the field sits on; the label gap patch uses it to hide the outline.
    property color containerFill: Appearance.m3colors.m3surfaceContainerLow

    // The label floats to the top outline when focused or non-empty.
    readonly property bool floating: root.activeFocus || root.length > 0

    background: Item {
        id: bg

        // Outline (native Rectangle border → clean antialiased corners).
        Rectangle {
            anchors.fill: parent
            radius: 4
            color: "transparent"
            border.width: root.activeFocus ? 2 : 1
            border.color: !root.enabled
                ? Appearance.m3colors.m3outlineVariant
                : root.activeFocus
                    ? Appearance.m3colors.m3primary
                    : root.hovered
                        ? Appearance.m3colors.m3onSurface
                        : Appearance.m3colors.m3outline

            Behavior on border.color {
                ColorAnimation {
                    duration: Appearance.animation.elementMoveFast.duration
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
                }
            }
            Behavior on border.width {
                NumberAnimation {
                    duration: Appearance.animation.elementMoveFast.duration
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
                }
            }
        }

        // Page-colored patch that erases the top outline behind the label.
        // Sized to the label's text (implicitWidth), not its elide width, so
        // the gap hugs the placeholder text instead of spanning the field.
        Rectangle {
            id: gapPatch
            y: 0
            x: root.leftPadding - 4
            height: root.activeFocus ? 2 : 1
            width: root.floating ? Math.min(bg.width - 8, label.implicitWidth + 8) : 0
            color: root.containerFill

            Behavior on width {
                NumberAnimation {
                    duration: Appearance.animation.elementMoveFast.duration
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
                }
            }
        }

        // Floating placeholder/label: sits centered in the field when empty,
        // animates to the top outline (overlapping it) on focus or text.
        Text {
            id: label
            text: root.placeholderText
            font.family: root.font.family
            font.pixelSize: root.floating ? 12 : root.font.pixelSize
            font.weight: root.font.weight
            font.hintingPreference: Font.PreferFullHinting
            color: !root.enabled
                ? Appearance.m3colors.m3outlineVariant
                : root.activeFocus
                    ? Appearance.m3colors.m3primary
                    : Appearance.m3colors.m3outline
            x: root.leftPadding
            y: root.floating
                ? Math.max(1, -label.height / 2 + 1)
                : (bg.height - label.height) / 2
            width: bg.width - root.leftPadding - root.rightPadding
            elide: Text.ElideRight
            renderType: Text.QtRendering

            Behavior on y {
                NumberAnimation {
                    duration: Appearance.animation.elementMoveFast.duration
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
                }
            }
            Behavior on font.pixelSize {
                NumberAnimation {
                    duration: Appearance.animation.elementMoveFast.duration
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
                }
            }
            Behavior on color {
                ColorAnimation {
                    duration: Appearance.animation.elementMoveFast.duration
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        hoverEnabled: true
        cursorShape: Qt.IBeamCursor
    }
}
