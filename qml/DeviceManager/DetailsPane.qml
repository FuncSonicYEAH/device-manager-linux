// Right-hand details pane showing the selected device.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

Item {
    id: root

    property var device: null

    signal propertiesRequested()
    signal refreshRequested()
    signal smartRequested()
    signal graphicsRequested()
    signal temperatureRequested()
    signal gpuMonitorRequested()
    signal networkMonitorRequested()

    // Button text uses StyledText with Appearance.font.pixelSize.small (15px);
    // size the text buttons from their label length so they fit every language:
    // full-width glyphs (CJK) take a whole em, latin/digits roughly half.
    function textButtonWidth(text) {
        if (text === undefined || text === null)
            return 0
        var full = Appearance.font.pixelSize.small
        var half = Math.round(full * 0.55)
        var w = 0
        for (var i = 0; i < text.length; i++) {
            var code = text.charCodeAt(i)
            if (code > 0x2E7F)
                w += full
            else
                w += half
        }
        return w + 24 // horizontal padding
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- empty state -------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.device === null

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 10
                MaterialSymbol {
                    Layout.alignment: Qt.AlignHCenter
                    text: "devices_other"
                    iconSize: 56
                    color: Appearance.colors.colOnLayer1Inactive
                }
                StyledText {
                    Layout.alignment: Qt.AlignHCenter
                    text: Tr.t("selectDeviceHint", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.normal
                    color: Appearance.colors.colOnLayer1Inactive
                }
                StyledText {
                    Layout.alignment: Qt.AlignHCenter
                    text: Tr.t("rightClickHint", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smaller
                    color: Appearance.colors.colOnLayer1Inactive
                }
            }
        }

        // ---- device info --------------------------------------------------
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.device !== null
            clip: true
            contentHeight: contentColumn.implicitHeight + 24

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            ColumnLayout {
                id: contentColumn
                width: root.width - 8
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 16
                spacing: 16

                // header
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    Rectangle {
                        Layout.preferredWidth: 56
                        Layout.preferredHeight: 56
                        radius: Appearance.rounding.large
                        color: Appearance.colors.colSecondaryContainer
                        MaterialSymbol {
                            anchors.centerIn: parent
                            text: root.device !== null ? root.device.icon : ""
                            iconSize: 30
                            color: Appearance.colors.colOnSecondaryContainer
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        StyledText {
                            Layout.fillWidth: true
                            text: root.device !== null ? root.device.name : ""
                            font.pixelSize: Appearance.font.pixelSize.title
                            font.weight: Font.Medium
                            color: Appearance.colors.colOnLayer1
                            wrapMode: Text.Wrap
                        }
                        StyledText {
                            Layout.fillWidth: true
                            text: root.device !== null
                                ? root.device.categoryName + (root.device.bus !== "" ? " · " + root.device.bus : "")
                                : ""
                            font.pixelSize: Appearance.font.pixelSize.smaller
                            color: Appearance.colors.colOnLayer1
                            wrapMode: Text.Wrap
                        }
                    }

                    StatusBadge {
                        status: root.device !== null ? root.device.status : "ok"
                        Layout.rightMargin: 12
                    }
                }

                // actions (flow wraps onto a second line when the pane is narrow)
                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    RippleButton {
                        buttonText: Tr.t("properties", Tr.language)
                        buttonRadius: Appearance.rounding.small
                        implicitWidth: root.textButtonWidth(Tr.t("properties", Tr.language))
                        implicitHeight: 36
                        onClicked: root.propertiesRequested()
                    }
                    RippleButton {
                        buttonText: Tr.t("scanHardwareChanges", Tr.language)
                        buttonRadius: Appearance.rounding.small
                        implicitWidth: root.textButtonWidth(Tr.t("scanHardwareChanges", Tr.language))
                        implicitHeight: 36
                        onClicked: root.refreshRequested()
                    }
                    RippleButton {
                        visible: root.device !== null && root.device.category === "disk"
                        buttonText: Tr.t("smartHealthCheck", Tr.language)
                        buttonRadius: Appearance.rounding.small
                        implicitWidth: root.textButtonWidth(Tr.t("smartHealthCheck", Tr.language))
                        implicitHeight: 36
                        onClicked: root.smartRequested()
                    }
                    RippleButton {
                        visible: root.device !== null && root.device.category === "display"
                        buttonText: Tr.t("graphicsSupportTitle", Tr.language)
                        buttonRadius: Appearance.rounding.small
                        implicitWidth: root.textButtonWidth(Tr.t("graphicsSupportTitle", Tr.language))
                        implicitHeight: 36
                        onClicked: root.graphicsRequested()
                    }
                    RippleButton {
                        visible: root.device !== null && Temperature.supportsTemperature(root.device)
                        buttonText: Tr.t("temperatureCurveTitle", Tr.language)
                        buttonRadius: Appearance.rounding.small
                        implicitWidth: root.textButtonWidth(Tr.t("temperatureCurveTitle", Tr.language))
                        implicitHeight: 36
                        onClicked: root.temperatureRequested()
                    }
                    RippleButton {
                        visible: root.device !== null && root.device.category === "display"
                                 && Monitor.supportsGpuMonitoring(root.device)
                        buttonText: Tr.t("gpuMonitorTitle", Tr.language)
                        buttonRadius: Appearance.rounding.small
                        implicitWidth: root.textButtonWidth(Tr.t("gpuMonitorTitle", Tr.language))
                        implicitHeight: 36
                        onClicked: root.gpuMonitorRequested()
                    }
                    RippleButton {
                        visible: root.device !== null && root.device.category === "network"
                                 && Monitor.supportsNetMonitoring(root.device)
                        buttonText: Tr.t("networkMonitorTitle", Tr.language)
                        buttonRadius: Appearance.rounding.small
                        implicitWidth: root.textButtonWidth(Tr.t("networkMonitorTitle", Tr.language))
                        implicitHeight: 36
                        onClicked: root.networkMonitorRequested()
                    }
                    RippleButton {
                        buttonRadius: Appearance.rounding.small
                        implicitWidth: 36
                        implicitHeight: 36
                        onClicked: root.refreshRequested()
                        contentItem: MaterialSymbol {
                            anchors.centerIn: parent
                            text: "refresh"
                            iconSize: 18
                            color: Appearance.colors.colOnLayer1
                        }
                    }
                }

                // properties
                StyledText {
                    text: Tr.t("deviceProperties", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.larger
                    font.weight: Font.Medium
                    color: Appearance.colors.colOnLayer1
                }

                PropsList {
                    Layout.fillWidth: true
                    props: root.device !== null ? root.device.props : []
                    showDividers: true
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}
