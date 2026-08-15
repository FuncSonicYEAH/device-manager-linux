// SMART health check dialog for disk drives. Reads the data asynchronously
// through the `Smart` backend (pkexec smartctl -a -j) and shows an overall
// health banner, key counters and the full attribute table.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

AnimatedDialog {
    id: root

    property var device: null
    property var smartData: null
    property bool loading: false

    width: Math.min(680, parent.width * 0.88)
    height: Math.min(580, parent.height * 0.88)

    readonly property string deviceNode: root.device !== null && root.device.entryName !== ""
        ? "/dev/" + root.device.entryName : ""

    function startRead() {
        if (root.device === null || root.deviceNode === "")
            return
        root.smartData = null
        root.loading = true
        Smart.request(root.deviceNode, root.device.id)
    }

    // Maps backend failure reasons to translated text.
    function errorText(data) {
        var msg = data !== null ? data.message : ""
        if (msg === "not authorized")
            return Tr.t("smartNotAuthorized", Tr.language)
        if (msg === "permission denied")
            return Tr.t("smartRequiresRoot", Tr.language)
        if (msg === "not supported")
            return Tr.t("smartNotSupported", Tr.language)
        if (msg === "pkexec not found")
            return Tr.t("smartNoPolkit", Tr.language)
        if (msg === "busy")
            return Tr.t("smartReading", Tr.language)
        return msg !== "" ? msg : Tr.t("smartReadFailed", Tr.language)
    }

    Connections {
        target: Smart
        function onSmartReady(deviceId, data) {
            if (root.device !== null && deviceId === root.device.id) {
                root.smartData = data
                root.loading = false
            }
        }
    }

    onOpened: root.startRead()

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
                    text: root.device !== null ? root.device.icon : "storage"
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
                    text: Tr.t("smartHealthCheck", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1
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

            // loading
            ColumnLayout {
                anchors.centerIn: parent
                visible: root.loading
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
                    running: root.loading
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
                    text: Tr.t("smartReading", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallie
                    color: Appearance.colors.colOnLayer1
                }
            }

            // read error / unsupported
            ColumnLayout {
                anchors.centerIn: parent
                visible: !root.loading && root.smartData !== null && !root.smartData.ok
                spacing: 12

                MaterialSymbol {
                    Layout.alignment: Qt.AlignHCenter
                    text: "error"
                    iconSize: 44
                    color: Appearance.m3colors.m3error
                }
                StyledText {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.maximumWidth: parent.width - 40
                    text: root.errorText(root.smartData)
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    font.pixelSize: Appearance.font.pixelSize.smallie
                    color: Appearance.colors.colOnLayer1
                }
            }

            // SMART unsupported
            ColumnLayout {
                anchors.centerIn: parent
                visible: !root.loading && root.smartData !== null && root.smartData.ok
                         && !root.smartData.available
                spacing: 12

                MaterialSymbol {
                    Layout.alignment: Qt.AlignHCenter
                    text: "help"
                    iconSize: 44
                    color: Appearance.colors.colOnLayer1Inactive
                }
                StyledText {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.errorText(root.smartData)
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: Appearance.font.pixelSize.smallie
                    color: Appearance.colors.colOnLayer1
                }
            }

            // data (only instantiated once the read succeeded, so the
            // smartData bindings below are safe)
            Loader {
                anchors.fill: parent
                active: !root.loading && root.smartData !== null && root.smartData.ok === true
                        && root.smartData.available === true
                sourceComponent: Component {
                    Flickable {
                        anchors.fill: parent
                        clip: true
                        contentHeight: dataColumn.implicitHeight + 8
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        ColumnLayout {
                            id: dataColumn
                            width: parent.width
                            spacing: 12

                            // ---- health banner --------------------------
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 64
                                radius: Appearance.rounding.normal
                                color: root.smartData.healthKnown
                                    ? (root.smartData.healthPassed
                                        ? Appearance.m3colors.m3successContainer
                                        : Appearance.m3colors.m3errorContainer)
                                    : Appearance.colors.colSurfaceContainerHighest

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 16
                                    anchors.rightMargin: 16
                                    spacing: 12

                                    MaterialSymbol {
                                        text: !root.smartData.healthKnown ? "help"
                                            : root.smartData.healthPassed ? "check_circle" : "error"
                                        iconSize: 30
                                        color: !root.smartData.healthKnown
                                            ? Appearance.colors.colOnSurfaceVariant
                                            : root.smartData.healthPassed
                                                ? Appearance.m3colors.m3onSuccessContainer
                                                : Appearance.m3colors.m3onErrorContainer
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        StyledText {
                                            text: Tr.t("smartHealth", Tr.language)
                                            font.pixelSize: Appearance.font.pixelSize.smallest
                                            color: !root.smartData.healthKnown
                                                ? Appearance.colors.colOnSurfaceVariant
                                                : root.smartData.healthPassed
                                                    ? Appearance.m3colors.m3onSuccessContainer
                                                    : Appearance.m3colors.m3onErrorContainer
                                        }
                                        StyledText {
                                            Layout.fillWidth: true
                                            text: root.smartData.modelName !== ""
                                                ? root.smartData.modelName
                                                : (root.smartData.modelFamily !== ""
                                                    ? root.smartData.modelFamily
                                                    : Tr.t("unknown", Tr.language))
                                            elide: Text.ElideRight
                                            font.pixelSize: Appearance.font.pixelSize.small
                                            font.weight: Font.Medium
                                            color: !root.smartData.healthKnown
                                                ? Appearance.colors.colOnSurfaceVariant
                                                : root.smartData.healthPassed
                                                    ? Appearance.m3colors.m3onSuccessContainer
                                                    : Appearance.m3colors.m3onErrorContainer
                                        }
                                    }

                                    StyledText {
                                        text: !root.smartData.healthKnown
                                            ? Tr.t("unknown", Tr.language)
                                            : root.smartData.healthPassed
                                                ? Tr.t("smartPassed", Tr.language)
                                                : Tr.t("smartFailed", Tr.language)
                                        font.pixelSize: Appearance.font.pixelSize.normal
                                        font.weight: Font.DemiBold
                                        color: !root.smartData.healthKnown
                                            ? Appearance.colors.colOnSurfaceVariant
                                            : root.smartData.healthPassed
                                                ? Appearance.m3colors.m3onSuccessContainer
                                                : Appearance.m3colors.m3onErrorContainer
                                    }
                                }
                            }

                            // ---- key counters ---------------------------
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Repeater {
                                    model: [
                                        { key: "smartTemperature",
                                          value: root.smartData.temperature >= 0
                                              ? root.smartData.temperature + " °C" : "—",
                                          icon: "thermostat" },
                                        { key: "smartPowerOnHours",
                                          value: root.smartData.powerOnHours >= 0
                                              ? root.smartData.powerOnHours + " h" : "—",
                                          icon: "schedule" },
                                        { key: "smartPowerCycles",
                                          value: root.smartData.powerCycles >= 0
                                              ? root.smartData.powerCycles : "—",
                                          icon: "repeat" }
                                    ]
                                    delegate: Rectangle {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 72
                                        radius: Appearance.rounding.normal
                                        color: Appearance.colors.colLayer2

                                        ColumnLayout {
                                            anchors.centerIn: parent
                                            spacing: 3

                                            RowLayout {
                                                Layout.alignment: Qt.AlignHCenter
                                                spacing: 5
                                                MaterialSymbol {
                                                    text: modelData.icon
                                                    iconSize: 14
                                                    color: Appearance.colors.colOnLayer1Inactive
                                                }
                                                StyledText {
                                                    text: Tr.t(modelData.key, Tr.language)
                                                    font.pixelSize: Appearance.font.pixelSize.smallest
                                                    color: Appearance.colors.colOnLayer1Inactive
                                                }
                                            }
                                            StyledText {
                                                Layout.alignment: Qt.AlignHCenter
                                                text: modelData.value
                                                font.pixelSize: Appearance.font.pixelSize.small
                                                font.weight: Font.DemiBold
                                                color: Appearance.colors.colOnLayer1
                                            }
                                        }
                                    }
                                }
                            }

                            // ---- attribute table ------------------------
                            StyledText {
                                text: Tr.t("smartAttribute", Tr.language)
                                font.pixelSize: Appearance.font.pixelSize.larger
                                font.weight: Font.Medium
                                color: Appearance.colors.colOnLayer1
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                // header row
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.bottomMargin: 4
                                    spacing: 10

                                    Repeater {
                                        model: [
                                            { key: "smartAttrId", width: 40, fill: false },
                                            { key: "smartAttrName", width: 0, fill: true },
                                            { key: "smartAttrValue", width: 46, fill: false },
                                            { key: "smartAttrWorst", width: 46, fill: false },
                                            { key: "smartAttrThresh", width: 50, fill: false },
                                            { key: "smartAttrRaw", width: 110, fill: false }
                                        ]
                                        delegate: StyledText {
                                            required property var modelData
                                            text: Tr.t(modelData.key, Tr.language)
                                            Layout.fillWidth: modelData.fill
                                            Layout.preferredWidth: modelData.fill ? -1 : modelData.width
                                            font.pixelSize: Appearance.font.pixelSize.smallest
                                            font.weight: Font.Medium
                                            color: Appearance.colors.colOnLayer1Inactive
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 1
                                    color: Appearance.colors.colOutlineVariant
                                    opacity: 0.35
                                }

                                Repeater {
                                    model: root.smartData.attributes
                                    delegate: RowLayout {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 30
                                        spacing: 10

                                        StyledText {
                                            text: modelData.id > 0 ? modelData.id : "—"
                                            Layout.preferredWidth: 40
                                            font.pixelSize: Appearance.font.pixelSize.smallest
                                            color: Appearance.colors.colOnLayer1Inactive
                                        }
                                        StyledText {
                                            text: modelData.name
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                            font.pixelSize: Appearance.font.pixelSize.smallest
                                            color: Appearance.colors.colOnLayer1
                                        }
                                        StyledText {
                                            text: modelData.value >= 0 ? modelData.value : "—"
                                            Layout.preferredWidth: 46
                                            font.pixelSize: Appearance.font.pixelSize.smallest
                                            color: Appearance.colors.colOnLayer1
                                        }
                                        StyledText {
                                            text: modelData.worst >= 0 ? modelData.worst : "—"
                                            Layout.preferredWidth: 46
                                            font.pixelSize: Appearance.font.pixelSize.smallest
                                            color: Appearance.colors.colOnLayer1
                                        }
                                        StyledText {
                                            text: modelData.thresh >= 0 ? modelData.thresh : "—"
                                            Layout.preferredWidth: 50
                                            font.pixelSize: Appearance.font.pixelSize.smallest
                                            color: Appearance.colors.colOnLayer1
                                        }
                                        StyledText {
                                            text: modelData.raw
                                            Layout.preferredWidth: 110
                                            elide: Text.ElideRight
                                            font.pixelSize: Appearance.font.pixelSize.smallest
                                            color: Appearance.m3colors.m3onSurface
                                        }
                                    }
                                }
                            }
                        }
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
                buttonText: Tr.t("refresh", Tr.language)
                buttonRadius: Appearance.rounding.small
                implicitWidth: 80
                implicitHeight: 34
                enabled: !root.loading
                onClicked: root.startRead()
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
