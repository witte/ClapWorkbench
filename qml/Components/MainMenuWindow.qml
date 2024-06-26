import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import ClapWorkbench
import "." as O


Window
{
    id: control

    flags: Qt.FramelessWindowHint

    width: menuHolder.width
    height: menuHolder.height

    // parent: null
    transientParent: null

    color: "transparent"

    Rectangle
    {
        id: menuHolder

        width: childrenRect.width
        height: childrenRect.height

        radius: 3

        color: "#404b56"

        clip: true

        Column
        {
            id: menu

            width: 220
            height: childrenRect.height

            MainMenuWindowItem
            {
                text: "New session"
                onClicked:
                {
                    app.newSession()
                    control.close()
                }
            }

            MainMenuWindowItem
            {
                text: "Load session"
                onClicked: mainControl.openLoadSessionDialog()
            }

            MainMenuWindowItem
            {
                text: "Save session"
                onClicked:
                {
                    if (app.isNewSession())
                    {
                        mainControl.openSaveSessionAsDialog()
                        return
                    }

                    app.saveSession()
                    control.close()
                }
            }

            MainMenuWindowItem
            {
                text: "Save session as"
                onClicked: mainControl.openSaveSessionAsDialog()
            }

            Item
            {
                height: 9
                width: parent.width

                Rectangle
                {
                    x: 4
                    y: 4
                    height: 1
                    width: parent.width - 8

                    color: "#30373f"
                }
            }

            MainMenuWindowItem
            {
                text: "Quit"
                onClicked: app.exit()
            }
        }
    }

    onActiveChanged:
    {
        if (!active)
            control.close()
    }
}