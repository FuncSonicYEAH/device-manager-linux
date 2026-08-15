// Port of modules/common/widgets/MaterialSymbol.qml
// Renders a Material Symbols icon using the variable font (FILL/opsz axes).
import QtQuick
import Components

StyledText {
    id: root

    property real iconSize: Appearance.font.pixelSize.small
    property real fill: 0

    renderType: Text.NativeRendering
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    font {
        hintingPreference: Font.PreferNoHinting
        family: Appearance.font.family.iconMaterial
        pixelSize: iconSize
        weight: Font.DemiBold
        variableAxes: ({ "FILL": root.fill, "opsz": iconSize })
    }

    Behavior on fill {
        NumberAnimation {
            duration: Appearance.animation.elementMoveFast.duration
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
        }
    }
}
