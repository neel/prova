import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: card
    property string cveId: ""
    property string datePublished: ""
    property string dateReserved: ""
    property string dateUpdated: ""
    property string provider: ""
    property string assigner: ""
    property var    descriptions: []

    width: parent.width
    height: childrenRect.height + 10
    color: "#f9f9f9"
    border.color: "#e1e1e1"
    radius: 5

    Rectangle{
        width: parent.width - 20
        anchors.centerIn: parent
        anchors.margins: 10
        height: childrenRect.height

        ColumnLayout {
            spacing: 10
            width: parent.width

            Text {
                width: parent.width
                text: card.cveId
                font.bold: true
                font.pixelSize: 20
            }

            RowLayout {
                width: parent.width
                spacing: 10
                Rectangle{
                    radius: 5
                    width: childrenRect.width
                    height: childrenRect.height
                    color: "#f0f0f0"
                    Column {
                        Text{text: "Published"}
                        Text{
                            text: card.datePublished
                        }
                    }
                }

                Rectangle{
                    radius: 5
                    width: childrenRect.width
                    height: childrenRect.height
                    color: "#f0f0f0"
                    Column {
                        Text{text: "Reserved"}
                        Text{
                            text: card.dateReserved
                        }
                    }
                }

                Rectangle{
                    radius: 5
                    width: childrenRect.width
                    height: childrenRect.height
                    color: "#f0f0f0"
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
                        color: "#ffffff"
                        border.color: "#cccccc"
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
        }
    }
}
