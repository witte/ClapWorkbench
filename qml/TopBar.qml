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
    }
}
