import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Helpers 1.0

Rectangle {
    id: card
    property string cveId: ""
    property string datePublished: ""
    property string dateReserved: ""
    property string dateUpdated: ""
    property string provider: ""
    property string assigner: ""
    property var    descriptions: []
    property var    artifacts: []
    property var    metrics: {}

    // color: "green"
    border.color: "#e1e1e1"
    radius: 5

    anchors.margins: 20
    width: parent.width-20
    height: childrenRect.height + 20

    Rectangle{
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        anchors.leftMargin: 10
        anchors.rightMargin: 30
        width: parent.width - 40
        height: childrenRect.height + 20
        // color: "red"

        ColumnLayout {
            spacing: 10
            width: parent.width

            RowLayout {
                width: parent.width
                spacing: 10

                Text {
                    // width: parent.width
                    text: card.cveId
                    font.bold: true
                    font.pixelSize: 20
                }

                Rectangle {
                    width: 130
                    height: 30
                    radius: 15
                    // border.color: "#606060"
                    border.width: 0
                    visible: card.metrics.version !== undefined
                    color: card.metrics.data ? getColorForScore(card.metrics.data["baseScore"]).background : "#5050b4"
                    Text {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.leftMargin: 8
                        anchors.topMargin: 4
                        text: card.metrics.data ? card.metrics.version : ""
                        font.pixelSize: 16
                        font.capitalization: Font.AllLowercase
                        color: (card.metrics && card.metrics.data && "baseScore" in card.metrics.data && card.metrics.data["baseScore"] !== undefined) ? getColorForScore(card.metrics.data["baseScore"]).foreground : Qt.rgba(1, 1, 1, 1)
                    }

                    Rectangle {
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.rightMargin: 4
                        anchors.topMargin: 3
                        width: 50
                        height: 25
                        radius: 15
                        // border.color: "#606060"
                        border.width: 0
                        color: Qt.rgba(1, 1, 1, 1)
                        Text {
                            anchors.centerIn: parent
                            text: (card.metrics && card.metrics.data && "baseScore" in card.metrics.data && card.metrics.data["baseScore"] !== undefined) ? card.metrics.data["baseScore"] : ""
                            font.pixelSize: 15
                            font.bold: true
                        }
                    }
                }

            }

            RowLayout {
                width: parent.width
                spacing: 10
                Rectangle{
                    // color: "yellow"
                    radius: 5
                    width: childrenRect.width
                    height: childrenRect.height
                    // color: "#f0f0f0"
                    Column {
                        Text{text: "Published"}
                        Text{
                            text: card.datePublished
                        }
                    }
                }

                Rectangle{
                    // color: "yellow"
                    radius: 5
                    width: childrenRect.width
                    height: childrenRect.height
                    // color: "#f0f0f0"
                    Column {
                        Text{text: "Reserved"}
                        Text{
                            text: card.dateReserved
                        }
                    }
                }

                Rectangle{
                    // color: "yellow"
                    radius: 5
                    width: childrenRect.width
                    height: childrenRect.height
                    // color: "#f0f0f0"
                    Column {
                        Text{text: "Updated"}
                        Text{
                            text: card.dateUpdated
                        }
                    }
                }
            }



            // // Provider and Assigner:
            // // If provider and assigner are the same (or assigner is empty), show only provider.
            // Text {
            //     text: (card.provider === card.assigner || card.assigner === "") ?
            //           ("Provider: " + card.provider) :
            //           ("Provider: " + card.provider + " | Assigner: " + card.assigner)
            // }

            Row {
                width: parent.width
                spacing: 10
                anchors.leftMargin: 20
                anchors.rightMargin: 0
                Repeater {
                    model: card.descriptions
                    delegate: Rectangle {
                        width: parent.width
                        color: "#f8f8f8"
                        radius: 5
                        TextEdit {
                            id: descText
                            readOnly: true
                            selectByMouse: true
                            width: parent.width
                            text: modelData.value
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                            padding: 10
                        }
                        implicitHeight: descText.implicitHeight + 10
                    }
                }
            }

            RowLayout {
                width: parent.width
                spacing: 10
                anchors.leftMargin: 20
                Repeater {
                    model: card.artifacts
                    delegate: Rectangle {
                        id: artifactRect
                        radius: 10
                        width: 70
                        height: 20
                        color: ColorHelper.colorForPath(modelData)
                        border.color: "#cccccc"
                        border.width: 1
                        // implicitHeight: childrenRect.height + 10

                        HoverHandler {
                            id: hoverHandler
                        }

                        ToolTip {
                            visible: hoverHandler.hovered
                            text: modelData
                            delay: 100
                            timeout: 5000
                        }


                        Component.onCompleted: {
                            console.log("Path:", modelData, "Color:", ColorHelper.colorForPath(modelData));
                        }
                    }
                }
            }
        }
    }

    // function getColorForScore(score) {
    //     var red = Math.min(255, 510 * score / 10);
    //     var green = Math.max(0, 510 - 510 * score / 10);
    //     return Qt.rgba(red / 255, green / 255, 0, 1);
    // }

    function getColorForScore(score) {
        let red = Math.min(255, 510 * score / 10);
        let green = Math.max(0, 510 - 510 * score / 10);
        let blue = 0; // Static blue component for simplicity
        let background = Qt.rgba(red / 255, green / 255, blue / 255, 1);

        // Calculate the luminance of the background using the formula for relative luminance
        let luminance = 0.299 * (red / 255) + 0.587 * (green / 255) + 0.114 * (blue / 255);
        let foreground;

        // If the background is light, use black text; if dark, use white text
        if (luminance > 0.5) {
            foreground = Qt.rgba(0, 0, 0, 1); // Black for light backgrounds
        } else {
            foreground = Qt.rgba(1, 1, 1, 1); // White for dark backgrounds
        }

        return { background: background, foreground: foreground };
    }
}

