import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// import CVE 1.0

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
                artifacts:      model.keywords
                datePublished:  model.details["cveMetadata"] ? model.details["cveMetadata"]["datePublished"] : ""
                dateReserved:   model.details["cveMetadata"] ? model.details["cveMetadata"]["dateReserved"] : ""
                dateUpdated:    model.details["cveMetadata"] ? model.details["cveMetadata"]["dateUpdated"] : ""
                provider:       model.details["providerMetadata"] ? model.details["providerMetadata"]["shortName"] : ""
                assigner:       model.details["providerMetadata"] ? model.details["providerMetadata"]["assignerShortName"] : ""
                descriptions:   model.details["containers"] ? model.details["containers"]["cna"]["descriptions"] : ""
                metrics: {
                    let containers = model.details["containers"];
                    let adp = containers["adp"];
                    let cna = containers["cna"];

                    let extract_cvss = function(obj){
                        for (let key in obj) {
                            if (key.startsWith("cvss") && obj[key].hasOwnProperty("baseScore")) {
                                // console.log("Found", model.id, JSON.stringify(obj))
                                return { "version": key.replace('_', '.'), "data": obj[key] };
                            } else {
                                // console.log("Not found", model.id, JSON.stringify(obj))
                            }
                        }
                        return false;
                    }

                    for(let a in adp){
                        if(a.hasOwnProperty("metrics")){
                            for(let m in a["metrics"]){
                                let metrics = a["metrics"][m]
                                let cvss = extract_cvss(metrics);
                                if(cvss !== false){
                                    return cvss;
                                }
                            }
                        }
                    }
                    if(cna.hasOwnProperty("metrics")){
                        for(let m in cna["metrics"]){
                            let metrics = cna["metrics"][m]
                            let cvss = extract_cvss(metrics);
                            if(cvss !== false){
                                return cvss;
                            }
                        }
                    }

                    return {};
                }
            }
        }
    }
}
