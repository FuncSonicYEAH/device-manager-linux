// Driver detection & installation dialog. Three tabs:
//  - "missing drivers": pci/usb devices with no driver bound; candidate
//    kernel modules from modules.alias can be loaded, and distro driver
//    packages can be searched and installed (PackageInstallDialog).
//  - "kernel modules": loaded modules from /proc/modules with modinfo
//    details and an unload action.
//  - "proprietary drivers": closed-source driver options for the detected
//    hardware (NVIDIA, Broadcom wl), with per-distro install flows such as
//    RPM Fusion enablement, ubuntu-drivers, or yay on Arch.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

AnimatedDialog {
    id: root

    property int currentTab: 0
    property string expandedDeviceId: ""
    property string expandedModule: ""
    property string confirmUnloadModule: ""
    property var pkgResults: ({})     // deviceId -> [{name, summary, recommended, installed}]
    property var searchingIds: ({})   // deviceId -> bool
    property var moduleInfos: ({})    // module name -> modinfo map
    property var proprietaryOptions: []   // closed-source driver candidates
    property string moduleFilter: ""
    property var pendingProbeDevice: null
    property string banner: ""
    property bool bannerOk: true

    // the main window re-enumerates after successful actions
    signal requestRefresh()

    width: Math.min(800, parent.width * 0.92)
    height: Math.min(620, parent.height * 0.92)

    function rescan() {
        Drivers.scan()
        Drivers.scanProprietary()
    }

    function showBanner(ok, message) {
        root.bannerOk = ok
        root.banner = message
    }

    function deviceMap(model) {
        return { id: model.id, sysfsPath: model.sysfsPath, bus: model.bus,
                 modalias: model.modalias, name: model.name, vendor: model.vendor }
    }

    function moduleStateText(state) {
        return state === "builtin" ? Tr.t("moduleStateBuiltin", Tr.language)
            : state === "loaded" ? Tr.t("moduleStateLoaded", Tr.language)
            : Tr.t("moduleStateAvailable", Tr.language)
    }

    function moduleInfoProps(mod) {
        var info = root.moduleInfos[mod]
        if (!info)
            return []
        if (info.error !== undefined)
            return [{ name: Tr.t("status", Tr.language), value: info.error }]
        var defs = [
            ["description", Tr.t("moduleDescription", Tr.language)],
            ["version", Tr.t("moduleVersion", Tr.language)],
            ["author", Tr.t("moduleAuthor", Tr.language)],
            ["license", Tr.t("moduleLicense", Tr.language)],
            ["firmware", Tr.t("moduleFirmware", Tr.language)],
            ["depends", Tr.t("moduleDepends", Tr.language)],
            ["vermagic", Tr.t("moduleVermagic", Tr.language)],
            ["filename", Tr.t("moduleFilename", Tr.language)]
        ]
        var props = []
        for (var i = 0; i < defs.length; i++) {
            var v = info[defs[i][0]]
            if (v !== undefined && v !== "" && v !== null)
                props.push({ name: defs[i][1], value: String(v) })
        }
        if (info.aliasCount !== undefined && info.aliasCount > 0)
            props.push({ name: Tr.t("moduleAliasCount", Tr.language),
                         value: info.aliasCount })
        return props
    }

    property var filteredModules: {
        var list = Drivers.loadedModules
        var f = root.moduleFilter.toLowerCase()
        var out = []
        for (var i = 0; i < list.length; i++)
            if (f === "" || list[i].name.toLowerCase().indexOf(f) >= 0)
                out.push(list[i])
        return out
    }

    onOpened: root.rescan()

    Connections {
        target: Drivers
        function onActionFinished(targetId, action, ok, message) {
            if (ok) {
                root.showBanner(true, Tr.t("actionSucceeded", Tr.language))
                root.rescan()
                root.requestRefresh()
            } else {
                root.showBanner(false, message !== "" ? message
                                                      : Tr.t("actionFailed", Tr.language))
            }
        }
        function onModuleInfoReady(module, info) {
            var m = Object.assign({}, root.moduleInfos)
            m[module] = info
            root.moduleInfos = m
        }
        function onSearchReady(deviceId, results) {
            var s = Object.assign({}, root.searchingIds)
            s[deviceId] = false
            root.searchingIds = s
            var r = Object.assign({}, root.pkgResults)
            r[deviceId] = results
            root.pkgResults = r
        }
        function onSearchFailed(deviceId, message) {
            var s = Object.assign({}, root.searchingIds)
            s[deviceId] = false
            root.searchingIds = s
            root.showBanner(false, message)
        }
        function onProprietaryReady(options) {
            root.proprietaryOptions = options
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        // ---- header -------------------------------------------------------
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
                    text: "memory"
                    iconSize: 24
                    color: Appearance.colors.colOnSecondaryContainer
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                StyledText {
                    Layout.fillWidth: true
                    text: Tr.t("driversTitle", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.normal
                    font.weight: Font.Medium
                    color: Appearance.colors.colOnLayer1
                    elide: Text.ElideRight
                }
                StyledText {
                    Layout.fillWidth: true
                    text: Tr.t("kernelFormat", Tr.language).arg(Drivers.kernelRelease)
                        + (Drivers.packageBackendName !== ""
                            ? "  ·  " + Tr.t("packageBackendFormat", Tr.language)
                                  .arg(Drivers.packageBackendName)
                            : "")
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1
                    elide: Text.ElideRight
                }
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

        // ---- tabs ------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 12
            spacing: 6

            RippleButton {
                buttonText: Tr.t("driversMissingTab", Tr.language)
                    + " (" + Drivers.missingDrivers.length + ")"
                buttonRadius: Appearance.rounding.full
                toggled: root.currentTab === 0
                colBackgroundToggled: Appearance.colors.colSecondaryContainer
                colBackgroundToggledHover: Appearance.colors.colSecondaryContainerHover
                colBackgroundToggledActive: Appearance.colors.colSecondaryContainerActive
                onClicked: root.currentTab = 0
            }
            RippleButton {
                buttonText: Tr.t("driversModulesTab", Tr.language)
                    + " (" + Drivers.loadedModules.length + ")"
                buttonRadius: Appearance.rounding.full
                toggled: root.currentTab === 1
                colBackgroundToggled: Appearance.colors.colSecondaryContainer
                colBackgroundToggledHover: Appearance.colors.colSecondaryContainerHover
                colBackgroundToggledActive: Appearance.colors.colSecondaryContainerActive
                onClicked: root.currentTab = 1
            }
            RippleButton {
                buttonText: Tr.t("proprietaryTab", Tr.language)
                    + " (" + root.proprietaryOptions.length + ")"
                buttonRadius: Appearance.rounding.full
                toggled: root.currentTab === 2
                colBackgroundToggled: Appearance.colors.colSecondaryContainer
                colBackgroundToggledHover: Appearance.colors.colSecondaryContainerHover
                colBackgroundToggledActive: Appearance.colors.colSecondaryContainerActive
                onClicked: root.currentTab = 2
            }
            Item { Layout.fillWidth: true }
        }

        // ---- body ----------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 12
            clip: true

            // == tab 0: missing drivers =====================================
            ListView {
                id: missingList
                anchors.fill: parent
                visible: root.currentTab === 0
                clip: true
                spacing: 8
                model: Drivers.missingDrivers
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                // all-good empty state
                ColumnLayout {
                    anchors.centerIn: parent
                    visible: root.currentTab === 0 && Drivers.missingDrivers.length === 0
                    spacing: 10
                    MaterialSymbol {
                        Layout.alignment: Qt.AlignHCenter
                        text: "verified"
                        iconSize: 46
                        color: Appearance.m3colors.m3success
                    }
                    StyledText {
                        Layout.alignment: Qt.AlignHCenter
                        text: Tr.t("noMissingDrivers", Tr.language)
                        font.pixelSize: Appearance.font.pixelSize.smallie
                        color: Appearance.colors.colOnLayer1
                    }
                    StyledText {
                        Layout.alignment: Qt.AlignHCenter
                        visible: Drivers.lastError !== ""
                        text: Drivers.lastError
                        font.pixelSize: Appearance.font.pixelSize.smallest
                        color: Appearance.m3colors.m3error
                    }
                }

                delegate: Rectangle {
                    width: missingList.width
                    height: deviceColumn.implicitHeight
                    radius: Appearance.rounding.small
                    color: Appearance.colors.colSurfaceContainerHighest
                    border.width: 1
                    border.color: root.expandedDeviceId === modelData.id
                        ? Appearance.colors.colOutline
                        : Appearance.colors.colOutlineVariant

                    readonly property var dev: root.deviceMap(modelData)
                    readonly property bool expanded: root.expandedDeviceId === modelData.id
                    readonly property var pkgs: root.pkgResults[modelData.id] !== undefined
                        ? root.pkgResults[modelData.id] : null

                    ColumnLayout {
                        id: deviceColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: 0

                        // summary row (click to expand)
                        RippleButton {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            buttonRadius: Appearance.rounding.small
                            onClicked: root.expandedDeviceId = expanded ? "" : modelData.id
                            contentItem: RowLayout {
                                spacing: 10

                                MaterialSymbol {
                                    text: modelData.bus === "usb" ? "usb" : "developer_board"
                                    iconSize: 20
                                    color: Appearance.colors.colOnLayer1
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    StyledText {
                                        Layout.fillWidth: true
                                        text: modelData.name
                                        font.pixelSize: Appearance.font.pixelSize.smallie
                                        font.weight: Font.Medium
                                        color: Appearance.colors.colOnLayer1
                                        elide: Text.ElideRight
                                    }
                                    StyledText {
                                        Layout.fillWidth: true
                                        text: (modelData.vendor !== "" ? modelData.vendor + "  ·  " : "")
                                            + modelData.bus.toUpperCase() + "  ·  " + modelData.modalias
                                        font.pixelSize: Appearance.font.pixelSize.smallest
                                        color: Appearance.colors.colOnSurfaceVariant
                                        elide: Text.ElideMiddle
                                    }
                                }
                                MaterialSymbol {
                                    text: "expand_more"
                                    iconSize: 20
                                    color: Appearance.colors.colOnLayer1
                                    rotation: expanded ? 180 : 0
                                    Behavior on rotation {
                                        NumberAnimation { duration: 150 }
                                    }
                                }
                            }
                        }

                        // expanded details
                        ColumnLayout {
                            visible: expanded
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Layout.bottomMargin: 12
                            spacing: 10

                            // candidate kernel modules
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                StyledText {
                                    text: Tr.t("candidateDrivers", Tr.language)
                                    font.pixelSize: Appearance.font.pixelSize.smallest
                                    font.weight: Font.Medium
                                    color: Appearance.colors.colOnSurfaceVariant
                                }
                                Repeater {
                                    model: modelData.candidates
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8
                                        StyledText {
                                            text: modelData.module
                                            font.pixelSize: Appearance.font.pixelSize.smallest
                                            font.family: "monospace"
                                            color: Appearance.colors.colOnLayer1
                                        }
                                        StyledText {
                                            text: root.moduleStateText(modelData.state)
                                            font.pixelSize: Appearance.font.pixelSize.smallest
                                            color: modelData.state === "loaded"
                                                ? Appearance.m3colors.m3success
                                                : modelData.state === "builtin"
                                                    ? Appearance.colors.colOnSurfaceVariant
                                                    : Appearance.m3colors.m3tertiary
                                        }
                                        Item { Layout.fillWidth: true }
                                        RippleButton {
                                            visible: modelData.state === "available"
                                            enabled: !Drivers.busy
                                            buttonText: Tr.t("loadModuleBtn", Tr.language)
                                            buttonRadius: Appearance.rounding.full
                                            onClicked: Drivers.loadModule(modelData.module)
                                        }
                                    }
                                }
                                StyledText {
                                    visible: modelData.candidates.length === 0
                                    Layout.fillWidth: true
                                    text: Tr.t("noCandidatesHint", Tr.language)
                                    font.pixelSize: Appearance.font.pixelSize.smallest
                                    color: Appearance.colors.colOnSurfaceVariant
                                    wrapMode: Text.Wrap
                                }
                            }

                            // distro packages
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                StyledText {
                                    text: Tr.t("distroPackages", Tr.language)
                                    font.pixelSize: Appearance.font.pixelSize.smallest
                                    font.weight: Font.Medium
                                    color: Appearance.colors.colOnSurfaceVariant
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    RippleButton {
                                        enabled: !root.searchingIds[modelData.id]
                                            && Drivers.packageBackend !== "unknown"
                                        buttonText: Tr.t("searchPackagesBtn", Tr.language)
                                        buttonRadius: Appearance.rounding.small
                                        onClicked: {
                                            var s = Object.assign({}, root.searchingIds)
                                            s[modelData.id] = true
                                            root.searchingIds = s
                                            Drivers.searchPackages(dev)
                                        }
                                    }
                                    CircularProgress {
                                        visible: root.searchingIds[modelData.id] === true
                                        implicitSize: 16
                                        value: searchAnim.value
                                    }
                                    StyledText {
                                        visible: Drivers.packageBackend === "unknown"
                                        Layout.fillWidth: true
                                        text: Tr.t("noPackageManager", Tr.language)
                                        font.pixelSize: Appearance.font.pixelSize.smallest
                                        color: Appearance.colors.colOnSurfaceVariant
                                        wrapMode: Text.Wrap
                                    }
                                }
                                Repeater {
                                    model: pkgs !== null ? pkgs : []
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 1
                                            RowLayout {
                                                spacing: 6
                                                StyledText {
                                                    text: modelData.name
                                                    font.pixelSize: Appearance.font.pixelSize.smallest
                                                    font.family: "monospace"
                                                    color: Appearance.colors.colOnLayer1
                                                    elide: Text.ElideMiddle
                                                    Layout.fillWidth: true
                                                }
                                                Rectangle {
                                                    visible: modelData.recommended
                                                    radius: 8
                                                    height: 16
                                                    width: recLabel.implicitWidth + 12
                                                    color: Appearance.m3colors.m3tertiaryContainer
                                                    StyledText {
                                                        id: recLabel
                                                        anchors.centerIn: parent
                                                        text: Tr.t("packageRecommended", Tr.language)
                                                        font.pixelSize: 12
                                                        color: Appearance.m3colors.m3onTertiaryContainer
                                                    }
                                                }
                                                Rectangle {
                                                    visible: modelData.installed
                                                    radius: 8
                                                    height: 16
                                                    width: instLabel.implicitWidth + 12
                                                    color: Appearance.m3colors.m3successContainer
                                                    StyledText {
                                                        id: instLabel
                                                        anchors.centerIn: parent
                                                        text: Tr.t("packageInstalled", Tr.language)
                                                        font.pixelSize: 12
                                                        color: Appearance.m3colors.m3onSuccessContainer
                                                    }
                                                }
                                            }
                                            StyledText {
                                                visible: modelData.summary !== ""
                                                Layout.fillWidth: true
                                                text: modelData.summary
                                                font.pixelSize: Appearance.font.pixelSize.smallest
                                                color: Appearance.colors.colOnSurfaceVariant
                                                elide: Text.ElideRight
                                            }
                                        }
                                        RippleButton {
                                            visible: !modelData.installed
                                            enabled: !Drivers.busy
                                            buttonText: Tr.t("installPackageBtn", Tr.language)
                                            buttonRadius: Appearance.rounding.full
                                            colBackground: Appearance.m3colors.m3tertiaryContainer
                                            colBackgroundHover: Appearance.m3colors.m3tertiaryContainer
                                            colBackgroundActive: Appearance.m3colors.m3tertiaryContainer
                                            contentItem: StyledText {
                                                text: Tr.t("installPackageBtn", Tr.language)
                                                horizontalAlignment: Text.AlignHCenter
                                                font.pixelSize: Appearance.font.pixelSize.smallie
                                                color: Appearance.m3colors.m3onTertiaryContainer
                                            }
                                            onClicked: {
                                                root.pendingProbeDevice = dev
                                                installDialog.proprietary = false
                                                installDialog.pkgName = modelData.name
                                                installDialog.deviceId = dev.id
                                                installDialog.deviceName = dev.name
                                                installDialog.open()
                                            }
                                        }
                                    }
                                }
                                StyledText {
                                    visible: pkgs !== null && pkgs.length === 0
                                    Layout.fillWidth: true
                                    text: Tr.t("noPackageResults", Tr.language)
                                    font.pixelSize: Appearance.font.pixelSize.smallest
                                    color: Appearance.colors.colOnSurfaceVariant
                                    wrapMode: Text.Wrap
                                }
                            }

                            // re-probe
                            RowLayout {
                                Layout.fillWidth: true
                                RippleButton {
                                    enabled: !Drivers.busy
                                    buttonText: Tr.t("probeDeviceBtn", Tr.language)
                                    buttonRadius: Appearance.rounding.small
                                    onClicked: Drivers.probeDevice(dev)
                                }
                                Item { Layout.fillWidth: true }
                            }

                            SequentialAnimation {
                                id: searchAnim
                                property real value: 0.05
                                running: root.searchingIds[modelData.id] === true && root.visible
                                loops: Animation.Infinite
                                NumberAnimation {
                                    target: searchAnim
                                    property: "value"
                                    from: 0.05
                                    to: 0.95
                                    duration: 1000
                                    easing.type: Easing.InOutQuad
                                }
                                NumberAnimation {
                                    target: searchAnim
                                    property: "value"
                                    from: 0.95
                                    to: 0.05
                                    duration: 1000
                                    easing.type: Easing.InOutQuad
                                }
                            }
                        }
                    }
                }
            }

            // == tab 1: kernel modules =======================================
            ColumnLayout {
                anchors.fill: parent
                visible: root.currentTab === 1
                spacing: 8

                MaterialTextField {
                    Layout.fillWidth: true
                    placeholderText: Tr.t("moduleSearchPlaceholder", Tr.language)
                    onTextChanged: root.moduleFilter = text
                    leftPadding: 34
                    MaterialSymbol {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 8
                        text: "search"
                        iconSize: 18
                        color: Appearance.colors.colOnLayer1Inactive
                    }
                }

                ListView {
                    id: moduleList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 4
                    model: root.filteredModules
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: ColumnLayout {
                        width: moduleList.width
                        spacing: 0

                        readonly property bool modExpanded: root.expandedModule === modelData.name

                        RippleButton {
                            Layout.fillWidth: true
                            buttonRadius: Appearance.rounding.small
                            onClicked: {
                                if (modExpanded) {
                                    root.expandedModule = ""
                                } else {
                                    root.expandedModule = modelData.name
                                    root.confirmUnloadModule = ""
                                    if (root.moduleInfos[modelData.name] === undefined)
                                        Drivers.moduleInfo(modelData.name)
                                }
                            }
                            contentItem: RowLayout {
                                spacing: 8
                                StyledText {
                                    Layout.fillWidth: true
                                    text: modelData.name
                                    font.pixelSize: Appearance.font.pixelSize.smallest
                                    font.family: "monospace"
                                    color: Appearance.colors.colOnLayer1
                                    elide: Text.ElideMiddle
                                }
                                StyledText {
                                    text: modelData.size
                                    font.pixelSize: Appearance.font.pixelSize.smallest
                                    color: Appearance.colors.colOnSurfaceVariant
                                }
                                StyledText {
                                    visible: modelData.usedCount > 0 || modelData.usedBy !== ""
                                    text: Tr.t("moduleUsedByFormat", Tr.language)
                                        .arg(modelData.usedCount)
                                        .arg(modelData.usedBy !== "" ? modelData.usedBy : "—")
                                    font.pixelSize: Appearance.font.pixelSize.smallest
                                    color: Appearance.colors.colOnSurfaceVariant
                                    elide: Text.ElideMiddle
                                    Layout.maximumWidth: 180
                                }
                            }
                        }

                        // expanded modinfo
                        Rectangle {
                            visible: modExpanded
                            Layout.fillWidth: true
                            implicitHeight: modColumn.implicitHeight + 16
                            radius: Appearance.rounding.small
                            color: Appearance.colors.colSurfaceContainerHighest

                            ColumnLayout {
                                id: modColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 8
                                spacing: 6

                                PropsList {
                                    Layout.fillWidth: true
                                    props: root.moduleInfoProps(modelData.name)
                                    showDividers: false
                                    nameWidth: 110
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    visible: root.confirmUnloadModule !== modelData.name

                                    StyledText {
                                        Layout.fillWidth: true
                                        visible: root.moduleInfos[modelData.name] === undefined
                                        text: "…"
                                        font.pixelSize: Appearance.font.pixelSize.smallest
                                        color: Appearance.colors.colOnSurfaceVariant
                                    }
                                    Item { Layout.fillWidth: true }
                                    RippleButton {
                                        enabled: !Drivers.busy
                                        buttonText: Tr.t("unloadModuleBtn", Tr.language)
                                        buttonRadius: Appearance.rounding.small
                                        onClicked: root.confirmUnloadModule = modelData.name
                                    }
                                }

                                // inline unload confirmation
                                ColumnLayout {
                                    visible: root.confirmUnloadModule === modelData.name
                                    Layout.fillWidth: true
                                    spacing: 6
                                    StyledText {
                                        Layout.fillWidth: true
                                        text: Tr.t("unloadConfirmText", Tr.language)
                                        font.pixelSize: Appearance.font.pixelSize.smallest
                                        color: Appearance.colors.colOnLayer1
                                        wrapMode: Text.Wrap
                                    }
                                    RowLayout {
                                        spacing: 8
                                        Item { Layout.fillWidth: true }
                                        RippleButton {
                                            buttonText: Tr.t("cancel", Tr.language)
                                            buttonRadius: Appearance.rounding.small
                                            onClicked: root.confirmUnloadModule = ""
                                        }
                                        RippleButton {
                                            enabled: !Drivers.busy
                                            buttonText: Tr.t("confirm", Tr.language)
                                            buttonRadius: Appearance.rounding.small
                                            colBackground: Appearance.m3colors.m3errorContainer
                                            colBackgroundHover: Appearance.m3colors.m3errorContainer
                                            colBackgroundActive: Appearance.m3colors.m3errorContainer
                                            contentItem: StyledText {
                                                text: Tr.t("confirm", Tr.language)
                                                horizontalAlignment: Text.AlignHCenter
                                                font.pixelSize: Appearance.font.pixelSize.smallie
                                                color: Appearance.m3colors.m3onErrorContainer
                                            }
                                            onClicked: {
                                                root.confirmUnloadModule = ""
                                                Drivers.unloadModule(modelData.name)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // == tab 2: proprietary (closed-source) drivers ==================
            ListView {
                id: proprietaryList
                anchors.fill: parent
                visible: root.currentTab === 2
                clip: true
                spacing: 8
                model: root.proprietaryOptions
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                // nothing to offer
                ColumnLayout {
                    anchors.centerIn: parent
                    visible: root.currentTab === 2 && root.proprietaryOptions.length === 0
                    spacing: 10
                    MaterialSymbol {
                        Layout.alignment: Qt.AlignHCenter
                        text: "extension_off"
                        iconSize: 46
                        color: Appearance.colors.colOnSurfaceVariant
                    }
                    StyledText {
                        Layout.alignment: Qt.AlignHCenter
                        text: Tr.t("noProprietaryOptions", Tr.language)
                        font.pixelSize: Appearance.font.pixelSize.smallie
                        color: Appearance.colors.colOnLayer1
                    }
                }

                delegate: Rectangle {
                    width: proprietaryList.width
                    height: propColumn.implicitHeight
                    radius: Appearance.rounding.small
                    color: Appearance.colors.colSurfaceContainerHighest
                    border.width: 1
                    border.color: Appearance.colors.colOutlineVariant

                    ColumnLayout {
                        id: propColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: 0

                        // header row
                        RippleButton {
                            Layout.fillWidth: true
                            buttonRadius: Appearance.rounding.small
                            enabled: false
                            contentItem: RowLayout {
                                spacing: 10
                                MaterialSymbol {
                                    text: modelData.key === "nvidia" ? "monitor" : "wifi"
                                    iconSize: 20
                                    color: Appearance.colors.colOnLayer1
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    StyledText {
                                        Layout.fillWidth: true
                                        text: modelData.key === "nvidia"
                                            ? Tr.t("nvidiaProprietaryName", Tr.language)
                                            : Tr.t("broadcomProprietaryName", Tr.language)
                                        font.pixelSize: Appearance.font.pixelSize.smallie
                                        font.weight: Font.Medium
                                        color: Appearance.colors.colOnLayer1
                                        elide: Text.ElideRight
                                    }
                                    StyledText {
                                        Layout.fillWidth: true
                                        text: modelData.device
                                            + (modelData.currentDriver !== ""
                                                ? "  ·  " + Tr.t("currentDriverFormat", Tr.language)
                                                      .arg(modelData.currentDriver)
                                                : "")
                                        font.pixelSize: Appearance.font.pixelSize.smallest
                                        color: Appearance.colors.colOnSurfaceVariant
                                        elide: Text.ElideMiddle
                                    }
                                }
                                Rectangle {
                                    visible: modelData.installed
                                    radius: 9
                                    height: 18
                                    width: propInstLabel.implicitWidth + 14
                                    color: Appearance.m3colors.m3successContainer
                                    StyledText {
                                        id: propInstLabel
                                        anchors.centerIn: parent
                                        text: Tr.t("packageInstalled", Tr.language)
                                        font.pixelSize: 12
                                        color: Appearance.m3colors.m3onSuccessContainer
                                    }
                                }
                            }
                        }

                        // body: package + install + notes
                        ColumnLayout {
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Layout.bottomMargin: 12
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                StyledText {
                                    text: Tr.t("packageFormat", Tr.language)
                                        .arg(modelData.package)
                                    font.pixelSize: Appearance.font.pixelSize.smallest
                                    font.family: "monospace"
                                    color: Appearance.colors.colOnLayer1
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                                RippleButton {
                                    visible: !modelData.installed
                                    enabled: !Drivers.busy
                                    buttonText: Tr.t("installPackageBtn", Tr.language)
                                    buttonRadius: Appearance.rounding.full
                                    colBackground: Appearance.m3colors.m3tertiaryContainer
                                    colBackgroundHover: Appearance.m3colors.m3tertiaryContainer
                                    colBackgroundActive: Appearance.m3colors.m3tertiaryContainer
                                    contentItem: StyledText {
                                        text: Tr.t("installPackageBtn", Tr.language)
                                        horizontalAlignment: Text.AlignHCenter
                                        font.pixelSize: Appearance.font.pixelSize.smallie
                                        color: Appearance.m3colors.m3onTertiaryContainer
                                    }
                                    onClicked: {
                                        installDialog.proprietary = true
                                        installDialog.pkgName = modelData.package
                                        installDialog.deviceId = modelData.key
                                        installDialog.deviceName = modelData.device
                                        installDialog.open()
                                    }
                                }
                            }

                            StyledText {
                                visible: Drivers.packageBackend === "pacman" && !Drivers.hasYay
                                Layout.fillWidth: true
                                text: Tr.t("yayMissing", Tr.language)
                                font.pixelSize: Appearance.font.pixelSize.smallest
                                color: Appearance.m3colors.m3error
                                wrapMode: Text.Wrap
                            }

                            StyledText {
                                Layout.fillWidth: true
                                text: Tr.t("proprietaryNote", Tr.language)
                                font.pixelSize: Appearance.font.pixelSize.smallest
                                color: Appearance.colors.colOnSurfaceVariant
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }
        }

        // ---- result banner --------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 4
            visible: root.banner !== ""
            implicitHeight: 34
            radius: Appearance.rounding.small
            color: root.bannerOk ? Appearance.colors.colSecondaryContainer
                                 : Appearance.m3colors.m3errorContainer

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 4
                spacing: 8

                MaterialSymbol {
                    text: root.bannerOk ? "check_circle" : "error"
                    iconSize: 16
                    color: root.bannerOk ? Appearance.colors.colOnSecondaryContainer
                                         : Appearance.m3colors.m3onErrorContainer
                }
                StyledText {
                    Layout.fillWidth: true
                    text: root.banner
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: root.bannerOk ? Appearance.colors.colOnSecondaryContainer
                                         : Appearance.m3colors.m3onErrorContainer
                    elide: Text.ElideMiddle
                }
                RippleButton {
                    buttonRadius: Appearance.rounding.full
                    implicitWidth: 24
                    implicitHeight: 24
                    onClicked: root.banner = ""
                    contentItem: MaterialSymbol {
                        anchors.centerIn: parent
                        text: "close"
                        iconSize: 14
                        color: root.bannerOk ? Appearance.colors.colOnSecondaryContainer
                                             : Appearance.m3colors.m3onErrorContainer
                    }
                }
            }
        }

        // ---- footer ------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            spacing: 8

            RippleButton {
                buttonText: Tr.t("rescan", Tr.language)
                buttonRadius: Appearance.rounding.small
                onClicked: root.rescan()
            }
            Item { Layout.fillWidth: true }
            RippleButton {
                buttonText: Tr.t("close", Tr.language)
                buttonRadius: Appearance.rounding.small
                onClicked: root.close()
            }
        }
    }

    // ---- install progress sub-dialog --------------------------------------
    PackageInstallDialog {
        id: installDialog
        backendName: Drivers.packageBackendName
        onInstallCompleted: (ok, deviceId) => {
            if (ok) {
                root.showBanner(true, installDialog.proprietary
                    ? Tr.t("installSucceeded", Tr.language) + " · "
                      + Tr.t("proprietaryRebootNote", Tr.language)
                    : Tr.t("installSucceeded", Tr.language))
                root.rescan()
                root.requestRefresh()
                // new module may now exist for the device; ask the kernel to
                // re-probe it so the driver binds without a reboot
                if (!installDialog.proprietary && root.pendingProbeDevice !== null)
                    Drivers.probeDevice(root.pendingProbeDevice)
            } else {
                root.showBanner(false, Tr.t("installFailed", Tr.language))
            }
        }
    }
}
