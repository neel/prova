#include "exuvulnerabilitiesviewer.h"
#include "ui_exuvulnerabilitiesviewer.h"
#include <QNetworkReply>
#include <iostream>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include "cvelistmodel.h"
#include "exuvulnerabilitiesprogresswidget.h"
#include <nlohmann/json.hpp>
#include "prova/artifact.h"

ExUVulnerabilitiesViewer::ExUVulnerabilitiesViewer(QNetworkAccessManager *network, QWidget *parent): QWidget(parent), ui(new Ui::ExUVulnerabilitiesViewer), _network(network){
    ui->setupUi(this);
    _cveModel = new CVEListModel{_network};

    _progressArea = new ExUVulnerabilitiesProgressWidget{this};
    QVBoxLayout* l = dynamic_cast<QVBoxLayout*>(layout());
    l->insertWidget(0, _progressArea);


    ui->quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    ui->quickWidget->engine()->addImportPath("qrc:/x");
    ui->quickWidget->engine()->rootContext()->setContextProperty("cveModel", _cveModel);
    ui->quickWidget->setSource(QUrl("qrc:/x/CVE/CVEResults.qml"));

    connect(_cveModel, &CVEListModel::searchFinished, this, &ExUVulnerabilitiesViewer::responseReceivedSlot);
}

ExUVulnerabilitiesViewer::~ExUVulnerabilitiesViewer(){
    delete ui;
}

void ExUVulnerabilitiesViewer::setUnit(std::shared_ptr<prova::execution_unit> unit){
    _unit = unit;
    for(auto artifact: *_unit){
        nlohmann::json artifact_properties = artifact->properties();
        if(artifact_properties.count("path") > 0){
            std::string path = artifact_properties["path"].get<std::string>();
            _paths.insert(QString::fromStdString(path));
        }
    }
    _progressArea->setMaxValue(_unit->artifacts_count());
    QString path = *_paths.begin();
    _cveModel->search(path);
    _progressArea->setLabel(path);
}

void ExUVulnerabilitiesViewer::filter(const QString &keyword){

}

void ExUVulnerabilitiesViewer::responseReceivedSlot(const QString &keyword){
    _paths.remove(keyword);
    _progressArea->updateProgress(_unit->artifacts_count() - _paths.size());
    updateGeometry();
    adjustSize();

    if(!_paths.isEmpty()){
        QString path = *_paths.begin();
        _cveModel->search(path);
        _progressArea->setLabel(path);
    } else {
        _progressArea->hide();
    }
}

