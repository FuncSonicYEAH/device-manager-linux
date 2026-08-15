// Port of modules/common/widgets/CircularProgress.qml
// Material 3 circular progress. https://m3.material.io/components/progress-indicators/specs
import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property int implicitSize: 30
    property int lineWidth: 2
    property real from: 0
    property real to: 1
    property real value: 0
    property int diameter: implicitSize
    property int thickness: lineWidth
    property color trackColor: Appearance.colors.colSecondaryContainer
    property color progressColor: Appearance.m3colors.m3onSecondaryContainer
    property real gapAngle: 360 / 18
    property bool fill: false
    property bool enableAnimation: true
    property int animationDuration: 800
    property var easingType: Easing.OutCubic

    implicitWidth: diameter
    implicitHeight: diameter

    property real degree: ((value - from) / (to - from)) * 360
    property real centerX: root.width / 2
    property real centerY: root.height / 2
    property real arcRadius: root.diameter / 2 - root.thickness
    property real startAngle: -90

    Behavior on degree {
        enabled: root.enableAnimation
        NumberAnimation {
            duration: root.animationDuration
            easing.type: root.easingType
        }
    }

    Loader {
        active: root.fill
        anchors.fill: parent
        sourceComponent: Rectangle {
            radius: 9999
            color: root.trackColor
        }
    }

    Shape {
        anchors.fill: parent
        layer.enabled: true
        layer.smooth: true
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            id: secondaryPath
            strokeColor: root.trackColor
            strokeWidth: root.thickness
            capStyle: ShapePath.RoundCap
            fillColor: "transparent"
            PathAngleArc {
                centerX: root.centerX
                centerY: root.centerY
                radiusX: root.arcRadius
                radiusY: root.arcRadius
                startAngle: root.startAngle - root.gapAngle
                sweepAngle: -(360 - root.degree - 2 * root.gapAngle)
            }
        }
        ShapePath {
            id: primaryPath
            strokeColor: root.progressColor
            strokeWidth: root.thickness
            capStyle: ShapePath.RoundCap
            fillColor: "transparent"
            PathAngleArc {
                centerX: root.centerX
                centerY: root.centerY
                radiusX: root.arcRadius
                radiusY: root.arcRadius
                startAngle: root.startAngle
                sweepAngle: root.degree
            }
        }
    }
}
