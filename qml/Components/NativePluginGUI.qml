import QtQuick
import QtQuick.Controls.Basic


WindowContainer
{
    id: control

    property QtObject plugin

    width: plugin? plugin.guiSize.width : 0
    height: plugin? plugin.guiSize.height: 0

    window: Window
    {
        id: pluginGUI

        x: control.x
        y: control.y
        width: control.width
        height: control.height

        visible: true
    }

    onPluginChanged:
    {
        if (plugin === null)
            return

        plugin.setParentWindow(pluginGUI)
    }

    Component.onCompleted:
    {
        if (!Window.window || !Window.window.addKeyPressListener)
            return

        Window.window.addKeyPressListener(pluginGUI)
    }
}
