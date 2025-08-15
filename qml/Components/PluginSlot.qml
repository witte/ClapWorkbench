import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window 2.2
import ClapWorkbench
import "." as O


MouseArea
{
    id: control

    property QtObject modelData
    property int buttonHeight: height + 1

    height: 21

    hoverEnabled: true

    onPressed: (event) => { event.accepted = false }


    O.Button
    {
        id: btnPinToMainWindow

        width: 26
        height: control.buttonHeight

        roundRight: false
        enabled: false

        text: "\u2190"

        checked: app.mainWindowPlugin === control.modelData
    }

    MouseArea
    {
        anchors.fill: btnPinToMainWindow

        onPressed: (event) =>
        {
            app.mainWindowPlugin = control.modelData
            event.accepted = false
        }
    }

    Item
    {
        property double buttonWidth: width / 4

        anchors.fill: parent
        anchors.leftMargin: btnPinToMainWindow.width
        anchors.bottomMargin: -1

        O.Button
        {
            anchors.fill: parent

            roundLeft: false

            text: control.modelData?.name

            visible: !control.containsMouse

            opacity: control.modelData?.isByPassed? 0.4 : 1
        }

        Row
        {
            visible: control.containsMouse

            O.Button
            {
                id: btnOpenBrowser

                width: parent.parent.buttonWidth
                height: control.buttonHeight

                roundLeft: false
                roundRight: false

                font.pointSize: 23
                textVerticalOffset: -0.5

                text: "\u2315"

                onClicked: app.openPluginBrowserWindow(control.modelData)
            }

            O.Button
            {
                id: btnPlugin

                width: parent.parent.buttonWidth
                height: control.buttonHeight

                roundLeft: false
                roundRight: false

                font.family: "icons"
                font.pointSize: 16

                text: "\ue900"

                checked: control.modelData? control.modelData.isFloatingWindowOpen : false

                onClicked:
                {
                    if (control.modelData === null)
                        return

                    control.modelData.isFloatingWindowOpen = true
                }
            }

            O.Button
            {
                id: btnUnload

                width: parent.parent.buttonWidth
                height: control.buttonHeight

                onClicked: audioEngine.unload(control.modelData)

                roundLeft: false
                roundRight: false

                font.pointSize: 12
                text: "X"
            }

            O.Button
            {
                id: btnBypass

                width: parent.parent.buttonWidth
                height: control.buttonHeight

                checked: control.modelData? control.modelData.isByPassed : false

                onClicked:
                {
                    if (control.modelData === null)
                        return

                    control.modelData.isByPassed = !control.modelData.isByPassed
                }

                roundLeft: false

                font.pointSize: 12
                text: "\u25CB"
            }
        }
    }
}
