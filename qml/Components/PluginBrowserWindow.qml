import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import ClapWorkbench
import "." as O


Rectangle
{
    id: control

    property QtObject plugin

    property int targetHeight: pluginsList.y + pluginsList.contentHeight + bottomBar.height
    property int lastHeight: targetHeight

    property int minimumHeight: 120
    property int maximumHeight: 520

    width: 720
    height: targetHeight > maximumHeight? maximumHeight : targetHeight < minimumHeight? minimumHeight: targetHeight

    visible: true

    color: "transparent"

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

    Rectangle
    {
        anchors.fill: parent

        radius: 6

        color: "#f8171b1f"

        focus: true

        O.LightButton
        {
            id: closeButton

            x: 2
            y: 2
            width: 20
            height: 18

            bgHoveredColor: "#88000000"
            bgPressedColor: "#bb000000"

            textColor: "#dddddd"
            textPressedColor: "#bbbbbb"

            textVerticalOffset: -1

            text: "x"
            onClicked: Window.window.close()
        }

        Text
        {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: (-bottomBar.height / 2) + 4

            visible: !audioEngine.pluginManager.hasFoundPlugins

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            color: "#ffffff"

            text: "No CLAP plugins found"
        }


        ListView
        {
            id: pluginsList

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: closeButton.bottom
            anchors.bottom: bottomBar.top
            anchors.topMargin: 2
            anchors.leftMargin: 2
            anchors.rightMargin: 2

            clip: true

            model: audioEngine.pluginManager.availablePlugins

            delegate: PluginBrowserListItem
            {
                width: pluginsList.width
                height: 30
            }
        }

        Rectangle
        {
            id: bottomBar

            height: 40
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            bottomLeftRadius: 6
            bottomRightRadius: 6

            color: "#f8171b1f"

            Text
            {
                id: otherPathsLabel

                x: 6
                anchors.verticalCenter: parent.verticalCenter

                color: "#ffffff"

                text: "Other paths:"
            }

            Rectangle
            {
                height: 24
                anchors.left: otherPathsLabel.right
                anchors.right: rescanClapPaths.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 4

                color: "#e7e6e1"

                TextInput
                {
                    id: extraClapPaths

                    clip: true

                    anchors.fill: parent

                    verticalAlignment: TextInput.AlignVCenter

                    text: audioEngine.pluginManager.pathsToScan
                    onAccepted: browser.pathsToScan = text
                }
            }

            O.LightButton
            {
                id: rescanClapPaths

                width: 62
                height: 32
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 4

                bgColor: "green"
                bgHoveredColor: "#88000000"
                bgPressedColor: "#bb000000"

                textColor: "#dddddd"
                textPressedColor: "#bbbbbb"

                textVerticalOffset: -1

                text: "re-scan"

                onClicked:
                {
                    control.lastHeight = control.height

                    audioEngine.pluginManager.pathsToScan = extraClapPaths.text
                    audioEngine.pluginManager.rescanPluginPaths()

                    control.resetWindowHeight()
                }
            }
        }

        Keys.onPressed: (event) =>
        {
            if (event.key === Qt.Key_Escape)
                Window.window.close()
        }
    }

    function resetWindowHeight()
    {
        let newY = y - ((height - lastHeight) / 2)
        if (newY < 0)
            newY = 0

        y = newY
        lastHeight = height
    }
}
