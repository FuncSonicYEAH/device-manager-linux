// Shared base for the app dialogs: Material surface styling plus the
// open/close animation — the dialog slides in from `popupOffset` pixels above
// the window center while fading in, and mirrors that when closing.
//
// Positioning is done through x/y bindings (centered on the window); the
// enter/exit transitions animate `y` so the slide cannot fight the anchors.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

Dialog {
    id: root

    // how far above the center the dialog starts (and exits to)
    property real popupOffset: 100

    readonly property real centerY: root.parent !== null
        ? root.parent.height / 2 - root.height / 2 : 0

    modal: true
    dim: true
    padding: 0

    x: root.parent !== null ? root.parent.width / 2 - root.width / 2 : 0
    y: root.centerY

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: 200
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "y"
                from: root.centerY - root.popupOffset
                to: root.centerY
                duration: 280
                easing.type: Easing.OutCubic
            }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: 160
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                property: "y"
                from: root.centerY
                to: root.centerY - root.popupOffset
                duration: 240
                easing.type: Easing.InCubic
            }
        }
    }

    Overlay.modal: Rectangle {
        color: Appearance.colors.colScrim
    }

    background: Rectangle {
        radius: Appearance.rounding.windowRounding
        color: Appearance.m3colors.m3surfaceContainerLow
        border.color: Appearance.colors.colOutlineVariant
        border.width: 1
    }
}
