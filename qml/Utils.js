
function openPluginBrowser(pluginHost)
{
    let component = Qt.createComponent("Components/PluginBrowserWindow.qml")

    if (component.status !== Component.Ready)
        return

    component.createObject(null, pluginHost? { plugin: pluginHost } : {})
}

function openMainMenuWindow()
{
    let component = Qt.createComponent("Components/MainMenuWindow.qml")

    if (component.status === Component.Ready)
        component.createObject(component)
}
