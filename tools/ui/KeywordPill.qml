import QtQuick 2.15
import QtQuick.Controls 2.15
import Helpers 1.0      // for ColorHelper

Rectangle {
    id: pill
    property string keyword
    property int    count     : 0
    property bool   enabled   : true
    signal toggled(bool newState)

    radius: 8
    width: 50
    height: 18
    color: ColorHelper.colorForPath(keyword)
    border.color: "#cccccc"
    border.width: 1
    implicitWidth: textRow.paintedWidth + (enabled ? 16 : 30)

    Row {
        id: textRow
        anchors.centerIn: parent
        spacing: 4
        Text { text: enabled ? "" : "\u2715"; color: "white"; font.pixelSize: 14 }
        Text { text: count ; color: "white"; font.pixelSize: 12 }
    }

    HoverHandler {
        id: hoverHandler
    }

    ToolTip {
        visible: hoverHandler.hovered
        text: keyword
        delay: 100
        timeout: 5000
    }

    MouseArea {
        anchors.fill: parent
        onClicked: toggled(!enabled)
    }
}
