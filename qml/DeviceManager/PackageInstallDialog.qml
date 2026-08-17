// Sub-dialog showing the live progress of a driver package installation:
// the package manager (dnf/apt/pacman/...) runs through pkexec and its output
// is streamed here line by line. The dialog cannot be closed while the
// transaction is running — closing it would not stop the install anyway.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

AnimatedDialog {
    id: root

    property string pkgName: ""
    property string deviceId: ""
    property string deviceName: ""
    property string backendName: ""
    // proprietary installs run per-driver multi-step commands (RPM Fusion /
    // ubuntu-drivers / yay) instead of a plain package install
    property bool proprietary: false

    property bool running: true
    property string phase: "prepare"   // prepare | download | install | done
    property int percent: -1           // -1 = unknown, busy indicator instead
    property var logLines: []
    property bool installOk: false
    property string message: ""

    signal installCompleted(bool ok, string deviceId)

    width: Math.min(660, parent.width * 0.9)
    height: Math.min(500, parent.height * 0.9)
    closePolicy: Popup.NoAutoClose

    readonly property string phaseText: root.phase === "download"
        ? Tr.t("installPhaseDownload", Tr.language)
        : root.phase === "install" ? Tr.t("installPhaseInstall", Tr.language)
        : root.phase === "done" ? Tr.t("installPhaseDone", Tr.language)
        : Tr.t("installPhasePrepare", Tr.language)

    readonly property string displayMessage: root.message === "yay not found"
        ? Tr.t("yayMissing", Tr.language) : root.message

    function start() {
        root.running = true
        root.phase = "prepare"
        root.percent = -1
        root.logLines = []
        root.installOk = false
        root.message = ""
        if (root.proprietary)
            Drivers.installProprietary(root.deviceId)
        else
            Drivers.installPackage(root.deviceId, root.pkgName)
    }

    function appendLine(line) {
        var lines = root.logLines
        lines.push(line)
        if (lines.length > 400)
            lines = lines.slice(lines.length - 400)
        root.logLines = lines
        root.scrollLogToEnd()
    }

    function scrollLogToEnd() {
        Qt.callLater(function() {
            logFlick.contentY = Math.max(0, logFlick.contentHeight - logFlick.height)
        })
    }

    onOpened: root.start()

    Connections {
        target: Drivers
        function onInstallOutput(deviceId, pkgName, line) {
            if (deviceId === root.deviceId && pkgName === root.pkgName)
                root.appendLine(line)
        }
        function onInstallProgress(deviceId, pkgName, percent) {
            if (deviceId === root.deviceId && pkgName === root.pkgName)
                root.percent = percent
        }
        function onInstallPhase(deviceId, pkgName, phase) {
            if (deviceId === root.deviceId && pkgName === root.pkgName)
                root.phase = phase
        }
        function onInstallFinished(deviceId, pkgName, ok, message) {
            if (deviceId === root.deviceId && pkgName === root.pkgName) {
                root.running = false
                root.installOk = ok
                root.message = message
                root.installCompleted(ok, deviceId)
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        // ---- header -------------------------------------------------------
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
                    text: "system_update_alt"
                    iconSize: 24
                    color: Appearance.colors.colOnSecondaryContainer
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                StyledText {
                    Layout.fillWidth: true
                    text: root.pkgName
                    font.pixelSize: Appearance.font.pixelSize.normal
                    font.weight: Font.Medium
                    color: Appearance.colors.colOnLayer1
                    elide: Text.ElideMiddle
                }
                StyledText {
                    Layout.fillWidth: true
                    text: (root.deviceName !== "" ? root.deviceName + "  ·  " : "")
                        + (root.backendName !== "" ? root.backendName : "")
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1
                    elide: Text.ElideRight
                }
            }

            MaterialSymbol {
                visible: !root.running
                text: root.installOk ? "check_circle" : "error"
                iconSize: 24
                color: root.installOk ? Appearance.m3colors.m3success
                                      : Appearance.m3colors.m3error
            }

            RippleButton {
                buttonRadius: Appearance.rounding.full
                implicitWidth: 30
                implicitHeight: 30
                enabled: !root.running
                onClicked: root.close()
                contentItem: MaterialSymbol {
                    anchors.centerIn: parent
                    text: "close"
                    iconSize: 18
                    color: root.running ? Appearance.colors.colOnLayer1Inactive
                                        : Appearance.colors.colOnLayer1
                }
            }
        }

        // ---- progress ------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 10
            spacing: 10

            StyledText {
                text: root.phaseText
                font.pixelSize: Appearance.font.pixelSize.smallie
                color: Appearance.colors.colOnLayer1
            }
            Item { Layout.fillWidth: true }
            StyledText {
                visible: root.percent >= 0 && root.running
                text: root.percent + "%"
                font.pixelSize: Appearance.font.pixelSize.smallie
                color: Appearance.colors.colOnLayer1
            }
        }

        StyledProgressBar {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            visible: root.percent >= 0
            value: root.percent / 100
            valueBarHeight: 5
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            visible: root.percent < 0 && root.running
            spacing: 10

            CircularProgress {
                Layout.alignment: Qt.AlignVCenter
                implicitSize: 18
                value: busyAnim.value
            }
            StyledText {
                Layout.fillWidth: true
                text: Tr.t("installRequiresRoot", Tr.language)
                font.pixelSize: Appearance.font.pixelSize.smallest
                color: Appearance.colors.colOnLayer1
                wrapMode: Text.Wrap
            }
            SequentialAnimation {
                id: busyAnim
                property real value: 0.05
                running: root.running && root.percent < 0 && root.visible
                loops: Animation.Infinite
                NumberAnimation {
                    target: busyAnim
                    property: "value"
                    from: 0.05
                    to: 0.95
                    duration: 1000
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    target: busyAnim
                    property: "value"
                    from: 0.95
                    to: 0.05
                    duration: 1000
                    easing.type: Easing.InOutQuad
                }
            }
        }

        // ---- live log --------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 18
            radius: Appearance.rounding.small
            color: Appearance.colors.colSurfaceContainerHighest
            clip: true

            Flickable {
                id: logFlick
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                contentHeight: logColumn.implicitHeight
                contentWidth: width
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: logColumn
                    width: logFlick.width
                    spacing: 0

                    Repeater {
                        model: root.logLines
                        StyledText {
                            required property string modelData
                            Layout.fillWidth: true
                            text: modelData
                            font.pixelSize: Appearance.font.pixelSize.smallest
                            font.family: "monospace"
                            color: Appearance.colors.colOnLayer1
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        // ---- footer -----------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            spacing: 8

            StyledText {
                visible: !root.running && root.displayMessage !== "" && root.message !== "ok"
                Layout.fillWidth: true
                text: root.displayMessage
                font.pixelSize: Appearance.font.pixelSize.smallest
                color: root.installOk ? Appearance.colors.colOnLayer1
                                      : Appearance.m3colors.m3error
                elide: Text.ElideMiddle
            }
            Item { Layout.fillWidth: true }
            RippleButton {
                buttonText: Tr.t("close", Tr.language)
                buttonRadius: Appearance.rounding.small
                enabled: !root.running
                onClicked: root.close()
            }
        }
    }
}
