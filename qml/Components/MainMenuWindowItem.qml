import QtQuick


MouseArea
{
    id: control

    property string text

    hoverEnabled: true

    height: 24
    width: parent.width

    Rectangle
    {
        anchors.fill: parent
        anchors.margins: 2

        color: parent.pressed
                    ? "#5fffffff"
                    : parent.containsMouse
                        ? "#3fffffff"
                        : "transparent"

        Text
        {
            x: 6
            text: control.text

            anchors.verticalCenter: parent.verticalCenter

            elide: Text.ElideRight
            color: "#ffffff"
        }
    }
}
