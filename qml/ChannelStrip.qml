import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import ClapWorkbench
import "Components" as O


Rectangle
{
    id: control

    property QtObject node
    property int insertHeight: 20

    width: nodeChannels.visible? nodeChannels.width + 151 : 151

    color: "#404b56"
    border.color: "#30373f"
    border.width: 1

    Row
    {
        id: nodeChannels
        spacing: 0
        layoutDirection: Qt.RightToLeft
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10

        Repeater
        {
            model: node.channels

            delegate: DelegateChooser
            {
                id: chooser
                role: "type"

                DelegateChoice
                {
                    roleValue: 1
                    delegate: channelStripDelegate
                }

                DelegateChoice
                {
                    roleValue: 3
                    delegate: O.MidiFilePlayer
                    {
                        node: modelData
                    }
                }
            }
        }
    }

    Item
    {
        id: mainChannel

        width: 151
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        TextInput
        {
            id: input

            y: 4
            width: contentWidth
            height: contentHeight
            anchors.horizontalCenter: parent.horizontalCenter

            text: node.name

            color: "#dddddd"

            onEditingFinished: node.name = text
        }

        Rectangle
        {
            width: parent.width
            height: childrenRect.height + 4

            anchors.top: parent.top
            anchors.topMargin: 24
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

                model: node.nodes

                delegate: O.PluginSlot
                {
                    channelStrip: control.node
                    width: pluginSlots.width
                    height: 24
                }

                onItemsReordered: (fromIndex, toIndex) =>
                {
                    control.node.reorder(fromIndex, toIndex)
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

                onClicked: app.openPluginBrowserWindow(control.node, null)
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

            value: node.outputVolume
            onPositionChanged: node.outputVolume = value

            orientation: Qt.Vertical
        }

        O.LightButton
        {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.margins: 3

            bgHoveredColor: "#2f3338"
            bgPressedColor: "#262A2E"

            textColor: "#dddddd"
            textPressedColor: "#bbbbbb"

            font.pointSize: 14

            visible: node.channels.length > 0
            text: checked? "<" : "v"

            checked: nodeChannels.visible
            onClicked: nodeChannels.visible = !nodeChannels.visible
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

            checked: node.isByPassed
            onClicked: node.isByPassed = !node.isByPassed
        }
    }

    Component.onCompleted: console.log(`! ${node.name}`)
    Component.onDestruction: console.log(`~ ${node.name}`)
}
