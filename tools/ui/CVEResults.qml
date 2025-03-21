import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CVE 1.0


Item {
    visible: true
    width: 640
    height: 480

    ScrollView {
        anchors.fill: parent

        ListView {
            id: listView
            anchors.fill: parent
            spacing: 10
            model: cveModel

            delegate: CVECard {
                cveId:          model.id
                datePublished:  model.details["cveMetadata"] ? model.details["cveMetadata"]["datePublished"] : ""
                dateReserved:   model.details["cveMetadata"] ? model.details["cveMetadata"]["dateReserved"] : ""
                dateUpdated:    model.details["cveMetadata"] ? model.details["cveMetadata"]["dateUpdated"] : ""
                provider:       model.details["providerMetadata"] ? model.details["providerMetadata"]["shortName"] : ""
                assigner:       model.details["providerMetadata"] ? model.details["providerMetadata"]["assignerShortName"] : ""
                descriptions:   model.details["containers"] ? model.details["containers"]["cna"]["descriptions"] : ""
            }
        }
    }
}
