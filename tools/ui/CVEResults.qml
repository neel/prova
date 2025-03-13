import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CVE 1.0


Item {
    visible: true
    width: 640
    height: 480

    ListModel {
        id: cveListModel
    }

    function addCveData(newCve) {
        cveListModel.append(newCve);
    }

    ScrollView {
        anchors.fill: parent

        ListView {
            id: listView
            anchors.fill: parent
            spacing: 10
            model: cveListModel

            delegate: CVECard {
                cveId:          model.cveMetadata.cveId
                datePublished:  model.cveMetadata.datePublished
                dateReserved:   model.cveMetadata.dateReserved
                dateUpdated:    model.cveMetadata.dateUpdated
                provider:       model.providerMetadata ? model.providerMetadata.shortName : ""
                assigner:       model.assignerShortName ? model.assignerShortName : ""
                descriptions:   model.containers.cna.descriptions
            }
        }
    }
}
