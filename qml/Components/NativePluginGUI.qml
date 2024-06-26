import QtQuick
import QtQuick.Controls.Basic


Item
{
    id: control

    property QtObject plugin
    property QtObject pluginWindow: pluginGUI

    width: pluginGUI.width
    height: pluginGUI.height

    Window
    {
        id: pluginGUI

        x: 0
        y: 0
        width: plugin? plugin.guiSize.width : 0
        height: plugin? plugin.guiSize.height: 0

        parent: control
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
        if (!Window.window ||
            !Window.window.addKeyPressListener)
            return

        Window.window.addKeyPressListener(pluginGUI)
    }
}
