import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T


T.Button
{
    id: control

    property bool roundLeft: true
    property bool roundRight: true
    property double textVerticalOffset: 0

    property color bgColor: "#475460"
    property color bgPressedColor: "#262A2E"

    property color bgGradientStartColor: "#4f5f6c"
    property color bgGradientStartPressedColor: "#24282c"

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
        implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
        implicitContentHeight + topPadding + bottomPadding)

    padding: 6
    horizontalPadding: padding + 2
    spacing: 6

    font.pointSize: 14


    Rectangle
    {
        anchors.fill: parent

        topLeftRadius:     control.roundLeft?  5 : 0
        bottomLeftRadius:  control.roundLeft?  5 : 0
        topRightRadius:    control.roundRight? 5 : 0
        bottomRightRadius: control.roundRight? 5 : 0

        color: "#aA262A2E"

        Rectangle
        {
            anchors.fill: parent
            anchors.leftMargin: 1
            anchors.rightMargin: control.roundRight? 1 : 0
            anchors.topMargin: 1
            anchors.bottomMargin: 1

            topLeftRadius:     control.roundLeft?  4 : 0
            bottomLeftRadius:  control.roundLeft?  4 : 0
            topRightRadius:    control.roundRight? 4 : 0
            bottomRightRadius: control.roundRight? 4 : 0

            gradient: Gradient
            {
                GradientStop
                {
                    position: 0.0;
                    color: control.pressed || control.checked? control.bgGradientStartPressedColor : control.bgGradientStartColor
                }

                GradientStop
                {
                    position: 0.3;
                    color: control.pressed || control.checked? control.bgPressedColor : control.bgColor
                }
            }
        }


        Rectangle
        {
            x: 2
            y: 2
            width:  control.width  - (!control.roundRight? 3 : 4)
            height: control.height - 4

            topLeftRadius:     control.roundLeft?  3 : 0
            bottomLeftRadius:  control.roundLeft?  3 : 0
            topRightRadius:    control.roundRight? 3 : 0
            bottomRightRadius: control.roundRight? 3 : 0

            color: control.pressed || control.checked? control.bgPressedColor : control.bgColor
        }
    }

    contentItem: Text
    {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: control.textVerticalOffset

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        color: control.pressed || control.checked? "#dddddd" : "#ffffff"
        font: control.font

        elide: Text.ElideRight

        text: control.text
    }
}
