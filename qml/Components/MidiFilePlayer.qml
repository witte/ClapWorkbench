import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import ClapWorkbench


Rectangle
{
    id: control

    property QtObject node

    width: 91
    height: parent.height

    color: "#ac2929"
    border.color: "#30373f"
    border.width: 1

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

    Text
    {
        anchors.centerIn: parent

        color: "#dddddd"
        font.pointSize: 38

        text: "[WIP]"
    }
}
