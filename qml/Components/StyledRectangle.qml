// Port of modules/common/widgets/StyledRectangle.qml
import QtQuick

Rectangle {
    id: root

    enum ContentLayer { Background, Pane, Group, Subgroup, Control }

    property var contentLayer: StyledRectangle.ContentLayer.Pane

    color: switch (contentLayer) {
        case StyledRectangle.ContentLayer.Background: return Appearance.colors.colLayer0;
        case StyledRectangle.ContentLayer.Pane: return Appearance.colors.colLayer1;
        case StyledRectangle.ContentLayer.Group: return Appearance.colors.colLayer2;
        case StyledRectangle.ContentLayer.Subgroup: return Appearance.colors.colLayer3;
        case StyledRectangle.ContentLayer.Control: return Appearance.colors.colLayer4;
        default: return Appearance.colors.colLayer1;
    }
}
