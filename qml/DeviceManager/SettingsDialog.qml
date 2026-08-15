// Settings dialog: appearance (system/light/dark theme) and UI language.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

Dialog {
    id: root

    modal: true
    dim: true
    anchors.centerIn: parent
    width: Math.min(420, parent.width * 0.85)
    height: Math.min(430, parent.height * 0.85)
    padding: 0

    Overlay.modal: Rectangle {
        color: Appearance.colors.colScrim
    }

    background: Rectangle {
        radius: Appearance.rounding.windowRounding
        color: Appearance.m3colors.m3surfaceContainerLow
        border.color: Appearance.colors.colOutlineVariant
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 0

        // header
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 18
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                radius: Appearance.rounding.normal
                color: Appearance.colors.colSecondaryContainer
                MaterialSymbol {
                    anchors.centerIn: parent
                    text: "settings"
                    iconSize: 22
                    color: Appearance.colors.colOnSecondaryContainer
                }
            }

            StyledText {
                Layout.fillWidth: true
                text: Tr.t("settings", Tr.language)
                font.pixelSize: Appearance.font.pixelSize.normal
                font.weight: Font.Medium
                color: Appearance.m3colors.m3onSurface
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

        // body
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 14
            clip: true
            contentHeight: bodyColumn.implicitHeight

            ColumnLayout {
                id: bodyColumn
                width: parent.width
                spacing: 18

                // ---- appearance -------------------------------------------------
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    StyledText {
                        text: Tr.t("appearance", Tr.language)
                        font.pixelSize: Appearance.font.pixelSize.smallie
                        font.weight: Font.Medium
                        color: Appearance.m3colors.m3onSurface
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        RippleButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            buttonRadius: Appearance.rounding.small
                            toggled: Appearance.themeMode === "system"
                            colBackgroundToggled: Appearance.colors.colSecondaryContainer
                            colBackgroundToggledHover: Appearance.colors.colSecondaryContainerHover
                            colBackgroundToggledActive: Appearance.colors.colSecondaryContainerActive
                            onClicked: Appearance.setThemeMode("system")
                            contentItem: StyledText {
                                text: Tr.t("followSystem", Tr.language)
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: Appearance.font.pixelSize.smallie
                                color: Appearance.themeMode === "system"
                                    ? Appearance.colors.colOnSecondaryContainer
                                    : Appearance.colors.colOnLayer1
                            }
                        }
                        RippleButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            buttonRadius: Appearance.rounding.small
                            toggled: Appearance.themeMode === "light"
                            colBackgroundToggled: Appearance.colors.colSecondaryContainer
                            colBackgroundToggledHover: Appearance.colors.colSecondaryContainerHover
                            colBackgroundToggledActive: Appearance.colors.colSecondaryContainerActive
                            onClicked: Appearance.setThemeMode("light")
                            contentItem: StyledText {
                                text: Tr.t("light", Tr.language)
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: Appearance.font.pixelSize.smallie
                                color: Appearance.themeMode === "light"
                                    ? Appearance.colors.colOnSecondaryContainer
                                    : Appearance.colors.colOnLayer1
                            }
                        }
                        RippleButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            buttonRadius: Appearance.rounding.small
                            toggled: Appearance.themeMode === "dark"
                            colBackgroundToggled: Appearance.colors.colSecondaryContainer
                            colBackgroundToggledHover: Appearance.colors.colSecondaryContainerHover
                            colBackgroundToggledActive: Appearance.colors.colSecondaryContainerActive
                            onClicked: Appearance.setThemeMode("dark")
                            contentItem: StyledText {
                                text: Tr.t("dark", Tr.language)
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: Appearance.font.pixelSize.smallie
                                color: Appearance.themeMode === "dark"
                                    ? Appearance.colors.colOnSecondaryContainer
                                    : Appearance.colors.colOnLayer1
                            }
                        }
                    }
                }

                // ---- language ---------------------------------------------------
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    StyledText {
                        text: Tr.t("language", Tr.language)
                        font.pixelSize: Appearance.font.pixelSize.smallie
                        font.weight: Font.Medium
                        color: Appearance.m3colors.m3onSurface
                    }

                    Repeater {
                        model: Tr.languageCodes
                        delegate: RippleButton {
                            required property int index
                            required property string modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            buttonRadius: Appearance.rounding.small
                            toggled: Tr.language === modelData
                            colBackgroundToggled: Appearance.colors.colSecondaryContainer
                            colBackgroundToggledHover: Appearance.colors.colSecondaryContainerHover
                            colBackgroundToggledActive: Appearance.colors.colSecondaryContainerActive
                            onClicked: Tr.setLanguage(modelData)
                            contentItem: RowLayout {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 8
                                StyledText {
                                    text: Tr.languageNames[index]
                                    Layout.fillWidth: true
                                    font.pixelSize: Appearance.font.pixelSize.smallie
                                    color: Tr.language === modelData
                                        ? Appearance.colors.colOnSecondaryContainer
                                        : Appearance.colors.colOnLayer1
                                }
                                MaterialSymbol {
                                    visible: Tr.language === modelData
                                    text: "check"
                                    iconSize: 18
                                    color: Appearance.colors.colOnSecondaryContainer
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
