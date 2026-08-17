// Warning dialog shown before device power actions (suspend / enable /
// start). After the user confirms, the action runs through the `DeviceActions`
// backend (pkexec) and this dialog switches to a spinner while the polkit
// prompt is up, then to a success/failure result.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

AnimatedDialog {
    id: root

    property var device: null
    property string action: "suspend"   // suspend | enable | start

    property bool running: false
    property bool done: false
    property bool actionOk: false
    property string resultMessage: ""

    signal actionCompleted(string deviceId)

    width: Math.min(500, parent.width * 0.85)
    height: Math.min(360, parent.height * 0.8)

    readonly property string actionIcon: root.action === "suspend" ? "pause_circle"
        : root.action === "enable" ? "play_circle" : "power_settings_new"
    readonly property string actionTitle: root.action === "suspend"
        ? Tr.t("suspendDevice", Tr.language)
        : root.action === "enable" ? Tr.t("enableDevice", Tr.language)
        : Tr.t("startDevice", Tr.language)
    readonly property string actionWarning: root.action === "suspend"
        ? Tr.t("suspendWarning", Tr.language)
        : root.action === "enable" ? Tr.t("enableWarning", Tr.language)
        : Tr.t("startWarning", Tr.language)
    readonly property string actionRequiresRoot: Tr.t("actionRequiresRoot", Tr.language)

    function resetState() {
        root.running = false
        root.done = false
        root.actionOk = false
        root.resultMessage = ""
    }

    function start() {
        root.running = true
        root.done = false
        DeviceActions.perform(root.device, root.action)
    }

    Connections {
        target: DeviceActions
        function onActionFinished(deviceId, action, ok, message) {
            if (root.device !== null && deviceId === root.device.id
                && action === root.action) {
                root.running = false
                root.done = true
                root.actionOk = ok
                root.resultMessage = message
                if (ok)
                    root.actionCompleted(deviceId)
            }
        }
    }

    onOpened: root.resetState()

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
                    text: root.actionIcon
                    iconSize: 24
                    color: Appearance.colors.colOnSecondaryContainer
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                StyledText {
                    Layout.fillWidth: true
                    text: root.actionTitle
                    font.pixelSize: Appearance.font.pixelSize.normal
                    font.weight: Font.Medium
                    color: Appearance.colors.colOnLayer1
                    elide: Text.ElideRight
                }
                StyledText {
                    Layout.fillWidth: true
                    text: root.device !== null ? root.device.name : ""
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1
                    elide: Text.ElideRight
                }
            }

            StatusBadge {
                status: root.device !== null ? root.device.status : "ok"
                Layout.rightMargin: 8
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
                    color: Appearance.colors.colOnLayer1
                }
            }
        }

        // ---- body -------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 12
            clip: true

            // confirm
            ColumnLayout {
                anchors.fill: parent
                visible: !root.running && !root.done
                spacing: 14

                MaterialSymbol {
                    Layout.alignment: Qt.AlignHCenter
                    text: "warning"
                    iconSize: 46
                    color: Appearance.m3colors.m3tertiary
                }
                StyledText {
                    Layout.fillWidth: true
                    text: root.actionWarning
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    font.pixelSize: Appearance.font.pixelSize.smallie
                    color: Appearance.colors.colOnLayer1
                }
                StyledText {
                    Layout.fillWidth: true
                    text: root.actionRequiresRoot
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1
                }
                StyledText {
                    Layout.fillWidth: true
                    text: root.device !== null ? root.device.sysfsPath : ""
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideMiddle
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1
                }
                Item { Layout.fillHeight: true }
            }

            // running
            ColumnLayout {
                anchors.centerIn: parent
                visible: root.running
                spacing: 14

                CircularProgress {
                    id: spinner
                    Layout.alignment: Qt.AlignHCenter
                    implicitSize: 34
                    value: spinnerAnim.value
                }
                SequentialAnimation {
                    id: spinnerAnim
                    property real value: 0.05
                    running: root.running
                    loops: Animation.Infinite
                    NumberAnimation {
                        target: spinnerAnim
                        property: "value"
                        from: 0.05
                        to: 0.95
                        duration: 1000
                        easing.type: Easing.InOutQuad
                    }
                    NumberAnimation {
                        target: spinnerAnim
                        property: "value"
                        from: 0.95
                        to: 0.05
                        duration: 1000
                        easing.type: Easing.InOutQuad
                    }
                }
                StyledText {
                    Layout.alignment: Qt.AlignHCenter
                    text: Tr.t("actionRunning", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallie
                    color: Appearance.colors.colOnLayer1
                }
            }

            // result
            ColumnLayout {
                anchors.centerIn: parent
                visible: root.done
                spacing: 12

                MaterialSymbol {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.actionOk ? "check_circle" : "error"
                    iconSize: 46
                    color: root.actionOk
                        ? Appearance.m3colors.m3success
                        : Appearance.m3colors.m3error
                }
                StyledText {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.actionOk
                        ? Tr.t("actionSucceeded", Tr.language)
                        : Tr.t("actionFailed", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.normal
                    font.weight: Font.Medium
                    color: Appearance.colors.colOnLayer1
                }
                StyledText {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.maximumWidth: root.width - 80
                    visible: root.resultMessage !== ""
                    text: root.resultMessage
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1
                }
            }
        }

        // ---- footer -----------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            spacing: 8

            Item { Layout.fillWidth: true }

            RippleButton {
                visible: !root.running && !root.done
                buttonText: Tr.t("cancel", Tr.language)
                buttonRadius: Appearance.rounding.small
                onClicked: root.close()
            }
            RippleButton {
                visible: !root.running && !root.done
                buttonText: Tr.t("confirm", Tr.language)
                buttonRadius: Appearance.rounding.small
                colBackground: Appearance.m3colors.m3tertiaryContainer
                colBackgroundHover: Appearance.m3colors.m3tertiaryContainer
                colBackgroundActive: Appearance.m3colors.m3tertiaryContainer
                onClicked: root.start()
                contentItem: StyledText {
                    text: Tr.t("confirm", Tr.language)
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: Appearance.font.pixelSize.smallie
                    color: Appearance.m3colors.m3onTertiaryContainer
                }
            }
            RippleButton {
                visible: root.done
                buttonText: Tr.t("close", Tr.language)
                buttonRadius: Appearance.rounding.small
                onClicked: root.close()
            }
        }
    }
}
