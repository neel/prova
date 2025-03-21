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

ExUVulnerabilitiesViewer::ExUVulnerabilitiesViewer(QWidget *parent): QWidget(parent), ui(new Ui::ExUVulnerabilitiesViewer){
    ui->setupUi(this);
    _network = new QNetworkAccessManager(this);
    _cveModel = new CVEListModel{_network};

    _quickWidget = new QQuickWidget(this);
    _quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    _quickWidget->engine()->addImportPath("qrc:/x");
    _quickWidget->engine()->rootContext()->setContextProperty("cveModel", _cveModel);
    _quickWidget->setSource(QUrl("qrc:/x/CVE/CVEResults.qml"));

    ui->centralLayout->addWidget(_quickWidget);
}

ExUVulnerabilitiesViewer::~ExUVulnerabilitiesViewer(){
    delete ui;
}

void ExUVulnerabilitiesViewer::request(const QString &keyword){
    _cveModel->search(keyword);

}

void ExUVulnerabilitiesViewer::filter(const QString &keyword){

}

