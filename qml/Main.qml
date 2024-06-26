import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window 2.2
import QtQuick.Dialogs
import ClapWorkbench
import "./Components" as O


Rectangle
{
    id: mainControl

    anchors.fill: parent

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

    Item
    {
        id: rect

        anchors
        {
            left: parent.left
            top: topBar.bottom
            right: channelStrip.left
            bottom: parent.bottom
            margins: 6
        }

        Loader
        {
            id: mainPluginWindowComponentLoader

            anchors.centerIn: parent
        }
    }

    TopBar
    {
        id: topBar

        anchors.left: parent.left
        anchors.right: parent.right
    }

    ChannelStrip
    {
        id: channelStrip

        anchors.top: topBar.bottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 178
    }

    Connections
    {
        target: app

        function onMainWindowPluginChanged()
        {
            if (!app.mainWindowPlugin)
            {
                mainPluginWindowComponentLoader.sourceComponent = undefined
                return
            }

            mainPluginWindowComponentLoader.setSource("Components/MainWindowPluginGUI.qml", { "plugin": app.mainWindowPlugin })
        }
    }

    FileDialog
    {
        id: loadSessionFileDialog

        nameFilters: ["Session files (*.js)"]
        fileMode: FileDialog.OpenFile

        onRejected: app.setAllFloatingWindowsVisibility(true)
        onAccepted: app.loadSession(selectedFile)
    }

    FileDialog
    {
        id: saveSessionAsFileDialog

        nameFilters: ["Session files (*.js)"]
        fileMode: FileDialog.SaveFile
        defaultSuffix: "js"

        onRejected: app.setAllFloatingWindowsVisibility(true)
        onAccepted: app.saveSession(selectedFile)
    }

    function openLoadSessionDialog()
    {
        app.setAllFloatingWindowsVisibility(false)
        loadSessionFileDialog.open()
    }

    function openSaveSessionAsDialog()
    {
        app.setAllFloatingWindowsVisibility(false)
        saveSessionAsFileDialog.open()
    }

    Component.onCompleted: app.informLoadFinished()
}
