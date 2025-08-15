import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window 2.2
import QtQuick.Dialogs
import ClapWorkbench
import "." as O


Rectangle
{
    id: control

    property QtObject plugin
    property int guiType

    focus: true

    width: componentLoader.width
    height: componentLoader.height + buttonsRow.height

    radius: 8

    clip: true

    color: "#e7e6e1"

    MouseArea
    {
        anchors.fill: parent

        onPressedChanged:
        {
            if (!pressed)
                return

            Window.window.startSystemMove()
        }
    }

    O.LightButton
    {
        x: 3
        y: 3

        text: "\u2315"

        font.pointSize: 35

        onClicked: app.openPluginBrowserWindow(control.plugin, null)
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

            visible: control.plugin && control.plugin.hasNativeGUI
            onClicked: control.switchGuiType()

            font.family: "icons"
            font.pointSize: 15
        }

        O.LightButton
        {
            text: "\u2190"

            onClicked: app.mainWindowPlugin = control.plugin

            font.family: "icons"
            font.pointSize: 19
        }

        O.LightButton
        {
            text: "\u25CB"

            onClicked: control.plugin.isByPassed = !control.plugin.isByPassed

            checked: plugin && control.plugin.isByPassed

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

    onPluginChanged: updateGuiType()

    Connections
    {
        target: control.plugin

        function onHostedPluginChanged()
        {
            updateGuiType()
        }
    }

    function updateLoader()
    {
        componentLoader.setSource("")
        let newSource = ""

        switch (control.guiType)
        {
            case Ocp.GuiType.Generic: newSource = "GenericPluginGUI.qml"; break
            case Ocp.GuiType.Native:  newSource = "NativePluginGUI.qml" ; break
        }

        componentLoader.setSource(newSource, { "plugin": control.plugin })
    }

    function updateGuiType()
    {
        if (control.plugin === null)
        {
            componentLoader.setSource("")
            return
        }

        control.guiType = control.plugin.hasNativeGUI? Ocp.GuiType.Native : Ocp.GuiType.Generic
        updateLoader()
    }

    function switchGuiType()
    {
        control.guiType = control.guiType === Ocp.GuiType.Native? Ocp.GuiType.Generic : Ocp.GuiType.Native
        updateLoader()
    }
}
