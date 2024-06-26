import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T


T.Button
{
    id: control

    property bool roundLeft: true
    property bool roundRight: true

    property color bgColor: "transparent"
    property color bgHoveredColor: "#dadada"
    property color bgPressedColor: "#c4c4c4"

    property color textColor: "#515151"
    property color textPressedColor: "#454545"

    property double textVerticalOffset: 0

    width: 26
    height: 24

    opacity: enabled? 1.0 : 0.2

    font.pointSize: 14

    background: Rectangle
    {
        topLeftRadius:     control.roundLeft?  3 : 0
        bottomLeftRadius:  control.roundLeft?  3 : 0
        topRightRadius:    control.roundRight? 3 : 0
        bottomRightRadius: control.roundRight? 3 : 0

        color: control.pressed || control.checked? control.bgPressedColor
                : control.hovered? control.bgHoveredColor : control.bgColor
    }

    contentItem: Text
    {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: control.textVerticalOffset

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        color: control.pressed || control.checked? control.textPressedColor : control.textColor
        font: control.font

        elide: Text.ElideRight

        text: control.text
    }
}
