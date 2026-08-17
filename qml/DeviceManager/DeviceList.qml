// Two-level device tree: expandable categories with device rows underneath.
// Rows live in a ListModel that rebuild() patches incrementally (diff against
// the previous row list), so unchanged rows keep their delegates and the
// add/remove/displaced transitions only run for rows that really changed —
// that is what makes category expand/collapse animate.
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
        applyRows(out)
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

    // Stable identity of a row across rebuilds; rows never reorder, they are
    // only inserted/removed in place, so key comparison drives the diff below.
    function rowKey(r) {
        return r.kind === "category" ? "c:" + r.key : "d:" + r.id
    }

    function rowsEqual(a, b) {
        if (rowKey(a) !== rowKey(b))
            return false
        if (a.kind === "category")
            return a.name === b.name && a.icon === b.icon
                    && a.count === b.count && a.expanded === b.expanded
        return a.icon === b.icon && a.status === b.status && a.name === b.name
                && a.subtitle === b.subtitle && a.dev === b.dev
    }

    // Apply the target row list to rowModel with minimal edits (update in
    // place / insert / remove) instead of resetting the model, so the view can
    // animate only the rows that actually changed.
    function applyRows(out) {
        var present = {}
        for (var k = 0; k < out.length; k++)
            present[rowKey(out[k])] = true
        var i = 0, j = 0
        while (j < out.length) {
            if (i < root.rows.length && rowKey(root.rows[i]) === rowKey(out[j])) {
                if (!rowsEqual(root.rows[i], out[j]))
                    rowModel.set(i, { rowData: out[j] })
                root.rows[i] = out[j]
                i++
                j++
                continue
            }
            if (i < root.rows.length && !present[rowKey(root.rows[i])]) {
                rowModel.remove(i, 1)
                root.rows.splice(i, 1)
                continue
            }
            rowModel.insert(i, { rowData: out[j] })
            root.rows.splice(i, 0, out[j])
            i++
            j++
        }
        while (i < root.rows.length) {
            rowModel.remove(i, 1)
            root.rows.splice(i, 1)
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

    ListModel {
        id: rowModel
    }

    ListView {
        id: listView
        anchors.fill: parent
        clip: true
        model: rowModel
        spacing: 2
        topMargin: 4
        bottomMargin: 4
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        // Expand/collapse: device rows grow from small to full size while
        // fading in; on collapse they shrink upward into the category while
        // fading out. Every row stays inside its own layout slot during the
        // animation (nothing slides over a neighbour), so the texts of two
        // rows never cross; the rows below slide up or down into the vacated
        // space. Unchanged rows are never touched (see applyRows).
        add: Transition {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: Appearance.animation.elementMoveEnter.duration
                easing.bezierCurve: Appearance.animation.elementMoveEnter.bezierCurve
            }
            NumberAnimation {
                property: "scale"
                from: 0.8
                to: 1
                duration: Appearance.animation.elementMoveEnter.duration
                easing.bezierCurve: Appearance.animation.elementMoveEnter.bezierCurve
            }
        }
        remove: Transition {
            // Collapse toward the category header: the decel curve shrinks the
            // row out of the way before the rows below slide up into its
            // slot, so their texts never overlap.
            PropertyAction {
                property: "transformOrigin"
                value: Item.Top
            }
            NumberAnimation {
                property: "opacity"
                to: 0
                duration: Appearance.animation.elementMoveExit.duration
                easing.bezierCurve: Appearance.animationCurves.emphasizedDecel
            }
            NumberAnimation {
                property: "scale"
                to: 0.05
                duration: Appearance.animation.elementMoveExit.duration
                easing.bezierCurve: Appearance.animationCurves.emphasizedDecel
            }
        }
        displaced: Transition {
            NumberAnimation {
                property: "y"
                duration: Appearance.animation.elementMoveFast.duration
                easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
            }
        }

        delegate: Loader {
            required property var rowData
            width: listView.width
            sourceComponent: rowData.kind === "category" ? categoryComponent : deviceComponent
            onLoaded: item.rowData = rowData
            onRowDataChanged: if (item) item.rowData = rowData
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
                    text: "expand_more"
                    iconSize: 18
                    color: Appearance.colors.colOnLayer1Inactive
                    Layout.preferredWidth: 18
                    rotation: rowData.expanded ? 0 : -90
                    Behavior on rotation {
                        NumberAnimation {
                            duration: Appearance.animation.elementMoveFast.duration
                            easing.bezierCurve: Appearance.animation.elementMoveFast.bezierCurve
                        }
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
