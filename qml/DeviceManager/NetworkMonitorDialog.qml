// Live network traffic dialog. Samples the cumulative byte counters of one
// interface from /proc/net/dev once per second, computes RX/TX rates and draws
// the rolling history as a smoothed line chart. The interface is taken from the
// selected device (its sysfs entry name), with a ComboBox to switch to any
// other interface present on the system.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

AnimatedDialog {
    id: root

    property var device: null
    property string iface: ""

    width: Math.min(780, parent.width * 0.9)
    height: Math.min(680, parent.height * 0.9)

    readonly property bool running: Monitor.netRunning
    readonly property var history: Monitor.netHistory
    readonly property var ifaceList: Monitor.netInterfaces

    readonly property var lastPoint: root.history.length > 0
        ? root.history[root.history.length - 1] : null
    readonly property double rxNow: root.lastPoint !== null ? root.lastPoint.rx : -1
    readonly property double txNow: root.lastPoint !== null ? root.lastPoint.tx : -1

    readonly property color rxColor: Appearance.m3colors.m3primary
    readonly property color txColor: Appearance.m3colors.m3tertiary

    function indexOfIface(name) {
        for (var i = 0; i < root.ifaceList.length; i++)
            if (root.ifaceList[i].name === name)
                return i
        return -1
    }

    function inList(name) {
        return root.indexOfIface(name) >= 0
    }

    function pickDefaultIface() {
        var target = root.device !== null ? root.device.entryName : ""
        if (root.iface !== "" && root.inList(root.iface))
            return
        if (target !== "" && target !== "lo" && root.inList(target)) {
            root.iface = target
            return
        }
        for (var i = 0; i < root.ifaceList.length; i++) {
            if (root.ifaceList[i].name !== "lo") {
                root.iface = root.ifaceList[i].name
                return
            }
        }
        root.iface = root.ifaceList.length > 0 ? root.ifaceList[0].name : ""
    }

    function selectIface(name) {
        if (name === root.iface)
            return
        root.iface = name
        Monitor.startNet(name)
    }

    function buildRxSeries(hist) {
        var values = []
        for (var i = 0; i < hist.length; i++)
            values.push(hist[i].rx)
        return [{ color: root.rxColor, values: values }]
    }
    function buildTxSeries(hist) {
        var values = []
        for (var i = 0; i < hist.length; i++)
            values.push(hist[i].tx)
        return [{ color: root.txColor, values: values }]
    }

    function formatRate(bytesPerSec) {
        if (bytesPerSec < 0)
            return "—"
        if (bytesPerSec < 1024)
            return bytesPerSec.toFixed(0) + " B/s"
        if (bytesPerSec < 1024 * 1024)
            return (bytesPerSec / 1024).toFixed(1) + " KB/s"
        if (bytesPerSec < 1024 * 1024 * 1024)
            return (bytesPerSec / (1024 * 1024)).toFixed(2) + " MB/s"
        return (bytesPerSec / (1024 * 1024 * 1024)).toFixed(2) + " GB/s"
    }
    function formatTotal(bytes) {
        if (bytes < 1024)
            return bytes.toFixed(0) + " B"
        if (bytes < 1024 * 1024)
            return (bytes / 1024).toFixed(1) + " KB"
        if (bytes < 1024 * 1024 * 1024)
            return (bytes / (1024 * 1024)).toFixed(1) + " MB"
        return (bytes / (1024 * 1024 * 1024)).toFixed(2) + " GB"
    }
    function formatAxisRate(v) {
        if (v < 1024)
            return v.toFixed(0)
        if (v < 1024 * 1024)
            return (v / 1024).toFixed(0) + "K"
        return (v / (1024 * 1024)).toFixed(1) + "M"
    }

    onOpened: {
        root.pickDefaultIface()
        if (root.iface !== "")
            Monitor.startNet(root.iface)
    }
    onClosed: Monitor.stopNet()

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
                    text: "swap_vert"
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
                    text: Tr.t("networkMonitorSubtitle", Tr.language)
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

        // ---- interface picker + current values --------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 8
            spacing: 12

            ComboBox {
                id: ifaceBox
                Layout.preferredWidth: 150
                visible: root.ifaceList.length > 1
                model: root.ifaceList
                textRole: "name"
                currentIndex: root.indexOfIface(root.iface)
                onActivated: (index) => root.selectIface(root.ifaceList[index].name)
                background: Rectangle {
                    radius: Appearance.rounding.small
                    color: Appearance.m3colors.m3surfaceContainerHigh
                    border.color: Appearance.colors.colOutlineVariant
                    border.width: 1
                }
                contentItem: StyledText {
                    text: ifaceBox.currentText
                    font.pixelSize: Appearance.font.pixelSize.smaller
                    color: Appearance.colors.colOnLayer1
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 10
                }
                indicator: MaterialSymbol {
                    x: ifaceBox.width - width - 8
                    y: ifaceBox.height / 2 - height / 2
                    text: "arrow_drop_down"
                    iconSize: 18
                    color: Appearance.colors.colOnLayer1Inactive
                }
                popup: Popup {
                    y: ifaceBox.height + 4
                    width: ifaceBox.width
                    implicitHeight: contentHeight + 8
                    padding: 4
                    background: Rectangle {
                        radius: Appearance.rounding.small
                        color: Appearance.m3colors.m3surfaceContainerHigh
                        border.color: Appearance.colors.colOutlineVariant
                        border.width: 1
                    }
                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: ifaceBox.popup.visible ? ifaceBox.delegateModel : null
                        currentIndex: ifaceBox.highlightedIndex
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    }
                }
                delegate: ItemDelegate {
                    width: ifaceBox.width
                    contentItem: StyledText {
                        text: modelData.name
                        font.pixelSize: Appearance.font.pixelSize.smaller
                        color: Appearance.colors.colOnLayer1
                    }
                    highlighted: ifaceBox.highlightedIndex === index
                    background: Rectangle {
                        color: ifaceBox.highlightedIndex === index
                            ? Appearance.colors.colSecondaryContainer : "transparent"
                        radius: 6
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
                        text: Tr.t("rxRate", Tr.language)
                        font.pixelSize: Appearance.font.pixelSize.smallest
                        color: Appearance.colors.colOnLayer1
                    }
                    StyledText {
                        Layout.fillWidth: true
                        text: root.formatRate(root.rxNow)
                        font.pixelSize: 26
                        font.weight: Font.DemiBold
                        color: root.rxColor
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
                        text: Tr.t("txRate", Tr.language)
                        font.pixelSize: Appearance.font.pixelSize.smallest
                        color: Appearance.colors.colOnLayer1
                    }
                    StyledText {
                        Layout.fillWidth: true
                        text: root.formatRate(root.txNow)
                        font.pixelSize: 26
                        font.weight: Font.DemiBold
                        color: root.txColor
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
                    color: root.rxColor
                }
                StyledText {
                    text: Tr.t("rxRate", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1
                }
            }
            RowLayout {
                spacing: 5
                Rectangle {
                    width: 9
                    height: 9
                    radius: 4.5
                    color: root.txColor
                }
                StyledText {
                    text: Tr.t("txRate", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1
                }
            }

            Item { Layout.fillWidth: true }

            // cumulative totals
            StyledText {
                text: (root.lastPoint !== null)
                    ? Tr.t("rxTotalFormat", Tr.language).replace("%1", root.formatTotal(root.lastPoint.rxTotal))
                      + "  ·  "
                      + Tr.t("txTotalFormat", Tr.language).replace("%1", root.formatTotal(root.lastPoint.txTotal))
                    : ""
                font.pixelSize: Appearance.font.pixelSize.smallest
                color: Appearance.colors.colOnLayer1
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

            // no readable network interface
            ColumnLayout {
                anchors.centerIn: parent
                visible: root.ifaceList.length === 0
                spacing: 10
                MaterialSymbol {
                    Layout.alignment: Qt.AlignHCenter
                    text: "wifi_off"
                    iconSize: 40
                    color: Appearance.colors.colOnLayer1Inactive
                }
                StyledText {
                    Layout.alignment: Qt.AlignHCenter
                    text: Tr.t("noNetworkInterface", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallie
                    color: Appearance.colors.colOnLayer1
                }
            }

            // waiting for the first sample
            ColumnLayout {
                anchors.centerIn: parent
                visible: root.ifaceList.length > 0 && root.history.length === 0
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
                    running: root.ifaceList.length > 0 && root.history.length === 0
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

            LineChart {
                anchors.fill: parent
                visible: root.history.length > 0
                series: root.buildRxSeries(Monitor.netHistory).concat(root.buildTxSeries(Monitor.netHistory))
                timeCount: Monitor.netHistory.length
                unit: "B/s"
                formatValue: function(v) { return root.formatAxisRate(v) }
            }
        }

        // ---- process usage ------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 4
            spacing: 4
            visible: root.ifaceList.length > 0

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
                    Layout.preferredWidth: 110
                    horizontalAlignment: Text.AlignRight
                    text: Tr.t("rxRate", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1Inactive
                }
                StyledText {
                    Layout.preferredWidth: 110
                    horizontalAlignment: Text.AlignRight
                    text: Tr.t("txRate", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1Inactive
                }
                StyledText {
                    Layout.preferredWidth: 60
                    horizontalAlignment: Text.AlignRight
                    text: Tr.t("connections", Tr.language)
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
                    visible: Monitor.netProcesses.length === 0
                    text: Tr.t("netProcessNone", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1Inactive
                }

                ListView {
                    id: procList
                    anchors.fill: parent
                    anchors.margins: 6
                    visible: Monitor.netProcesses.length > 0
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
                        readonly property bool hasRow: index < Monitor.netProcesses.length
                        readonly property var row: hasRow ? Monitor.netProcesses[index]
                            : ({ pid: 0, name: "", rx: -1, tx: -1, sockets: 0 })
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

                            StyledText {
                                Layout.preferredWidth: 110
                                horizontalAlignment: Text.AlignRight
                                text: root.formatRate(procRow.row.rx)
                                font.pixelSize: Appearance.font.pixelSize.smaller
                                color: root.rxColor
                            }
                            StyledText {
                                Layout.preferredWidth: 110
                                horizontalAlignment: Text.AlignRight
                                text: root.formatRate(procRow.row.tx)
                                font.pixelSize: Appearance.font.pixelSize.smaller
                                color: root.txColor
                            }
                            StyledText {
                                Layout.preferredWidth: 60
                                horizontalAlignment: Text.AlignRight
                                text: procRow.row.sockets
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
                enabled: root.iface !== "" && root.ifaceList.length > 0
                onClicked: {
                    if (root.running)
                        Monitor.stopNet()
                    else if (root.iface !== "")
                        Monitor.startNet(root.iface)
                }
            }
            RippleButton {
                buttonText: Tr.t("clear", Tr.language)
                buttonRadius: Appearance.rounding.small
                enabled: root.history.length > 0
                onClicked: Monitor.clearNet()
            }
            Item { Layout.fillWidth: true }
            RippleButton {
                buttonText: Tr.t("close", Tr.language)
                buttonRadius: Appearance.rounding.small
                onClicked: root.close()
            }
        }
    }
}