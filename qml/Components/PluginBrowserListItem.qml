import QtQuick
import QtQuick.Controls.Basic


MouseArea
{
    height: 30

    hoverEnabled: true

    Rectangle
    {
        anchors.fill: parent

        color: parent.pressed
                    ? "#5fffffff"
                    : parent.containsMouse
                        ? "#3fffffff"
                        : index % 2 === 0
                            ? "#18ffffff"
                            : "#1fffffff"

        PluginBrowserListItemText
        {
            id: name

            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.right: version.left

            text: modelData.name
            toolTip: modelData.name + " (" + modelData.path + ")"
        }

        PluginBrowserListItemText
        {
            id: version

            width: 60
            anchors.right: vendor.left

            text: modelData.version
        }

        PluginBrowserListItemText
        {
            id: vendor

            width: 120
            anchors.right: features.left

            text: modelData.vendor
        }

        PluginBrowserListItemText
        {
            id: features

            width: 160
            anchors.right: parent.right

            text: modelData.features
        }
    }

    onClicked:
    {
        if (Window.window)
            Window.window.close()

        audioEngine.load(control.plugin, modelData.path, modelData.index)
    }
}
