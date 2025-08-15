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

    focus: true

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

    TopBar
    {
        id: topBar

        anchors.left: parent.left
        anchors.right: parent.right
    }

    // ChannelStrip
    // {
    //     id: channelStrip
    //
    //     anchors.top: topBar.bottom
    //     anchors.left: parent.left
    //     anchors.bottom: parent.bottom
    // }
    //
    // ChannelStrip
    // {
    //     id: channelStrip3
    //
    //     anchors.top: topBar.bottom
    //     anchors.left: channelStrip.right
    //     anchors.bottom: parent.bottom
    // }

    // Item
    // {
    //     id: rect
    //
    //     anchors
    //     {
    //         left: parent.left
    //         top: topBar.bottom
    //         right: channelStrip2.left
    //         bottom: parent.bottom
    //         margins: 6
    //     }
    //
    //     Loader
    //     {
    //         id: mainPluginWindowComponentLoader
    //
    //         anchors.centerIn: parent
    //     }
    // }

    ListView
    {
        id: channels

        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        model: audioEngine.channelStrips

        orientation: ListView.Horizontal
        layoutDirection: Qt.RightToLeft

        delegate: ChannelStrip
        {
            channelStrip: modelData
        }

        footer: Item
        {
            width: 32
            height: channels.height

            O.LightButton
            {
                anchors.verticalCenter: parent.verticalCenter

                font.pointSize: 22
                textVerticalOffset: -2

                text: "+"

                onClicked: audioEngine.addNewChannelStrip()
            }
        }
    }

    Keys.onPressed: (event) =>
    {
        audioEngine.key(event.key, true)
        event.accepted = true
    }

    Keys.onReleased: (event) =>
    {
        audioEngine.key(event.key, false)
        event.accepted = true
    }

    Connections
    {
        target: app

        function onMainWindowPluginChanged()
        {
            if (!app.mainWindowPlugin)
            {
                // mainPluginWindowComponentLoader.sourceComponent = undefined
                return
            }

            // mainPluginWindowComponentLoader.setSource("Components/MainWindowPluginGUI.qml", { "plugin": app.mainWindowPlugin })
        }
    }

    FileDialog
    {
        id: loadSessionFileDialog

        nameFilters: ["Session files (*.js)"]
        fileMode: FileDialog.OpenFile

        onAccepted: app.loadSession(selectedFile)
    }

    FileDialog
    {
        id: saveSessionAsFileDialog

        nameFilters: ["Session files (*.js)"]
        fileMode: FileDialog.SaveFile
        defaultSuffix: "js"

        onAccepted: app.saveSession(selectedFile)
    }

    function openLoadSessionDialog()
    {
        loadSessionFileDialog.open()
    }

    function openSaveSessionAsDialog()
    {
        saveSessionAsFileDialog.open()
    }

    Component.onCompleted: app.informLoadFinished()
}
