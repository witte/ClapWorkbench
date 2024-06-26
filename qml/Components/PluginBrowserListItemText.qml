import QtQuick
import QtQuick.Controls.Basic


Text
{
    property string toolTip: text

    anchors.verticalCenter: parent.verticalCenter

    elide: Text.ElideRight
    color: "#ffffff"

    MouseArea
    {
        id: toolTipArea

        anchors.fill: parent

        hoverEnabled: true
        propagateComposedEvents: true
    }

    ToolTip.visible: toolTipArea.containsMouse
    ToolTip.text: toolTip
    ToolTip.delay: 600
}
