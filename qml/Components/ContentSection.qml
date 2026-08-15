// Port of modules/common/widgets/ContentSection.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Components
import Components

ColumnLayout {
    id: root

    property string title
    property string icon: ""
    default property alias contentData: sectionContent.data

    Layout.fillWidth: true
    spacing: 6

    RowLayout {
        spacing: 6
        Loader {
            active: root.icon !== ""
            sourceComponent: MaterialSymbol {
                iconSize: Appearance.font.pixelSize.hugeass
                text: root.icon
                color: Appearance.colors.colOnSecondaryContainer
            }
        }
        StyledText {
            text: root.title
            font.pixelSize: Appearance.font.pixelSize.larger
            font.weight: Font.Medium
            color: Appearance.colors.colOnSecondaryContainer
        }
    }

    ColumnLayout {
        id: sectionContent
        Layout.fillWidth: true
        spacing: 4
    }
}
