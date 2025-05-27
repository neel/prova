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
#include <QSortFilterProxyModel>
#include "cvelistmodel.h"
#include "cveproxymodel.h"
#include "exuvulnerabilitiesprogresswidget.h"
#include <nlohmann/json.hpp>
#include "prova/artifact.h"

ExUVulnerabilitiesViewer::ExUVulnerabilitiesViewer(QNetworkAccessManager *network, QWidget *parent): QWidget(parent), ui(new Ui::ExUVulnerabilitiesViewer), _network(network){
    ui->setupUi(this);
    _cveModel = new CVEListModel{_network};
    _cveFilterModel = new CVEProxyModel;
    _cveFilterModel->setSourceModel(_cveModel);
    _cveFilterModel->setSortRole(CVEListModel::IdRole);
    _cveFilterModel->sort(0, Qt::DescendingOrder);
    _cveFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    _progressArea = new ExUVulnerabilitiesProgressWidget{this};
    QVBoxLayout* l = dynamic_cast<QVBoxLayout*>(layout());
    l->insertWidget(0, _progressArea);


    ui->quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    ui->quickWidget->engine()->addImportPath("qrc:/x");
    ui->quickWidget->engine()->rootContext()->setContextProperty("cveModel", _cveFilterModel);
    ui->quickWidget->setSource(QUrl("qrc:/x/CVE/CVEResults.qml"));

    connect(_cveModel, &CVEListModel::searchFinished, this, &ExUVulnerabilitiesViewer::responseReceivedSlot);
    connect(ui->searchEdit, &QLineEdit::textChanged, this, &ExUVulnerabilitiesViewer::setFilterText);
    connect(_cveFilterModel, &QSortFilterProxyModel::dataChanged, this, &ExUVulnerabilitiesViewer::updateLabelCount);
    connect(_cveFilterModel, &QSortFilterProxyModel::modelReset, this, &ExUVulnerabilitiesViewer::updateLabelCount);
    connect(_cveFilterModel, &QSortFilterProxyModel::rowsInserted, this, &ExUVulnerabilitiesViewer::updateLabelCount);
    connect(_cveFilterModel, &QSortFilterProxyModel::rowsRemoved, this, &ExUVulnerabilitiesViewer::updateLabelCount);
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
            _paths.insert(QString::fromStdString(path).trimmed());
        }
    }
    _progressArea->setMaxValue(_unit->artifacts_count());
    QString path = *_paths.begin();
    _cveModel->search(path);
    _progressArea->setLabel(path);
}

void ExUVulnerabilitiesViewer::filter(const QString &keyword){
    qDebug() << "Filter " << keyword;
    ui->searchEdit->setText(keyword);
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

void ExUVulnerabilitiesViewer::setFilterText(const QString &text){
    QString adjustedPattern = text.trimmed();
    if (!adjustedPattern.isEmpty() && !adjustedPattern.startsWith("CVE") && !adjustedPattern.startsWith("cve")) {
        adjustedPattern += "$";
    }
    _cveFilterModel->setFilterRegularExpression(QRegularExpression(adjustedPattern, QRegularExpression::CaseInsensitiveOption));
}

void ExUVulnerabilitiesViewer::updateLabelCount(){
    int visibleCount = _cveFilterModel->rowCount();
    int totalCount = _cveModel->rowCount();
    ui->label->setText(QString("Showing %1 / %2").arg(visibleCount).arg(totalCount));
}

