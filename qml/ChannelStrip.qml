import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import ClapWorkbench
import "Components" as O


Rectangle
{
    id: control

    property QtObject channelStrip
    property int insertHeight: 20

    width: 151
    height: parent.height

    color: "#404b56"
    border.color: "#30373f"
    border.width: 1

    Rectangle
    {
        width: control.width
        height: childrenRect.height + 4

        anchors.top: parent.top
        anchors.topMargin: 14
        anchors.horizontalCenter: parent.horizontalCenter

        clip: true

        color: "#3b4750"
        border.color: "#aA262A2E"
        border.width: 1

        O.ReorderableListView
        {
            id: pluginSlots

            width: parent.width
            height: 260

            model: channelStrip.nodes

            delegate: O.PluginSlot
            {
                width: pluginSlots.width
                height: 24
            }

            onItemsReordered: (fromIndex, toIndex) =>
            {
                control.channelStrip.reorder(fromIndex, toIndex)
            }
        }

        O.LightButton
        {
            y: pluginSlots.childrenRect.bottom + 4
            anchors.horizontalCenter: parent.horizontalCenter

            bgHoveredColor: "#2f3338"
            bgPressedColor: "#262A2E"

            textColor: "#dddddd"
            textPressedColor: "#bbbbbb"

            font.pointSize: 22
            textVerticalOffset: -2

            text: "+"

            onClicked: app.openPluginBrowserWindow(control.channelStrip)
        }
    }

    O.Fader
    {
        id: fader

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 4

        height: 200

        from: 0.0
        to: 2.0

        value: channelStrip.outputVolume
        onPositionChanged: channelStrip.outputVolume = value

        orientation: Qt.Vertical
    }

    O.LightButton
    {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 3

        bgHoveredColor: "#2f3338"
        bgPressedColor: "#262A2E"

        textColor: "#dddddd"
        textPressedColor: "#bbbbbb"

        font.pointSize: 14

        text: "M"

        checked: channelStrip.isByPassed
        onClicked: channelStrip.isByPassed = !channelStrip.isByPassed
    }
}
