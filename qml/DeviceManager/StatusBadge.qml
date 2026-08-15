// Colored status pill used in the device list and details pane.
import QtQuick
import QtQuick.Layouts
import Components

Rectangle {
    id: root

    property string status: "ok"
    property int labelPixelSize: 12

    readonly property var info: root.infoFor(root.status)

    function infoFor(status) {
        switch (status) {
        case "ok":
            return {
                bg: Appearance.m3colors.m3successContainer,
                fg: Appearance.m3colors.m3onSuccessContainer,
                icon: "check_circle"
            };
        case "disabled":
            return {
                bg: Appearance.colors.colSurfaceContainerHighest,
                fg: Appearance.colors.colOnSurfaceVariant,
                icon: "block"
            };
        case "suspended":
            return {
                bg: Appearance.m3colors.m3tertiaryContainer,
                fg: Appearance.m3colors.m3onTertiaryContainer,
                icon: "pause_circle"
            };
        case "unplugged":
            return { bg: "#FFD54F", fg: "#4A3800", icon: "wifi_off" };
        case "error":
            return {
                bg: Appearance.m3colors.m3errorContainer,
                fg: Appearance.m3colors.m3onErrorContainer,
                icon: "error"
            };
        default:
            return {
                bg: Appearance.colors.colSurfaceContainerHighest,
                fg: Appearance.colors.colOnSurfaceVariant,
                icon: "help"
            };
        }
    }

    implicitHeight: 22
    implicitWidth: statusRow.implicitWidth + 16
    radius: 11
    color: root.info.bg

    RowLayout {
        id: statusRow
        anchors.centerIn: parent
        spacing: 4
        MaterialSymbol {
            text: root.info.icon
            iconSize: root.labelPixelSize + 2
            color: root.info.fg
        }
        StyledText {
            text: Tr.t(root.status === "ok" ? "statusOk"
                : root.status === "disabled" ? "statusDisabled"
                : root.status === "suspended" ? "statusSuspended"
                : root.status === "unplugged" ? "statusUnplugged"
                : root.status === "error" ? "statusProblem"
                : "unknown", Tr.language)
            font.pixelSize: root.labelPixelSize
            color: root.info.fg
        }
    }
}
