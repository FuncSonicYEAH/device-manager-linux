// Live temperature curve dialog. Samples the temperature sensors of a device
// through the `Temperature` backend (sysfs hwmon attributes / thermal zones)
// once per second and draws the rolling history as a smoothed line chart.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

AnimatedDialog {
    id: root

    property var device: null

    width: Math.min(780, parent.width * 0.9)
    height: Math.min(580, parent.height * 0.88)

    readonly property bool running: Temperature.running
    readonly property var sensors: Temperature.sensors
    readonly property var history: Temperature.history

    // palette used for the sensor curves
    readonly property var lineColors: [
        Appearance.m3colors.m3primary,
        Appearance.m3colors.m3tertiary,
        Appearance.m3colors.m3error,
        Appearance.m3colors.m3success,
        "#7E57C2",
        "#F4511E",
        "#00897B",
        "#C2185B"
    ]

    function lineColor(index) {
        return root.lineColors[index % root.lineColors.length]
    }

    // pick a "nice" gridline step (1/2/5 × 10^n) covering the range
    function niceStep(range) {
        if (range <= 0)
            return 1
        var pow = Math.pow(10, Math.floor(Math.log(range) / Math.LN10))
        var frac = range / pow
        return (frac >= 5 ? 10 : frac >= 2 ? 5 : 2) * pow
    }

    function formatTemp(v) {
        if (v !== null && v !== undefined && !isNaN(v))
            return v.toFixed(1) + " °C"
        return "—"
    }

    onOpened: {
        if (root.device !== null)
            Temperature.start(root.device)
    }
    onClosed: Temperature.stop()

    contentItem: ColumnLayout {
        spacing: 0

        // ---- header -----------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 18
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
                radius: Appearance.rounding.normal
                color: Appearance.colors.colSecondaryContainer
                MaterialSymbol {
                    anchors.centerIn: parent
                    text: root.device !== null && root.device.icon !== "" ? root.device.icon : "thermostat"
                    iconSize: 24
                    color: Appearance.colors.colOnSecondaryContainer
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                StyledText {
                    Layout.fillWidth: true
                    text: root.device !== null ? root.device.name : ""
                    font.pixelSize: Appearance.font.pixelSize.normal
                    font.weight: Font.Medium
                    color: Appearance.colors.colOnLayer1
                    elide: Text.ElideRight
                }
                StyledText {
                    Layout.fillWidth: true
                    text: Tr.t("temperatureCurveSubtitle", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1
                }
            }

            // live indicator
            Rectangle {
                visible: root.running
                implicitWidth: liveRow.implicitWidth + 14
                implicitHeight: 22
                radius: 11
                color: Appearance.m3colors.m3errorContainer
                RowLayout {
                    id: liveRow
                    anchors.centerIn: parent
                    spacing: 4
                    MaterialSymbol {
                        text: "fiber_manual_record"
                        iconSize: 12
                        color: Appearance.m3colors.m3error
                    }
                    StyledText {
                        text: Tr.t("recording", Tr.language)
                        font.pixelSize: 11
                        color: Appearance.m3colors.m3onErrorContainer
                    }
                }
            }

            RippleButton {
                buttonRadius: Appearance.rounding.full
                implicitWidth: 30
                implicitHeight: 30
                onClicked: root.close()
                contentItem: MaterialSymbol {
                    anchors.centerIn: parent
                    text: "close"
                    iconSize: 18
                    color: Appearance.colors.colOnLayer1
                }
            }
        }

        // ---- sensor legend ----------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 8
            spacing: 8

            Flow {
                Layout.fillWidth: true
                spacing: 14

                Repeater {
                    model: root.sensors
                    delegate: RowLayout {
                        required property int index
                        required property var modelData

                        spacing: 5
                        Rectangle {
                            width: 9
                            height: 9
                            radius: 4.5
                            color: root.lineColor(index)
                        }
                        StyledText {
                            text: modelData.name
                            font.pixelSize: Appearance.font.pixelSize.smallest
                            color: Appearance.colors.colOnLayer1
                        }
                        StyledText {
                            text: root.formatTemp(modelData.current)
                            font.pixelSize: Appearance.font.pixelSize.smallest
                            font.weight: Font.DemiBold
                            color: Appearance.colors.colOnLayer1
                        }
                    }
                }
            }

            StyledText {
                visible: root.history.length > 0
                text: Tr.t("timeWindow", Tr.language).replace("%1", root.history.length - 1)
                font.pixelSize: Appearance.font.pixelSize.smallest
                color: Appearance.colors.colOnLayer1
            }
        }

        // ---- chart ------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 12
            clip: true

            // no temperature source for this device
            ColumnLayout {
                anchors.centerIn: parent
                visible: root.sensors.length === 0
                spacing: 10
                MaterialSymbol {
                    Layout.alignment: Qt.AlignHCenter
                    text: "thermostat"
                    iconSize: 40
                    color: Appearance.colors.colOnLayer1Inactive
                }
                StyledText {
                    Layout.alignment: Qt.AlignHCenter
                    text: Tr.t("noTemperatureSource", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallie
                    color: Appearance.colors.colOnLayer1
                }
            }

            // waiting for the first sample
            ColumnLayout {
                anchors.centerIn: parent
                visible: root.sensors.length > 0 && root.history.length === 0
                spacing: 12
                CircularProgress {
                    id: spinner
                    Layout.alignment: Qt.AlignHCenter
                    implicitSize: 30
                    value: spinnerAnim.value
                }
                SequentialAnimation {
                    id: spinnerAnim
                    property real value: 0.05
                    running: root.sensors.length > 0 && root.history.length === 0
                    loops: Animation.Infinite
                    NumberAnimation {
                        target: spinnerAnim
                        property: "value"
                        from: 0.05
                        to: 0.95
                        duration: 800
                        easing.type: Easing.InOutQuad
                    }
                    NumberAnimation {
                        target: spinnerAnim
                        property: "value"
                        from: 0.95
                        to: 0.05
                        duration: 800
                        easing.type: Easing.InOutQuad
                    }
                }
                StyledText {
                    Layout.alignment: Qt.AlignHCenter
                    text: Tr.t("waitingData", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallie
                    color: Appearance.colors.colOnLayer1
                }
            }

            Canvas {
                id: chart
                anchors.fill: parent
                visible: root.history.length > 0

                property var history: root.history
                property var sensors: root.sensors

                onHistoryChanged: requestPaint()
                onSensorsChanged: requestPaint()
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.setTransform(1, 0, 0, 1, 0, 0)

                    var ml = 38, mr = 10, mt = 12, mb = 26
                    var pw = width - ml - mr
                    var ph = height - mt - mb
                    if (pw <= 0 || ph <= 0)
                        return

                    var hist = root.history
                    var n = hist.length
                    var nSensors = root.sensors.length

                    // ---- value range from the data ----------------------
                    var vmin = Infinity, vmax = -Infinity
                    for (var i = 0; i < n; i++) {
                        var vals = hist[i].values
                        for (var j = 0; j < vals.length; j++) {
                            var v = vals[j]
                            if (isNaN(v))
                                continue
                            if (v < vmin) vmin = v
                            if (v > vmax) vmax = v
                        }
                    }
                    if (!isFinite(vmin)) {
                        vmin = 0
                        vmax = 80
                    }
                    var pad = Math.max(2, (vmax - vmin) * 0.15)
                    vmin = Math.floor(vmin - pad)
                    vmax = Math.ceil(vmax + pad)
                    if (vmax - vmin < 4)
                        vmax = vmin + 4

                    var yStep = root.niceStep((vmax - vmin) / 4)
                    var y0 = Math.floor(vmin / yStep) * yStep

                    function xFor(i) { return ml + (n <= 1 ? pw : (i / (n - 1)) * pw) }
                    function yFor(v) { return mt + ph - ((v - vmin) / (vmax - vmin)) * ph }

                    // ---- horizontal grid + y labels ---------------------
                    ctx.font = "10px sans-serif"
                    ctx.textAlign = "right"
                    ctx.textBaseline = "middle"
                    for (var gy = y0; gy <= vmax; gy += yStep) {
                        var yy = yFor(gy)
                        ctx.strokeStyle = Appearance.colors.colOnLayer1
                        ctx.globalAlpha = 0.18
                        ctx.lineWidth = 1
                        ctx.beginPath()
                        ctx.moveTo(ml, yy)
                        ctx.lineTo(ml + pw, yy)
                        ctx.stroke()
                        ctx.globalAlpha = 1
                        ctx.fillStyle = Appearance.colors.colOnLayer1
                        ctx.fillText(gy, ml - 6, yy)
                    }
                    ctx.textAlign = "left"
                    ctx.fillText("°C", ml + 4, mt + 4)

                    // ---- vertical grid + x labels (time) ----------------
                    if (n > 1) {
                        var tmin = hist[0].time
                        var tStep = root.niceStep((0 - tmin) / 5)
                        var t0 = Math.floor(tmin / tStep) * tStep
                        ctx.textAlign = "center"
                        for (var tx = t0; tx <= 0; tx += tStep) {
                            var xi = ml + ((tx - tmin) / (0 - tmin)) * pw
                            ctx.strokeStyle = Appearance.colors.colOnLayer1
                            ctx.globalAlpha = 0.1
                            ctx.beginPath()
                            ctx.moveTo(xi, mt)
                            ctx.lineTo(xi, mt + ph)
                            ctx.stroke()
                            ctx.globalAlpha = 1
                            ctx.fillStyle = Appearance.colors.colOnLayer1
                            ctx.fillText(tx + "s", xi, mt + ph + 14)
                        }
                    }

                    // ---- sensor curves ----------------------------------
                    for (var s = 0; s < nSensors; s++) {
                        var pts = []
                        for (i = 0; i < n; i++) {
                            var value = hist[i].values[s]
                            if (value === undefined || isNaN(value))
                                continue
                            pts.push({ x: xFor(i), y: yFor(value) })
                        }
                        if (pts.length < 2)
                            continue

                        ctx.strokeStyle = root.lineColor(s)
                        ctx.lineWidth = 2
                        ctx.lineJoin = "round"
                        ctx.lineCap = "round"
                        ctx.beginPath()
                        ctx.moveTo(pts[0].x, pts[0].y)
                        // smooth polyline: quadratic curves through midpoints
                        for (var k = 1; k < pts.length - 1; k++) {
                            var xc = (pts[k].x + pts[k + 1].x) / 2
                            var yc = (pts[k].y + pts[k + 1].y) / 2
                            ctx.quadraticCurveTo(pts[k].x, pts[k].y, xc, yc)
                        }
                        ctx.lineTo(pts[pts.length - 1].x, pts[pts.length - 1].y)
                        ctx.stroke()

                        // dot on the latest point
                        var last = pts[pts.length - 1]
                        ctx.fillStyle = root.lineColor(s)
                        ctx.beginPath()
                        ctx.arc(last.x, last.y, 3.5, 0, Math.PI * 2)
                        ctx.fill()
                    }
                }
            }
        }

        // ---- footer -----------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            spacing: 8

            RippleButton {
                buttonText: root.running
                    ? Tr.t("stopMonitoring", Tr.language)
                    : Tr.t("startMonitoring", Tr.language)
                buttonRadius: Appearance.rounding.small
                implicitWidth: 110
                implicitHeight: 34
                enabled: root.sensors.length > 0
                onClicked: {
                    if (root.running)
                        Temperature.stop()
                    else if (root.device !== null)
                        Temperature.start(root.device)
                }
            }
            RippleButton {
                buttonText: Tr.t("clear", Tr.language)
                buttonRadius: Appearance.rounding.small
                implicitWidth: 80
                implicitHeight: 34
                enabled: root.history.length > 0
                onClicked: Temperature.clear()
            }
            Item { Layout.fillWidth: true }
            RippleButton {
                buttonText: Tr.t("close", Tr.language)
                buttonRadius: Appearance.rounding.small
                implicitWidth: 80
                implicitHeight: 34
                onClicked: root.close()
            }
        }
    }
}
