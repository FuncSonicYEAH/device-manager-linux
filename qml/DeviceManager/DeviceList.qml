// Two-level device tree: expandable categories with device rows underneath.
// Rows are rebuilt into a plain JS array so the delegates stay simple.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

Item {
    id: root

    property var groups: []
    property string searchText: ""
    property var expandedKeys: ({})
    property string selectedDeviceId: ""

    signal deviceClicked(var device)
    // right-click: coordinates are relative to the window so a context menu
    // can be positioned at the pointer
    signal deviceContextRequested(var device, real x, real y)
    signal categoryToggled(string key)

    property var rows: []

    function matches(dev, query) {
        var q = query.toLowerCase()
        return dev.name.toLowerCase().indexOf(q) !== -1
            || dev.vendor.toLowerCase().indexOf(q) !== -1
            || dev.driver.toLowerCase().indexOf(q) !== -1
            || dev.modalias.toLowerCase().indexOf(q) !== -1
            || dev.subtitle.toLowerCase().indexOf(q) !== -1
    }

    function rebuild() {
        var q = root.searchText.trim()
        var searching = q !== ""
        var out = []
        for (var g = 0; g < root.groups.length; g++) {
            var group = root.groups[g]
            var devs = group.devices
            if (searching) {
                var matched = []
                for (var d = 0; d < devs.length; d++)
                    if (root.matches(devs[d], q))
                        matched.push(devs[d])
                if (matched.length === 0)
                    continue
                out.push({ kind: "category", key: group.key, name: group.name, icon: group.icon, count: matched.length, expanded: true })
                for (var m = 0; m < matched.length; m++)
                    out.push(root.rowForDevice(matched[m], group))
            } else {
                var expanded = root.expandedKeys[group.key] === true
                out.push({ kind: "category", key: group.key, name: group.name, icon: group.icon, count: devs.length, expanded: expanded })
                if (expanded)
                    for (var e = 0; e < devs.length; e++)
                        out.push(root.rowForDevice(devs[e], group))
            }
        }
        root.rows = out
    }

    function rowForDevice(dev, group) {
        return {
            kind: "device",
            id: dev.id,
            icon: dev.icon,
            status: dev.status,
            name: dev.name,
            subtitle: dev.subtitle,
            dev: dev
        }
    }

    // Arrow-key navigation across visible device rows.
    function moveSelection(delta) {
        var idx = -1
        for (var i = 0; i < root.rows.length; i++)
            if (root.rows[i].kind === "device" && root.rows[i].id === root.selectedDeviceId) {
                idx = i
                break
            }
        var j = idx
        for (;;) {
            j += delta
            if (j < 0 || j >= root.rows.length)
                return
            if (root.rows[j].kind === "device") {
                root.deviceClicked(root.rows[j].dev)
                return
            }
        }
    }

    onGroupsChanged: root.rebuild()
    onSearchTextChanged: root.rebuild()
    onExpandedKeysChanged: root.rebuild()

    ListView {
        id: listView
        anchors.fill: parent
        clip: true
        model: root.rows
        spacing: 2
        topMargin: 4
        bottomMargin: 4
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        delegate: Loader {
            required property var modelData
            width: listView.width
            sourceComponent: modelData.kind === "category" ? categoryComponent : deviceComponent
            onLoaded: {
                item.rowData = modelData
            }
        }
    }

    Component {
        id: categoryComponent
        RippleButton {
            id: catRow
            property var rowData
            width: listView.width
            height: 42
            buttonRadius: Appearance.rounding.small
            rippleEnabled: false
            onClicked: root.categoryToggled(rowData.key)

            contentItem: RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8

                MaterialSymbol {
                    text: rowData.expanded ? "expand_more" : "chevron_right"
                    iconSize: 18
                    color: Appearance.colors.colOnLayer1Inactive
                    Layout.preferredWidth: 18
                    Behavior on rotation {
                        NumberAnimation { duration: Appearance.animation.elementMoveFast.duration }
                    }
                }
                MaterialSymbol {
                    text: rowData.icon
                    iconSize: 20
                    color: Appearance.m3colors.m3primary
                    Layout.preferredWidth: 24
                }
                StyledText {
                    text: rowData.name
                    Layout.fillWidth: true
                    font.pixelSize: Appearance.font.pixelSize.small
                    font.weight: Font.DemiBold
                    color: Appearance.colors.colOnLayer1
                }
                StyledText {
                    text: rowData.count
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1Inactive
                }
            }
        }
    }

    Component {
        id: deviceComponent
        RippleButton {
            id: devRow
            property var rowData
            width: listView.width
            height: 50
            buttonRadius: Appearance.rounding.small
            toggled: root.selectedDeviceId === rowData.id
            colBackgroundToggled: Appearance.colors.colSecondaryContainer
            colBackgroundToggledHover: Appearance.colors.colSecondaryContainerHover
            colBackgroundToggledActive: Appearance.colors.colSecondaryContainerActive
            onClicked: root.deviceClicked(rowData.dev)
            altAction: (event) => {
                var pos = devRow.mapToItem(null, event.x, event.y)
                root.deviceContextRequested(rowData.dev, pos.x, pos.y)
            }

            contentItem: RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 12
                anchors.rightMargin: 10
                spacing: 10

                Rectangle {
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    radius: Appearance.rounding.small
                    color: devRow.toggled
                        ? Appearance.colors.colSecondaryContainer
                        : Appearance.colors.colLayer2
                    MaterialSymbol {
                        anchors.centerIn: parent
                        text: rowData.icon
                        iconSize: 19
                        color: devRow.toggled
                            ? Appearance.colors.colOnSecondaryContainer
                            : Appearance.colors.colOnLayer2
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1

                    StyledText {
                        text: rowData.name
                        Layout.fillWidth: true
                        font.pixelSize: Appearance.font.pixelSize.small
                        color: Appearance.colors.colOnLayer1
                        elide: Text.ElideRight
                    }
                    StyledText {
                        text: rowData.subtitle
                        Layout.fillWidth: true
                        font.pixelSize: Appearance.font.pixelSize.smallest
                        color: Appearance.colors.colOnLayer1
                        elide: Text.ElideRight
                    }
                }

                StatusBadge {
                    visible: rowData.status !== "ok"
                    status: rowData.status
                    Layout.preferredHeight: 20
                }
            }
        }
    }
}
