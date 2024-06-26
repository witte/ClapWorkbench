import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import ClapWorkbench
import "Components" as O


Item
{
    id: control

    property int insertHeight: 20

    WindowContainer
    {
        width: control.width
        height: control.height

        window: Window {
            width: control.width
            height: control.height

            color: "#404b56"

            Rectangle {
                anchors.fill: parent

                color: "#404b56"
                border.color: "#30373f"
                border.width: 1
            }

            Rectangle {
                width: 152
                height: childrenRect.height + 4
                anchors.top: parent.top
                anchors.topMargin: 14
                anchors.horizontalCenter: parent.horizontalCenter

                clip: true
                radius: 5

                color: "#3b4750"
                border.color: "#aA262A2E"
                border.width: 1

                O.ReorderableListView {
                    id: pluginSlots

                    width: parent.width
                    height: 260

                    model: audioEngine.plugins

                    delegate: O.PluginSlot
                    {
                        width: pluginSlots.width
                        height: 24
                    }

                    onItemsReordered: (fromIndex, toIndex) => {
                        audioEngine.reorder(fromIndex, toIndex)
                    }
                }

                O.LightButton {
                    y: pluginSlots.childrenRect.bottom + 4
                    anchors.horizontalCenter: parent.horizontalCenter

                    bgHoveredColor: "#2f3338"
                    bgPressedColor: "#262A2E"

                    textColor: "#dddddd"
                    textPressedColor: "#bbbbbb"

                    font.pointSize: 22
                    textVerticalOffset: -2

                    text: "+"

                    onClicked: app.openPluginBrowserWindow()
                }
            }

            O.Fader {
                id: fader

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 4

                height: 200

                from: 0.0
                to: 2.0

                value: audioEngine.outputVolume
                onPositionChanged: audioEngine.outputVolume = value

                orientation: Qt.Vertical
            }
        }
    }
}