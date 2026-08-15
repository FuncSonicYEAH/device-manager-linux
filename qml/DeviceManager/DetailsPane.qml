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

                // actions
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    RippleButton {
                        buttonText: Tr.t("properties", Tr.language)
                        buttonRadius: Appearance.rounding.small
                        implicitWidth: 90
                        implicitHeight: 36
                        onClicked: root.propertiesRequested()
                    }
                    RippleButton {
                        buttonText: Tr.t("scanHardwareChanges", Tr.language)
                        buttonRadius: Appearance.rounding.small
                        implicitWidth: 130
                        implicitHeight: 36
                        onClicked: root.refreshRequested()
                    }
                    RippleButton {
                        visible: root.device !== null && root.device.category === "disk"
                        buttonText: Tr.t("smartHealthCheck", Tr.language)
                        buttonRadius: Appearance.rounding.small
                        implicitWidth: 120
                        implicitHeight: 36
                        onClicked: root.smartRequested()
                    }
                    RippleButton {
                        visible: root.device !== null && root.device.category === "display"
                        buttonText: Tr.t("graphicsSupportTitle", Tr.language)
                        buttonRadius: Appearance.rounding.small
                        implicitWidth: 130
                        implicitHeight: 36
                        onClicked: root.graphicsRequested()
                    }
                    Item { Layout.fillWidth: true }
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
