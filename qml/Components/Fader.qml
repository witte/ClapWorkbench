import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.Slider
{
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
        implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
        implicitHandleHeight + topPadding + bottomPadding)

    padding: 6

    background: Rectangle
    {
        x: control.leftPadding + (control.horizontal ? 0 : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : 0)
        implicitWidth: control.horizontal ? 200 : 6
        implicitHeight: control.horizontal ? 6 : 200
        width: control.horizontal ? control.availableWidth : implicitWidth
        height: control.horizontal ? implicitHeight : control.availableHeight
        radius: 3
        color: control.palette.dark
        scale: control.horizontal && control.mirrored ? -1 : 1
        border.color: "#30373f"

        Rectangle
        {
            y: control.horizontal ? 0 : control.visualPosition * parent.height
            width: control.horizontal ? control.position * parent.width : 6
            height: control.horizontal ? 6 : control.position * parent.height

            radius: 3
            color: control.palette.midlight
            border.color: "#30373f"
        }
    }

    handle: Rectangle
    {
        x: control.leftPadding + (control.horizontal ? control.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.visualPosition * (control.availableHeight - height))
        width: control.horizontal ? 14 : 28
        height: control.horizontal ? 28 : 14
        radius: 3
        color: control.pressed ? control.palette.light : control.palette.window
        border.width: control.visualFocus ? 2 : 1
        border.color: "#30373f"
    }
}