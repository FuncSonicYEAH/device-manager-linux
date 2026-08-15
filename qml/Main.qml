// 设备管理器 — a Windows Device Manager style hardware browser built on the
// m3-gallery Material 3 widget set (Qt6 Quick + C++ + Meson).
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Qt.labs.settings
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
    // device the pending/executing power action refers to
    property var actionDevice: null

    property var groups: viewMode === "type"
        ? DeviceManager.typeGroups
        : DeviceManager.connectionGroups

    // width of the details pane; adjustable by dragging the divider
    property real detailsPaneWidth: 470

    // persisted across runs via QSettings (same store as theme/language)
    Settings {
        id: appSettings
        property real detailsPaneWidth: 470
    }

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

    function openSmart() {
        if (root.selectedDevice !== null)
            smartDialog.open()
    }

    function openGraphics() {
        if (root.selectedDevice !== null)
            graphicsDialog.open()
    }

    // Build the right-click menu entries for a device.
    function buildMenuItems(dev) {
        var items = []
        items.push({ icon: "info", text: Tr.t("properties", Tr.language),
                     enabled: true, action: "properties" })
        if (dev.category === "disk")
            items.push({ icon: "monitor_heart", text: Tr.t("smartHealthCheck", Tr.language),
                         enabled: true, action: "smart" })
        if (dev.category === "display")
            items.push({ icon: "view_in_ar", text: Tr.t("graphicsSupportTitle", Tr.language),
                         enabled: true, action: "graphics" })
        items.push({ icon: "", text: "", enabled: false, action: "separator" })
        items.push({ icon: "pause_circle", text: Tr.t("suspendDevice", Tr.language),
                     enabled: DeviceActions.supportsAction(dev, "suspend"), action: "suspend" })
        items.push({ icon: "play_circle", text: Tr.t("enableDevice", Tr.language),
                     enabled: DeviceActions.supportsAction(dev, "enable"), action: "enable" })
        items.push({ icon: "power_settings_new", text: Tr.t("startDevice", Tr.language),
                     enabled: DeviceActions.supportsAction(dev, "start"), action: "start" })
        return items
    }

    function showContextMenu(dev, x, y) {
        root.selectDevice(dev)
        contextMenu.menuItems = root.buildMenuItems(dev)
        var mx = Math.max(8, Math.min(x, root.width - contextMenu.width - 8))
        var my = Math.max(8, Math.min(y, root.height - contextMenu.height - 8))
        contextMenu.x = mx
        contextMenu.y = my
        contextMenu.open()
    }

    function onContextItem(index) {
        var items = contextMenu.menuItems
        if (index < 0 || index >= items.length)
            return
        var action = items[index].action
        if (action === "smart") {
            root.openSmart()
        } else if (action === "graphics") {
            root.openGraphics()
        } else if (action === "suspend" || action === "enable" || action === "start") {
            root.actionDevice = root.selectedDevice
            actionWarningDialog.action = action
            actionWarningDialog.open()
        } else if (action === "properties") {
            root.openProperties()
        }
    }

    Component.onCompleted: {
        if (appSettings.detailsPaneWidth > 0)
            root.detailsPaneWidth = appSettings.detailsPaneWidth
        root.lastScanTime = new Date().toLocaleTimeString(Qt.locale(), Locale.ShortFormat)
    }

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
                    buttonRadius: Appearance.rounding.full
                    implicitWidth: 35
                    implicitHeight: 35
                    onClicked: aboutDialog.open()
                    contentItem: MaterialSymbol {
                        anchors.centerIn: parent
                        text: "info"
                        iconSize: 20
                        color: Appearance.colors.colOnLayer0
                    }
                    StyledToolTip { text: Tr.t("aboutApp", Tr.language) }
                }
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
            id: contentRow
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            // device tree
            StyledRectangle {
                contentLayer: StyledRectangle.ContentLayer.Pane
                Layout.fillWidth: true
                Layout.minimumWidth: 260
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
                        onDeviceContextRequested: (dev, x, y) => root.showContextMenu(dev, x, y)
                        onCategoryToggled: (key) => root.toggleCategory(key)
                        Keys.onUpPressed: deviceList.moveSelection(-1)
                        Keys.onDownPressed: deviceList.moveSelection(1)
                        Keys.onReturnPressed: root.openProperties()
                    }
                }
            }

            // draggable divider between the tree and the details pane
            Item {
                id: splitter
                Layout.fillHeight: true
                Layout.preferredWidth: 12

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    width: 2
                    height: parent.height - 28
                    radius: 1
                    color: splitterMouse.pressed || splitterMouse.containsMouse
                        ? Appearance.colors.colOutline
                        : Appearance.colors.colOutlineVariant
                }

                MouseArea {
                    id: splitterMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.SizeHorCursor
                    onPositionChanged: {
                        if (!pressed)
                            return
                        // map into the content row so the boundary tracks the
                        // cursor exactly even while the divider itself moves
                        var pos = splitterMouse.mapToItem(contentRow, mouse.x, mouse.y)
                        var maxWidth = Math.max(340, contentRow.width - 320)
                        root.detailsPaneWidth = Math.max(340, Math.min(maxWidth,
                            contentRow.width - pos.x - splitter.width / 2 - contentRow.spacing))
                    }
                    onReleased: appSettings.detailsPaneWidth = root.detailsPaneWidth
                }
            }

            // details
            StyledRectangle {
                contentLayer: StyledRectangle.ContentLayer.Pane
                Layout.preferredWidth: root.detailsPaneWidth
                Layout.minimumWidth: 340
                Layout.fillHeight: true
                radius: Appearance.rounding.windowRounding - 8
                clip: true

                DetailsPane {
                    anchors.fill: parent
                    device: root.selectedDevice
                    onPropertiesRequested: root.openProperties()
                    onRefreshRequested: root.refresh()
                    onSmartRequested: root.openSmart()
                    onGraphicsRequested: root.openGraphics()
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

    // ---- context menu (right-click on a device) ----------------------------
    ContextMenu {
        id: contextMenu
        onItemActivated: (index) => root.onContextItem(index)
    }

    // ---- properties dialog ------------------------------------------------
    PropertiesDialog {
        id: propertiesDialog
        device: root.selectedDevice
        onRefreshRequested: root.refresh()
    }

    // ---- SMART health check dialog -----------------------------------------
    SmartDialog {
        id: smartDialog
        device: root.selectedDevice
    }

    // ---- device action warning dialog --------------------------------------
    ActionWarningDialog {
        id: actionWarningDialog
        device: root.actionDevice
        onActionCompleted: (deviceId) => root.refresh()
    }

    // ---- graphics support (OpenGL / Vulkan) dialog --------------------------
    GraphicsDialog { id: graphicsDialog }

    // ---- about dialog -------------------------------------------------------
    AboutDialog { id: aboutDialog }

    // ---- settings dialog ---------------------------------------------------
    SettingsDialog { id: settingsDialog }
}
