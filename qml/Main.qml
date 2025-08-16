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

    Component
    {
        // we need this to be here because we can't instantiate a ChannelStrip that
        // can potentially also have ChannelStrip's in it
        id: channelStripDelegate

        ChannelStrip { node: modelData; height: parent?.height }
    }

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

        boundsBehavior: Flickable.StopAtBounds
        boundsMovement: Flickable.StopAtBounds

        maximumFlickVelocity: 1

        delegate: DelegateChooser
        {
            id: chooser
            role: "type"

            DelegateChoice { roleValue: 1; delegate: channelStripDelegate }
            DelegateChoice { roleValue: 3; O.MidiFilePlayer { node: modelData } }
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
}
