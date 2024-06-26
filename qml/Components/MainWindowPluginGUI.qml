import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window 2.2
import QtQuick.Dialogs
import ClapWorkbench
import "." as O
import "../Utils.js" as Utils


Rectangle
{
    id: control

    property QtObject plugin
    property int guiType

    width: componentLoader.width
    height: componentLoader.height + buttonsRow.height

    topLeftRadius: 5
    topRightRadius: 5

    color: "#f3f3f1"

    O.LightButton
    {
        x: 3
        y: 3
        text: "\u2315"

        font.pointSize: 35

        onClicked: Utils.openPluginBrowser(control.plugin)
    }

    Row
    {
        id: buttonsRow

        anchors.right: parent.right

        spacing: 1
        padding: 3

        O.LightButton
        {
            text: "UI"

            visible: control.plugin.hasNativeGUI
            enabled: !control.plugin.isFloatingWindowOpen
            onClicked: updateGuiType(control.guiType === Ocp.GuiType.Native? Ocp.GuiType.Generic : Ocp.GuiType.Native)

            font.family: "icons"
            font.pointSize: 15
        }

        O.LightButton
        {
            text: "\ue900"

            onClicked:
            {
                if (control.plugin.isFloatingWindowOpen)
                {
                    control.plugin.isFloatingWindowOpen = false
                }
                else
                {
                    control.guiType = Ocp.GuiType.Generic
                    updateGuiType()

                    control.plugin.isFloatingWindowOpen = true
                }
            }

            checked: control.plugin.isFloatingWindowOpen

            font.family: "icons"
            font.pointSize: 19
        }

        O.LightButton
        {
            text: "\u25CB"

            onClicked: control.plugin.isByPassed = !control.plugin.isByPassed

            checked: control.plugin.isByPassed

            font.family: "icons"
            font.pointSize: 17
        }
    }

    Loader
    {
        id: componentLoader

        x: 0
        y: buttonsRow.height
    }

    onPluginChanged: resetPluginGUIType()

    Connections
    {
        target: control.plugin

        function onIsFloatingWindowOpenChanged()
        {
            if (!control.plugin.isFloatingWindowOpen)
                return

            control.guiType = Ocp.GuiType.Generic
            updateGuiType()
        }
    }

    function updateGuiType(newGuiType)
    {
        if (newGuiType !== undefined)
            control.guiType = newGuiType

        let newSource = control.guiType === Ocp.GuiType.Native
            ? "NativePluginGUI.qml"
            : "GenericPluginGUI.qml"

        componentLoader.setSource(newSource, { "plugin": control.plugin })
    }

    function resetPluginGUIType()
    {
        control.guiType = (!control.plugin.hasNativeGUI || control.plugin.isFloatingWindowOpen)
            ? Ocp.GuiType.Generic
            : Ocp.GuiType.Native

        updateGuiType()
    }

    Component.onCompleted: resetPluginGUIType()
}
