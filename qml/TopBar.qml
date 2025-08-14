import QtQuick
import QtQuick.Controls.Basic
import "Components" as O


WindowContainer {
    id: control

    width: parent.width
    height: 36

    window: Window {
        width: control.width
        height: control.height

        Rectangle {
            anchors.fill: parent

            color: "#404b56"
            border.color: "#30373f"
            border.width: 1
        }

        Row {
            spacing: 3
            padding: 4

            O.Button {
                id: settings

                width: 42
                height: 28
                textVerticalOffset: -1.5

                text: "\u2261"
                font.pointSize: 24

                checked: mainMenu.visible

                onClicked: {
                    let globalPos = control.parent.mapToGlobal(3, 4)

                    mainMenu.x = globalPos.x
                    mainMenu.y = globalPos.y
                    mainMenu.visible = !mainMenu.visible
                }

                O.MainMenuWindow {
                    id: mainMenu
                }
            }

            O.Button {
                id: toggleEngineRunning

                width: 42
                height: 28

                text: "\u23FB"
                font.bold: true

                checked: audioEngine.isRunning
                onClicked: audioEngine.isRunning = !audioEngine.isRunning
            }
        }

        O.Fader
        {
            id: faderBpm

            anchors.right: faderOutputVolume.left

            width: 162
            height: control.height

            from: 40.0
            to: 240.0

            value: audioEngine.bpm
            onPositionChanged: audioEngine.bpm = value

            orientation: Qt.Horizontal
        }

        O.Fader
        {
            id: faderOutputVolume

            anchors.right: isByPassedBtn.left

            width: 62
            height: control.height

            from: 0.0
            to: 2.0

            value: audioEngine.outputVolume
            onPositionChanged: audioEngine.outputVolume = value

            orientation: Qt.Horizontal
        }

        O.LightButton
        {
            id: isByPassedBtn

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 3

            bgHoveredColor: "#2f3338"
            bgPressedColor: "#262A2E"

            textColor: "#dddddd"
            textPressedColor: "#bbbbbb"

            font.pointSize: 14

            text: "M"

            checked: audioEngine.isByPassed
            onClicked: audioEngine.isByPassed = !audioEngine.isByPassed
        }
    }
}
