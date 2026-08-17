// Device properties dialog, in the spirit of the Windows Device Manager
// "Properties" sheet with General / Details tabs.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

AnimatedDialog {
    id: root

    property var device: null
    property int currentTab: 0

    signal refreshRequested()

    width: Math.min(560, parent.width * 0.85)
    height: Math.min(460, parent.height * 0.85)

    contentItem: ColumnLayout {
        spacing: 0

        // header
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
                    text: root.device !== null ? root.device.icon : "devices_other"
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
                    text: root.device !== null ? root.device.categoryName : ""
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

        // tabs
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 12
            spacing: 6

            RippleButton {
                buttonText: Tr.t("general", Tr.language)
                buttonRadius: Appearance.rounding.full
                toggled: root.currentTab === 0
                colBackgroundToggled: Appearance.colors.colSecondaryContainer
                colBackgroundToggledHover: Appearance.colors.colSecondaryContainerHover
                colBackgroundToggledActive: Appearance.colors.colSecondaryContainerActive
                onClicked: root.currentTab = 0
            }
            RippleButton {
                buttonText: Tr.t("details", Tr.language)
                buttonRadius: Appearance.rounding.full
                toggled: root.currentTab === 1
                colBackgroundToggled: Appearance.colors.colSecondaryContainer
                colBackgroundToggledHover: Appearance.colors.colSecondaryContainerHover
                colBackgroundToggledActive: Appearance.colors.colSecondaryContainerActive
                onClicked: root.currentTab = 1
            }
            Item { Layout.fillWidth: true }
        }

        // body
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 12
            clip: true

            Flickable {
                anchors.fill: parent
                visible: root.currentTab === 0
                clip: true
                contentHeight: generalColumn.implicitHeight + 8
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: generalColumn
                    width: parent.width
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        StyledText {
                            text: Tr.t("status", Tr.language)
                            Layout.preferredWidth: 120
                            font.pixelSize: Appearance.font.pixelSize.smallie
                            font.weight: Font.Medium
                            color: Appearance.m3colors.m3onSurface
                        }
                        StatusBadge {
                            status: root.device !== null ? root.device.status : "ok"
                        }
                    }

                    PropsList {
                        Layout.fillWidth: true
                        props: root.device !== null ? root.device.props.slice(0, 8) : []
                        showDividers: true
                        nameWidth: 110
                    }
                }
            }

            Flickable {
                anchors.fill: parent
                visible: root.currentTab === 1
                clip: true
                contentHeight: detailsColumn.implicitHeight + 8
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: detailsColumn
                    width: parent.width
                    spacing: 4

                    PropsList {
                        Layout.fillWidth: true
                        props: root.device !== null ? root.device.props : []
                        showDividers: true
                        nameWidth: 130
                    }
                }
            }
        }

        // footer
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            spacing: 8

            RippleButton {
                buttonText: Tr.t("refresh", Tr.language)
                buttonRadius: Appearance.rounding.small
                onClicked: root.refreshRequested()
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
