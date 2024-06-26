import QtQuick
import QtQuick.Controls.Basic
import ClapWorkbench


ListView
{
    id: control

    property QtObject plugin
    property int rowItemWidth: 182

    clip: true

    width: 600
    height: childrenRect.height > 552? 552 : childrenRect.height

    model: plugin === null? null : plugin.parameters

    delegate: Item
    {
        width: control.width
        height: 24

        Item
        {
            anchors.horizontalCenter: parent.horizontalCenter
            width: rowItemWidth * 2
            height: 24

            Text
            {
                width: rowItemWidth
                anchors.verticalCenter: parent.verticalCenter

                horizontalAlignment: Text.AlignRight
                text: model.name
            }

            Slider
            {
                x: rowItemWidth
                width: rowItemWidth
                anchors.verticalCenter: parent.verticalCenter

                property int parameterId: model.id

                from: model.min
                to: model.max
                enabled: !model.readOnly

                stepSize: model.stepped ? 1.0 : 0.0
                snapMode: model.stepped ? Slider.SnapAlways : Slider.NoSnap

                onValueChanged: control.model.setValue(index, parameterId, value)
                value: model.value

                onPressedChanged: pressed ? control.model.startGesture(parameterId) :
                    control.model.stopGesture(parameterId)

                ToolTip.visible: pressed
                ToolTip.text: control.model.getTextValue(parameterId, value)
            }
        }
    }
}
