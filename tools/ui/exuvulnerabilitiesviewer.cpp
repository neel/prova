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

ExUVulnerabilitiesViewer::ExUVulnerabilitiesViewer(QWidget *parent): QWidget(parent), ui(new Ui::ExUVulnerabilitiesViewer){
    ui->setupUi(this);
    _network = new QNetworkAccessManager(this);
    _cveModel = new CVEListModel{_network};

    ui->quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    ui->quickWidget->engine()->addImportPath("qrc:/x");
    ui->quickWidget->engine()->rootContext()->setContextProperty("cveModel", _cveModel);
    ui->quickWidget->setSource(QUrl("qrc:/x/CVE/CVEResults.qml"));

    connect(_cveModel, &CVEListModel::searchFinished, this, &ExUVulnerabilitiesViewer::responseReceivedSlot);
}

ExUVulnerabilitiesViewer::~ExUVulnerabilitiesViewer(){
    delete ui;
}

void ExUVulnerabilitiesViewer::request(const QString &keyword){
    _cveModel->search(keyword);
    ExUVulnerabilitiesProgressWidget* progressWidget = new ExUVulnerabilitiesProgressWidget{keyword, this};
    _progressWidgets.insert(keyword, progressWidget);
    ui->progressLayout->addWidget(progressWidget);
}

void ExUVulnerabilitiesViewer::filter(const QString &keyword){

}

void ExUVulnerabilitiesViewer::responseReceivedSlot(const QString &keyword){
    auto it = _progressWidgets.find(keyword);
    if(it != _progressWidgets.end()){
        ExUVulnerabilitiesProgressWidget* progressWidget = it.value();
        ui->progressLayout->removeWidget(progressWidget);
        _progressWidgets.remove(keyword);
        delete progressWidget;
        progressWidget = 0x0;
    }
}

