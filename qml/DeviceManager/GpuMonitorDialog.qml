// Live GPU monitor dialog. Samples the selected GPU's utilization (%) and
// VRAM usage through the `Monitor` backend once per second and draws the
// rolling history as two smoothed line charts (utilization fixed 0–100%,
// VRAM auto-scaled). Sources: amdgpu/radeon sysfs attributes, NVIDIA
// `nvidia-smi`, Intel `intel_gpu_top`; unsupported GPUs show a notice.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

AnimatedDialog {
    id: root

    property var device: null

    width: Math.min(780, parent.width * 0.9)
    height: Math.min(720, parent.height * 0.94)

    readonly property bool running: Monitor.gpuRunning
    readonly property var history: Monitor.gpuHistory
    readonly property bool supported: root.device !== null && Monitor.supportsGpuMonitoring(root.device)

    readonly property var lastPoint: root.history.length > 0
        ? root.history[root.history.length - 1] : null

    readonly property double usageNow: root.lastPoint !== null && !isNaN(root.lastPoint.usage)
        ? root.lastPoint.usage : -1
    readonly property double vramUsedNow: root.lastPoint !== null && !isNaN(root.lastPoint.vramUsed)
        ? root.lastPoint.vramUsed : -1
    readonly property double vramTotalNow: root.lastPoint !== null && !isNaN(root.lastPoint.vramTotal)
        ? root.lastPoint.vramTotal : -1

    readonly property color usageColor: Appearance.m3colors.m3primary
    readonly property color vramColor: Appearance.m3colors.m3tertiary

    function buildUsageSeries(hist) {
        var values = []
        for (var i = 0; i < hist.length; i++)
            values.push(hist[i].usage)
        return [{ color: root.usageColor, values: values }]
    }
    function buildVramSeries(hist) {
        var values = []
        for (var i = 0; i < hist.length; i++)
            values.push(hist[i].vramUsed)
        return [{ color: root.vramColor, values: values }]
    }

    function formatVram(bytes) {
        if (bytes < 0)
            return "—"
        var mb = bytes / (1024 * 1024)
        if (mb < 1024)
            return mb.toFixed(0) + " MB"
        return (mb / 1024).toFixed(2) + " GB"
    }

    // compact label for the VRAM chart axis (kept short enough to fit the
    // left margin: "8G", "2.5G", "512M")
    function formatVramAxis(bytes) {
        if (bytes < 0)
            return "—"
        var mb = bytes / (1024 * 1024)
        if (mb < 1024)
            return Math.round(mb) + "M"
        var gb = mb / 1024
        if (gb >= 100 || Math.abs(gb - Math.round(gb)) < 0.05)
            return Math.round(gb) + "G"
        return gb.toFixed(1) + "G"
    }

    onOpened: {
        if (root.device !== null)
            Monitor.startGpu(root.device)
    }
    onClosed: Monitor.stopGpu()

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
                    text: "monitor_heart"
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
                    text: Tr.t("gpuMonitorSubtitle", Tr.language)
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
                        font.pixelSize: 12
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

        // ---- current values ---------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 8
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 76
                radius: Appearance.rounding.normal
                color: Appearance.m3colors.m3surfaceContainerHigh
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 2
                    StyledText {
                        Layout.fillWidth: true
                        text: Tr.t("gpuUtilization", Tr.language)
                        font.pixelSize: Appearance.font.pixelSize.smallest
                        color: Appearance.colors.colOnLayer1
                    }
                    StyledText {
                        Layout.fillWidth: true
                        text: root.usageNow >= 0 ? root.usageNow.toFixed(0) + " %" : "—"
                        font.pixelSize: 26
                        font.weight: Font.DemiBold
                        color: root.usageColor
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 76
                radius: Appearance.rounding.normal
                color: Appearance.m3colors.m3surfaceContainerHigh
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 2
                    StyledText {
                        Layout.fillWidth: true
                        text: Tr.t("vramUsage", Tr.language)
                        font.pixelSize: Appearance.font.pixelSize.smallest
                        color: Appearance.colors.colOnLayer1
                    }
                    StyledText {
                        Layout.fillWidth: true
                        text: root.vramUsedNow >= 0
                            ? root.formatVram(root.vramUsedNow) + " / " + root.formatVram(root.vramTotalNow)
                            : "—"
                        font.pixelSize: 26
                        font.weight: Font.DemiBold
                        color: root.vramColor
                    }
                }
            }
        }

        // ---- legend ------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 6
            spacing: 14

            RowLayout {
                spacing: 5
                Rectangle {
                    width: 9
                    height: 9
                    radius: 4.5
                    color: root.usageColor
                }
                StyledText {
                    text: Tr.t("gpuUtilization", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1
                }
            }
            RowLayout {
                visible: root.vramTotalNow >= 0
                spacing: 5
                Rectangle {
                    width: 9
                    height: 9
                    radius: 4.5
                    color: root.vramColor
                }
                StyledText {
                    text: Tr.t("vramUsage", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1
                }
            }

            Item { Layout.fillWidth: true }
            StyledText {
                visible: root.history.length > 0
                text: Tr.t("timeWindow", Tr.language).replace("%1", root.history.length - 1)
                font.pixelSize: Appearance.font.pixelSize.smallest
                color: Appearance.colors.colOnLayer1
            }
        }

        // ---- charts ------------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 8
            spacing: 8

            // no monitorable GPU for this device
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                visible: !root.supported && root.history.length === 0
                spacing: 10
                MaterialSymbol {
                    Layout.alignment: Qt.AlignHCenter
                    text: "monitor_heart"
                    iconSize: 40
                    color: Appearance.colors.colOnLayer1Inactive
                }
                StyledText {
                    Layout.alignment: Qt.AlignHCenter
                    text: Tr.t("noGpuMonitorSource", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallie
                    color: Appearance.colors.colOnLayer1
                }
            }

            // waiting for the first sample
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                visible: root.supported && root.history.length === 0
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
                    running: root.supported && root.history.length === 0
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

            // utilization chart
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.history.length > 0
                spacing: 4
                StyledText {
                    Layout.leftMargin: 4
                    text: Tr.t("gpuUtilization", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    font.weight: Font.DemiBold
                    color: Appearance.colors.colOnLayer1
                }
                LineChart {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    series: root.buildUsageSeries(Monitor.gpuHistory)
                    timeCount: Monitor.gpuHistory.length
                    fixedMin: 0
                    fixedMax: 100
                    unit: "%"
                    formatValue: function(v) { return Math.round(v) }
                }
            }

            // VRAM chart
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.history.length > 0 && root.vramTotalNow >= 0
                spacing: 4
                StyledText {
                    Layout.leftMargin: 4
                    text: Tr.t("vramUsage", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    font.weight: Font.DemiBold
                    color: Appearance.colors.colOnLayer1
                }
                LineChart {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    series: root.buildVramSeries(Monitor.gpuHistory)
                    timeCount: Monitor.gpuHistory.length
                    fixedMax: root.vramTotalNow >= 0 ? root.vramTotalNow : Number.NaN
                    unit: ""
                    formatValue: function(v) { return root.formatVramAxis(v) }
                }
            }
        }

        // ---- process usage ------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 4
            spacing: 4
            visible: root.supported

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                MaterialSymbol {
                    text: "apps"
                    iconSize: 16
                    color: Appearance.colors.colOnLayer1Inactive
                }
                StyledText {
                    text: Tr.t("processUsage", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    font.weight: Font.DemiBold
                    color: Appearance.colors.colOnLayer1
                }
                Item { Layout.fillWidth: true }
                StyledText {
                    visible: Monitor.processesLimited()
                    text: Tr.t("processesLimitedHint", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1Inactive
                }
            }

            // column headers
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                spacing: 12
                StyledText {
                    Layout.fillWidth: true
                    text: Tr.t("processColumn", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1Inactive
                }
                StyledText {
                    Layout.preferredWidth: 130
                    horizontalAlignment: Text.AlignRight
                    text: Tr.t("gpuUtilization", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1Inactive
                }
                StyledText {
                    Layout.preferredWidth: 90
                    horizontalAlignment: Text.AlignRight
                    text: Tr.t("vramUsage", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1Inactive
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 170
                radius: Appearance.rounding.normal
                color: Appearance.m3colors.m3surfaceContainerHigh
                clip: true

                StyledText {
                    anchors.centerIn: parent
                    visible: Monitor.gpuProcesses.length === 0
                    text: Tr.t("gpuProcessNone", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1Inactive
                }

                ListView {
                    id: procList
                    anchors.fill: parent
                    anchors.margins: 6
                    visible: Monitor.gpuProcesses.length > 0
                    clip: true
                    // The process array is replaced every second; binding it
                    // as the model would reset the view (and the scroll
                    // position) on each update. Keep a constant slot count
                    // (kMaxProcRows on the backend) and let the delegates
                    // look their row up by index instead.
                    model: 20
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: Item {
                        id: procRow
                        readonly property bool hasRow: index < Monitor.gpuProcesses.length
                        readonly property var row: hasRow ? Monitor.gpuProcesses[index]
                            : ({ pid: 0, name: "", usage: 0, mem: -1 })
                        width: ListView.view.width
                        height: hasRow ? 30 : 0
                        visible: hasRow

                        RowLayout {
                            anchors.fill: parent
                            anchors.topMargin: 1
                            anchors.bottomMargin: 1
                            spacing: 12

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                StyledText {
                                    Layout.fillWidth: true
                                    text: procRow.row.name
                                    font.pixelSize: Appearance.font.pixelSize.smaller
                                    color: Appearance.colors.colOnLayer1
                                    elide: Text.ElideRight
                                }
                                StyledText {
                                    visible: procRow.row.pid >= 0
                                    text: procRow.row.pid
                                    font.pixelSize: Appearance.font.pixelSize.smallest
                                    color: Appearance.colors.colOnLayer1Inactive
                                }
                            }

                            RowLayout {
                                Layout.preferredWidth: 130
                                spacing: 6
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    Layout.preferredWidth: 64
                                    Layout.preferredHeight: 6
                                    radius: 3
                                    color: Appearance.colors.colOutlineVariant
                                    Rectangle {
                                        width: parent.width * Math.min(1, Math.max(0, procRow.row.usage / 100))
                                        height: parent.height
                                        radius: parent.radius
                                        color: root.usageColor
                                    }
                                }
                                StyledText {
                                    Layout.preferredWidth: 44
                                    horizontalAlignment: Text.AlignRight
                                    text: Math.round(procRow.row.usage) + " %"
                                    font.pixelSize: Appearance.font.pixelSize.smaller
                                    color: Appearance.colors.colOnLayer1
                                }
                            }

                            StyledText {
                                Layout.preferredWidth: 90
                                horizontalAlignment: Text.AlignRight
                                text: procRow.row.mem >= 0 ? root.formatVram(procRow.row.mem) : "—"
                                font.pixelSize: Appearance.font.pixelSize.smaller
                                color: Appearance.colors.colOnLayer1
                            }
                        }
                    }
                }
            }
        }

        // ---- footer ------------------------------------------------------
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
                enabled: root.supported
                onClicked: {
                    if (root.running)
                        Monitor.stopGpu()
                    else if (root.device !== null)
                        Monitor.startGpu(root.device)
                }
            }
            RippleButton {
                buttonText: Tr.t("clear", Tr.language)
                buttonRadius: Appearance.rounding.small
                implicitWidth: 80
                implicitHeight: 34
                enabled: root.history.length > 0
                onClicked: Monitor.clearGpu()
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