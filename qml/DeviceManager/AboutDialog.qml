// About dialog: app icon/title/version, a short description, runtime info
// (Qt version) and the OS the app is running on. Static data comes from the
// `AboutInfo` context property, strings are translated through `Tr`.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

AnimatedDialog {
    id: root

    width: Math.min(430, parent.width * 0.85)
    height: Math.min(500, parent.height * 0.85)

    contentItem: ColumnLayout {
        spacing: 0

        // ---- centered header -------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 26
            Layout.bottomMargin: 8
            spacing: 8

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 64
                Layout.preferredHeight: 64
                radius: 18
                color: Appearance.colors.colSecondaryContainer
                MaterialSymbol {
                    anchors.centerIn: parent
                    text: "devices_other"
                    iconSize: 34
                    color: Appearance.colors.colOnSecondaryContainer
                }
            }

            StyledText {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: Tr.t("appTitle", Tr.language)
                font.pixelSize: Appearance.font.pixelSize.larger
                font.weight: Font.Medium
                color: Appearance.colors.colOnLayer1
            }

            StyledText {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: Tr.t("aboutVersion", Tr.language).arg(AboutInfo.appVersion)
                font.pixelSize: Appearance.font.pixelSize.smallest
                color: Appearance.colors.colOnLayer1
            }

            StyledText {
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: 340
                text: Tr.t("appDescription", Tr.language)
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                font.pixelSize: Appearance.font.pixelSize.smallest
                color: Appearance.colors.colOnLayer1
            }

            Item { Layout.preferredHeight: 4 }
        }

        // ---- runtime info ----------------------------------------------
        PropsList {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 6
            nameWidth: 130
            showDividers: true
            props: [
                { name: Tr.t("version", Tr.language), value: AboutInfo.appVersion },
                { name: Tr.t("qtVersion", Tr.language), value: AboutInfo.qtVersion },
                { name: Tr.t("operatingSystem", Tr.language), value: AboutInfo.osName },
                { name: Tr.t("kernel", Tr.language), value: AboutInfo.kernel },
                { name: Tr.t("cpuArchitecture", Tr.language), value: AboutInfo.cpuArch }
            ]
        }

        Item { Layout.fillHeight: true }

        // ---- footer -----------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            spacing: 8

            StyledText {
                Layout.fillWidth: true
                text: Tr.t("license", Tr.language) + " · GNU GPL v3"
                elide: Text.ElideRight
                font.pixelSize: Appearance.font.pixelSize.smallest
                color: Appearance.colors.colOnLayer1
            }
            RippleButton {
                buttonText: Tr.t("close", Tr.language)
                buttonRadius: Appearance.rounding.small
                onClicked: root.close()
            }
        }
    }
}
