import QtQuick 2.15
import QtQuick.Controls 2.15
import ClapWorkbench


MouseArea
{
    id: control

    property QtObject draggedItem
    property list<PluginHost> model
    property Component delegate
    property int draggedItemSpacing: 4

    signal itemsReordered(int fromIndex, int toIndex)

    Column
    {
        anchors.fill: parent

        Repeater
        {
            id: repeater

            model: control.model
            
            delegate: Item
            {
                id: delegateItem

                property int delegateIndex: index
                property bool animationEnabled: false

                width: loader.item?.width
                height: loader.item?.height

                Loader
                {
                    id: loader

                    sourceComponent: control.delegate

                    onLoaded: item.modelData = modelData
                }

                Behavior on y
                {
                    enabled: delegateItem.animationEnabled

                    NumberAnimation
                    {
                        duration: 75
                    }
                }
            }
        }
    }


    function adjustHeights()
    {
        let draggedItemVCenter = control.draggedItem.y + (control.draggedItem.height * 0.5)
        let pushDelegatesDownFromNowOn = false
        let indexWithoutDraggedItem = 0

        for (let i = 0; i < repeater.count; ++i)
        {
            let item = repeater.itemAt(i)

            if (item === control.draggedItem)
                continue

            let itemNewY = item.height * indexWithoutDraggedItem

            if (pushDelegatesDownFromNowOn)
            {
                item.y = itemNewY + item.height + control.draggedItemSpacing + control.draggedItemSpacing
                ++indexWithoutDraggedItem

                continue
            }

            let referenceVCenter = itemNewY + item.height + control.draggedItemSpacing

            if (draggedItemVCenter > referenceVCenter)
            {
                item.y = itemNewY
            }
            else
            {
                item.y = itemNewY + item.height + control.draggedItemSpacing + control.draggedItemSpacing
                pushDelegatesDownFromNowOn = true
            }

            ++indexWithoutDraggedItem
        }
    }

    onPositionChanged: (posChangedEvent) =>
    {
        if (!control.draggedItem)
        {
            let itemToDrag

            for (let i = 0; i < repeater.count; ++i)
            {
                let item = repeater.itemAt(i)

                if (item.y <= posChangedEvent.y && item.y + item.height >= posChangedEvent.y)
                {
                    itemToDrag = item
                    break
                }
            }

            if (!itemToDrag)
                return

            control.draggedItem = itemToDrag
            control.draggedItem.z = 1000
            control.draggedItem.animationEnabled = false
        }

        control.draggedItem.y = posChangedEvent.y - draggedItem.height / 2
        adjustHeights()
    }

    onReleased:
    {
        if (!control.draggedItem)
            return

        let targetIndex = -1

        for (let i = 0; i < repeater.count; ++i)
        {
            let item = repeater.itemAt(i)
            if (item === control.draggedItem || item.y < control.draggedItem.y)
                continue

            targetIndex = i
            break
        }

        if (targetIndex === -1)
            targetIndex = repeater.count

        if (control.draggedItem.delegateIndex < targetIndex)
            targetIndex--

        control.itemsReordered(control.draggedItem.delegateIndex, targetIndex)

        for (let i = 0; i < repeater.count; ++i)
        {
            let item = repeater.itemAt(i)
            item.z = 0
            item.y = item.height * i
            item.animationEnabled = true
        }

        control.draggedItem = null
    }
}
