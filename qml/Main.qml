// 设备管理器 — a Windows Device Manager style hardware browser built on the
// m3-gallery Material 3 widget set (Qt6 Quick + C++ + Meson).
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Components
import "DeviceManager"

ApplicationWindow {
    id: root

    visible: true
    title: Tr.t("appTitle", Tr.language)

    minimumWidth: 920
    minimumHeight: 600
    width: 1160
    height: 760
    color: Appearance.m3colors.m3background

    // ---- state ------------------------------------------------------------
    property string viewMode: "type"          // "type" | "connection"
    property var expandedKeys: ({})
    property string searchText: ""
    property string selectedDeviceId: ""
    property var selectedDevice: null
    property string lastScanTime: ""

    property var groups: viewMode === "type"
        ? DeviceManager.typeGroups
        : DeviceManager.connectionGroups

    function clone(obj) { return JSON.parse(JSON.stringify(obj)) }

    function toggleCategory(key) {
        var e = root.expandedKeys
        e[key] = !(e[key] !== false)
        root.expandedKeys = clone(e)
    }

    function setAllExpanded(v) {
        var e = {}
        for (var i = 0; i < root.groups.length; i++)
            e[root.groups[i].key] = v
        root.expandedKeys = e
    }

    function findDevice(id) {
        for (var v = 0; v < 2; v++) {
            var gs = v === 0 ? DeviceManager.typeGroups : DeviceManager.connectionGroups
            for (var g = 0; g < gs.length; g++) {
                var devs = gs[g].devices
                for (var d = 0; d < devs.length; d++)
                    if (devs[d].id === id)
                        return devs[d]
            }
        }
        return null
    }

    function selectDevice(dev) {
        root.selectedDeviceId = dev.id
        root.selectedDevice = dev
    }

    function refresh() {
        DeviceManager.refresh()
        if (root.selectedDeviceId !== "")
            root.selectedDevice = root.findDevice(root.selectedDeviceId)
        root.lastScanTime = new Date().toLocaleTimeString(Qt.locale(), Locale.ShortFormat)
    }

    function openProperties() {
        if (root.selectedDevice !== null)
            propertiesDialog.open()
    }

    Component.onCompleted: root.lastScanTime = new Date().toLocaleTimeString(Qt.locale(), Locale.ShortFormat)

    // When the device list is re-enumerated (e.g. after a language switch or a
    // manual scan), re-resolve the selected device so the details pane shows the
    // freshly translated data instead of the stale pre-switch snapshot.
    Connections {
        target: DeviceManager
        function onGroupsChanged() {
            if (root.selectedDeviceId !== "")
                root.selectedDevice = root.findDevice(root.selectedDeviceId)
        }
    }


    ColumnLayout {
        anchors {
            fill: parent
            margins: 8
        }

        // ---- titlebar -----------------------------------------------------
        Item {
            Layout.fillWidth: true
            implicitHeight: windowControlsRow.implicitHeight

            Rectangle { // segmented view switcher
                anchors {
                    left: parent.left
                    verticalCenter: parent.verticalCenter
                }
                width: 172
                height: 35
                radius: Appearance.rounding.small
                color: Appearance.colors.colLayer1Hover

                RowLayout {
                    anchors.fill: parent
                    spacing: 4
                    RippleButton {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        buttonRadius: Appearance.rounding.small
                        toggled: root.viewMode === "type"
                        colBackgroundToggled: Appearance.colors.colSecondaryContainer
                        colBackgroundToggledHover: Appearance.colors.colSecondaryContainerHover
                        colBackgroundToggledActive: Appearance.colors.colSecondaryContainerActive
                        onClicked: root.viewMode = "type"
                        contentItem: StyledText {
                            text: Tr.t("byType", Tr.language)
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: Appearance.font.pixelSize.smallie
                            color: root.viewMode === "type"
                                ? Appearance.colors.colOnSecondaryContainer
                                : Appearance.colors.colOnLayer1
                        }
                    }
                    RippleButton {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        buttonRadius: Appearance.rounding.small
                        toggled: root.viewMode === "connection"
                        colBackgroundToggled: Appearance.colors.colSecondaryContainer
                        colBackgroundToggledHover: Appearance.colors.colSecondaryContainerHover
                        colBackgroundToggledActive: Appearance.colors.colSecondaryContainerActive
                        onClicked: root.viewMode = "connection"
                        contentItem: StyledText {
                            text: Tr.t("byConnection", Tr.language)
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: Appearance.font.pixelSize.smallie
                            color: root.viewMode === "connection"
                                ? Appearance.colors.colOnSecondaryContainer
                                : Appearance.colors.colOnLayer1
                        }
                    }
                }
            }

            RowLayout {
                id: windowControlsRow
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                spacing: 6

                RippleButton {
                    id: settingsButton
                    buttonRadius: Appearance.rounding.full
                    implicitWidth: 35
                    implicitHeight: 35
                    onClicked: settingsDialog.open()
                    contentItem: MaterialSymbol {
                        anchors.centerIn: parent
                        text: "settings"
                        iconSize: 20
                        color: Appearance.colors.colOnLayer0
                    }
                    StyledToolTip { text: Tr.t("settings", Tr.language) }
                }
                RippleButton {
                    buttonRadius: Appearance.rounding.full
                    implicitWidth: 35
                    implicitHeight: 35
                    onClicked: root.close()
                    contentItem: MaterialSymbol {
                        anchors.centerIn: parent
                        text: "close"
                        iconSize: 20
                        color: Appearance.colors.colOnLayer0
                    }
                    StyledToolTip { text: Tr.t("close", Tr.language) }
                }
            }
        }

        // ---- toolbar ------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            RippleButton {
                buttonRadius: Appearance.rounding.small
                implicitWidth: 40
                implicitHeight: 38
                Layout.alignment: Qt.AlignVCenter
                onClicked: root.refresh()
                contentItem: MaterialSymbol {
                    anchors.centerIn: parent
                    text: "refresh"
                    iconSize: 20
                    color: Appearance.colors.colOnLayer0
                }
                StyledToolTip { text: Tr.t("scanForChanges", Tr.language) }
            }


            RippleButton {
                buttonRadius: Appearance.rounding.small
                implicitWidth: 36
                implicitHeight: 38
                Layout.alignment: Qt.AlignVCenter
                onClicked: root.setAllExpanded(true)
                contentItem: MaterialSymbol {
                    anchors.centerIn: parent
                    text: "unfold_more"
                    iconSize: 20
                    color: Appearance.colors.colOnLayer0
                }
                StyledToolTip { text: Tr.t("expandAll", Tr.language) }
            }
            RippleButton {
                buttonRadius: Appearance.rounding.small
                implicitWidth: 36
                implicitHeight: 38
                Layout.alignment: Qt.AlignVCenter
                onClicked: root.setAllExpanded(false)
                contentItem: MaterialSymbol {
                    anchors.centerIn: parent
                    text: "unfold_less"
                    iconSize: 20
                    color: Appearance.colors.colOnLayer0
                }
                StyledToolTip { text: Tr.t("collapseAll", Tr.language) }
            }

            Item { Layout.fillWidth: true }

            MaterialTextField {
                id: searchField
                Layout.preferredWidth: 230
                Layout.alignment: Qt.AlignVCenter
                placeholderText: Tr.t("searchDevices", Tr.language)
                onTextChanged: root.searchText = text
                Keys.onEscapePressed: {
                    clear()
                    root.searchText = ""
                    focus = false
                }
                leftPadding: 34
                MaterialSymbol {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 8
                    text: "search"
                    iconSize: 18
                    color: Appearance.colors.colOnLayer1Inactive
                }
                RippleButton {
                    visible: searchField.text !== ""
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 4
                    buttonRadius: Appearance.rounding.full
                    implicitWidth: 26
                    implicitHeight: 26
                    onClicked: {
                        searchField.clear()
                        root.searchText = ""
                        searchField.focus = true
                    }
                    contentItem: MaterialSymbol {
                        anchors.centerIn: parent
                        text: "close"
                        iconSize: 14
                        color: Appearance.colors.colOnLayer1Inactive
                    }
                }
            }
        }

        // ---- content ------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            // device tree
            StyledRectangle {
                contentLayer: StyledRectangle.ContentLayer.Pane
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: Appearance.rounding.windowRounding - 8
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    DeviceList {
                        id: deviceList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        focus: true
                        groups: root.groups
                        searchText: root.searchText
                        expandedKeys: root.expandedKeys
                        selectedDeviceId: root.selectedDeviceId
                        onDeviceClicked: (dev) => root.selectDevice(dev)
                        onDeviceAltClicked: (dev) => {
                            root.selectDevice(dev)
                            root.openProperties()
                        }
                        onCategoryToggled: (key) => root.toggleCategory(key)
                        Keys.onUpPressed: deviceList.moveSelection(-1)
                        Keys.onDownPressed: deviceList.moveSelection(1)
                        Keys.onReturnPressed: root.openProperties()
                    }
                }
            }

            // details
            StyledRectangle {
                contentLayer: StyledRectangle.ContentLayer.Pane
                Layout.preferredWidth: 470
                Layout.minimumWidth: 340
                Layout.fillHeight: true
                radius: Appearance.rounding.windowRounding - 8
                clip: true

                DetailsPane {
                    anchors.fill: parent
                    device: root.selectedDevice
                    onPropertiesRequested: root.openProperties()
                    onRefreshRequested: root.refresh()
                }
            }
        }

        // ---- status bar ---------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 2
            spacing: 8

            StyledText {
                text: Tr.t("deviceSummaryFormat", Tr.language)
                    .replace("%1", DeviceManager.deviceCount)
                    .replace("%2", DeviceManager.deviceCount - DeviceManager.problemCount)
                    .replace("%3", DeviceManager.problemCount)
                font.pixelSize: Appearance.font.pixelSize.smallest
                color: Appearance.colors.colOnLayer1
            }
            Item { Layout.fillWidth: true }
            StyledText {
                text: Tr.t("viewFormat", Tr.language).arg(root.viewMode === "type"
                    ? Tr.t("byType", Tr.language) : Tr.t("byConnection", Tr.language))
                font.pixelSize: Appearance.font.pixelSize.smallest
                color: Appearance.colors.colOnLayer1
            }
            StyledText {
                text: Tr.t("lastScanFormat", Tr.language).arg(root.lastScanTime)
                font.pixelSize: Appearance.font.pixelSize.smallest
                color: Appearance.colors.colOnLayer1
            }
        }
    }

    // ---- properties dialog ------------------------------------------------
    PropertiesDialog {
        id: propertiesDialog
        device: root.selectedDevice
        onRefreshRequested: root.refresh()
    }

    // ---- settings dialog ---------------------------------------------------
    SettingsDialog { id: settingsDialog }
}
