// Graphics support check dialog: shows whether the detected GPUs support
// OpenGL and Vulkan. Detection is done by the `Graphics` backend (driver /
// ICD file probing, plus optional glxinfo / vulkaninfo enrichment).
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components

AnimatedDialog {
    id: root

    width: Math.min(720, parent.width * 0.9)
    height: Math.min(600, parent.height * 0.88)

    // whether the detected OpenGL is software-only (llvmpipe / softpipe)
    readonly property bool glSoftware: {
        var r = Graphics.openGLRenderer.toLowerCase()
        return r.indexOf("llvmpipe") >= 0 || r.indexOf("softpipe") >= 0
    }

    function listHasSoftware(list) {
        for (var i = 0; i < list.length; i++)
            if (list[i].indexOf("software") >= 0 || list[i].indexOf("LLVMpipe") >= 0)
                return true
        return false
    }

    // small colored pill: supported / software / not supported
    component CapBadge: Rectangle {
        property bool supported: false
        property bool software: false
        property string label: ""

        implicitHeight: 22
        implicitWidth: badgeRow.implicitWidth + 14
        radius: 11
        color: supported ? Appearance.m3colors.m3successContainer
            : software ? "#FFD54F"
            : Appearance.colors.colSurfaceContainerHighest

        RowLayout {
            id: badgeRow
            anchors.centerIn: parent
            spacing: 4
            MaterialSymbol {
                text: supported ? "check_circle" : software ? "blur_on" : "block"
                iconSize: 12
                color: supported ? Appearance.m3colors.m3onSuccessContainer
                    : software ? "#4A3800" : Appearance.colors.colOnSurfaceVariant
            }
            StyledText {
                text: label
                font.pixelSize: 11
                color: supported ? Appearance.m3colors.m3onSuccessContainer
                    : software ? "#4A3800" : Appearance.colors.colOnSurfaceVariant
            }
        }
    }

    onOpened: {
        Graphics.refresh()
        Graphics.requestDetails()
    }

    contentItem: ColumnLayout {
        spacing: 0

        // ---- header -----------------------------------------------------
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
                    text: "view_in_ar"
                    iconSize: 24
                    color: Appearance.colors.colOnSecondaryContainer
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                StyledText {
                    Layout.fillWidth: true
                    text: Tr.t("graphicsSupportTitle", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.normal
                    font.weight: Font.Medium
                    color: Appearance.colors.colOnLayer1
                }
                StyledText {
                    Layout.fillWidth: true
                    text: Tr.t("graphicsSupportSubtitle", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallest
                    color: Appearance.colors.colOnLayer1
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

        // ---- overall OpenGL / Vulkan cards ------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 10
            spacing: 10

            // OpenGL
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 108
                radius: Appearance.rounding.normal
                color: Appearance.colors.colLayer2

                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    anchors.topMargin: 12
                    anchors.bottomMargin: 10
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        MaterialSymbol {
                            text: "gradient"
                            iconSize: 20
                            color: Graphics.openGLSupported
                                ? (root.glSoftware ? "#4A3800" : Appearance.m3colors.m3success)
                                : Appearance.colors.colOnLayer1Inactive
                        }
                        StyledText {
                            text: Tr.t("openGL", Tr.language)
                            font.pixelSize: Appearance.font.pixelSize.smallie
                            font.weight: Font.DemiBold
                            color: Appearance.colors.colOnLayer1
                        }
                        Item { Layout.fillWidth: true }
                        CapBadge {
                            supported: Graphics.openGLSupported
                            software: root.glSoftware
                            label: !Graphics.openGLSupported ? Tr.t("notSupported", Tr.language)
                                : root.glSoftware ? Tr.t("softwareRendering", Tr.language)
                                : Tr.t("supported", Tr.language)
                        }
                    }

                    StyledText {
                        Layout.fillWidth: true
                        text: Graphics.openGLRenderer !== ""
                            ? Graphics.openGLRenderer
                            : Graphics.openGLProviders.join(", ")
                        elide: Text.ElideRight
                        font.pixelSize: Appearance.font.pixelSize.smallest
                        color: Graphics.openGLSupported
                            ? Appearance.colors.colOnLayer1
                            : Appearance.colors.colOnLayer1Inactive
                    }
                    StyledText {
                        Layout.fillWidth: true
                        text: Graphics.openGLVersion !== ""
                            ? Tr.t("glVersion", Tr.language) + "：" + Graphics.openGLVersion
                            : Tr.t("graphicsToolsHint", Tr.language)
                        elide: Text.ElideRight
                        font.pixelSize: Appearance.font.pixelSize.smallest
                        color: Appearance.colors.colOnLayer1
                    }
                }
            }

            // Vulkan
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 108
                radius: Appearance.rounding.normal
                color: Appearance.colors.colLayer2

                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    anchors.topMargin: 12
                    anchors.bottomMargin: 10
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        MaterialSymbol {
                            text: "view_in_ar"
                            iconSize: 20
                            color: Graphics.vulkanSupported
                                ? (root.listHasSoftware(Graphics.vulkanDrivers) ? "#4A3800" : Appearance.m3colors.m3success)
                                : Appearance.colors.colOnLayer1Inactive
                        }
                        StyledText {
                            text: Tr.t("vulkan", Tr.language)
                            font.pixelSize: Appearance.font.pixelSize.smallie
                            font.weight: Font.DemiBold
                            color: Appearance.colors.colOnLayer1
                        }
                        Item { Layout.fillWidth: true }
                        CapBadge {
                            supported: Graphics.vulkanSupported
                            software: root.listHasSoftware(Graphics.vulkanDrivers)
                            label: !Graphics.vulkanSupported ? Tr.t("notSupported", Tr.language)
                                : root.listHasSoftware(Graphics.vulkanDrivers) ? Tr.t("softwareRendering", Tr.language)
                                : Tr.t("supported", Tr.language)
                        }
                    }

                    StyledText {
                        Layout.fillWidth: true
                        text: Graphics.vulkanDrivers.join(", ")
                        elide: Text.ElideRight
                        font.pixelSize: Appearance.font.pixelSize.smallest
                        color: Graphics.vulkanSupported
                            ? Appearance.colors.colOnLayer1
                            : Appearance.colors.colOnLayer1Inactive
                    }
                    StyledText {
                        Layout.fillWidth: true
                        text: Graphics.vulkanApiVersion !== ""
                            ? Tr.t("vkApiVersion", Tr.language) + "：" + Graphics.vulkanApiVersion
                            : Tr.t("graphicsToolsHint", Tr.language)
                        elide: Text.ElideRight
                        font.pixelSize: Appearance.font.pixelSize.smallest
                        color: Appearance.colors.colOnLayer1
                    }
                }
            }
        }

        // ---- probing indicator ------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            visible: Graphics.loading
            spacing: 6

            MaterialSymbol {
                text: "sync"
                iconSize: 15
                color: Appearance.colors.colOnLayer1Inactive
                RotationAnimation on rotation {
                    from: 0
                    to: 360
                    duration: 900
                    loops: Animation.Infinite
                }
            }
            StyledText {
                text: Tr.t("graphicsDetecting", Tr.language)
                font.pixelSize: Appearance.font.pixelSize.smallest
                color: Appearance.colors.colOnLayer1
            }
        }

        // ---- GPU list ---------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 4
            spacing: 8

            StyledText {
                text: Tr.t("gpuList", Tr.language)
                font.pixelSize: Appearance.font.pixelSize.smallie
                font.weight: Font.DemiBold
                color: Appearance.colors.colOnLayer1
            }
            Item { Layout.fillWidth: true }
            StyledText {
                text: Graphics.gpus.length + " · " + Tr.t("openGL", Tr.language) + " / " + Tr.t("vulkan", Tr.language)
                font.pixelSize: Appearance.font.pixelSize.smallest
                color: Appearance.colors.colOnLayer1
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.bottomMargin: 12
            clip: true

            // no GPU detected
            ColumnLayout {
                anchors.centerIn: parent
                visible: Graphics.gpus.length === 0
                spacing: 8
                MaterialSymbol {
                    Layout.alignment: Qt.AlignHCenter
                    text: "monitor"
                    iconSize: 36
                    color: Appearance.colors.colOnLayer1Inactive
                }
                StyledText {
                    Layout.alignment: Qt.AlignHCenter
                    text: Tr.t("noGpuFound", Tr.language)
                    font.pixelSize: Appearance.font.pixelSize.smallie
                    color: Appearance.colors.colOnLayer1
                }
            }

            Flickable {
                anchors.fill: parent
                visible: Graphics.gpus.length > 0
                clip: true
                contentHeight: gpuColumn.implicitHeight + 8
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: gpuColumn
                    width: parent.width
                    spacing: 8

                    Repeater {
                        model: Graphics.gpus
                        delegate: Rectangle {
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: 66
                            radius: Appearance.rounding.normal
                            color: Appearance.colors.colLayer2

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 12

                                Rectangle {
                                    Layout.preferredWidth: 38
                                    Layout.preferredHeight: 38
                                    radius: Appearance.rounding.normal
                                    color: Appearance.colors.colSecondaryContainer
                                    MaterialSymbol {
                                        anchors.centerIn: parent
                                        text: "monitor"
                                        iconSize: 20
                                        color: Appearance.colors.colOnSecondaryContainer
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    StyledText {
                                        Layout.fillWidth: true
                                        text: modelData.name
                                        elide: Text.ElideRight
                                        font.pixelSize: Appearance.font.pixelSize.smallie
                                        font.weight: Font.Medium
                                        color: Appearance.colors.colOnLayer1
                                    }
                                    StyledText {
                                        Layout.fillWidth: true
                                        text: modelData.vendor + (modelData.driver !== "" ? " · " + modelData.driver : "")
                                        elide: Text.ElideRight
                                        font.pixelSize: Appearance.font.pixelSize.smallest
                                        color: Appearance.colors.colOnLayer1
                                    }
                                    StyledText {
                                        Layout.fillWidth: true
                                        visible: modelData.glNote !== "" || modelData.vkNote !== ""
                                        text: modelData.glNote + (modelData.glNote !== "" && modelData.vkNote !== "" ? " · " : "") + modelData.vkNote
                                        elide: Text.ElideRight
                                        font.pixelSize: Appearance.font.pixelSize.smallest
                                        color: (modelData.glSupported || modelData.vkSupported)
                                            ? Appearance.colors.colOnLayer1Inactive
                                            : Appearance.m3colors.m3error
                                    }
                                }

                                CapBadge {
                                    supported: modelData.glSupported
                                    software: false
                                    label: Tr.t("openGL", Tr.language)
                                }
                                CapBadge {
                                    supported: modelData.vkSupported
                                    software: false
                                    label: Tr.t("vulkan", Tr.language)
                                }
                            }
                        }
                    }
                }
            }
        }

        // ---- footer -----------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            spacing: 8

            RippleButton {
                buttonText: Tr.t("refresh", Tr.language)
                buttonRadius: Appearance.rounding.small
                implicitWidth: 80
                implicitHeight: 34
                enabled: !Graphics.loading
                onClicked: {
                    Graphics.refresh()
                    Graphics.requestDetails()
                }
            }
            Item { Layout.fillWidth: true }
            RippleButton {
                buttonText: Tr.t("close", Tr.language)
                buttonRadius: Appearance.rounding.small
                implicitWidth: 80
                implicitHeight: 34
                onClicked: root.close()
            }
        }
    }
}
