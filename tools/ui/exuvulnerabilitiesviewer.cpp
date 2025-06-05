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
#include "keywordlistmodel.h"
#include "cveproxymodel.h"
#include "exuvulnerabilitiesprogresswidget.h"
#include <nlohmann/json.hpp>
#include "prova/artifact.h"
#include "directorytrie.h"
#include <QFileInfo>
#include <QDir>
#include <QTimer>

ExUVulnerabilitiesViewer::ExUVulnerabilitiesViewer(QNetworkAccessManager *network, QWidget *parent): QWidget(parent), ui(new Ui::ExUVulnerabilitiesViewer), _network(network){
    ui->setupUi(this);
    _keywordsModel = new KeywordListModel{this};
    _cveModel = new CVEListModel{_network, *_keywordsModel};
    _cveFilterModel = new CVEProxyModel;
    _cveFilterModel->setSourceModel(_cveModel);
    _cveFilterModel->setSortRole(CVEListModel::IdRole);
    _cveFilterModel->sort(0, Qt::DescendingOrder);
    _cveFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    connect(_keywordsModel, &KeywordListModel::dataChanged, _cveFilterModel, &CVEProxyModel::invalidate);

    _progressArea = new ExUVulnerabilitiesProgressWidget{this};
    QVBoxLayout* l = dynamic_cast<QVBoxLayout*>(layout());
    l->insertWidget(0, _progressArea);

    ui->quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    ui->quickWidget->engine()->addImportPath("qrc:/x");
    ui->quickWidget->engine()->rootContext()->setContextProperty("cveModel", _cveFilterModel);
    ui->quickWidget->engine()->rootContext()->setContextProperty("keywordModel", _keywordsModel);
    ui->quickWidget->setSource(QUrl("qrc:/x/CVE/CVEResults.qml"));

    connect(_cveModel, &CVEListModel::searchFinished, this, &ExUVulnerabilitiesViewer::responseReceivedSlot);
    connect(_cveModel, &CVEListModel::searchSlowdown, this, &ExUVulnerabilitiesViewer::delayNextSearch);
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
    DirectoryTrie  dirTrie;
    for (auto artifact : *_unit) {
        const std::string subtype = artifact->subtype();
        const nlohmann::json props = artifact->properties();

        if (props.contains("path")) {
            const QString qPath   = QString::fromStdString(props["path"].get<std::string>()).trimmed();
            qDebug() << qPath;
            if (subtype == "file") {
                const QString fileName = QFileInfo(qPath).fileName();
                if (!fileName.isEmpty())
                    _paths.insert(qPath, fileName);
            } else if (subtype == "directory") {
                const QStringList segments = qPath.split(QDir::separator(), Qt::SkipEmptyParts);

                if (!segments.isEmpty())
                    dirTrie.insert(segments);
            }
            /* else: ignore sockets/pipes/whatever */
        }
    }

    // TODO enrich the unit with these mappings

    QMap<QString, QString> dir_paths_map = dirTrie.suffixesMap(0);
    _paths.insert(dir_paths_map);

    _progressArea->setMaxValue(_unit->artifacts_count());
    if(_paths.size() > 0) {
        QString fullpath = _paths.begin().key();
        QString path = _paths.begin().value();
        _cveModel->search(fullpath, path);
        _progressArea->setLabel(path);
    }
}

void ExUVulnerabilitiesViewer::filter(const QString& fullpath){
    if(_paths.count(fullpath)){
        QString query = _paths[fullpath];
        qDebug() << "Filter " << fullpath << " -> " << query;
        ui->searchEdit->setText(query);
    } else {
        qDebug() << "Filter " << fullpath;
        ui->searchEdit->setText(fullpath);
    }
}

void ExUVulnerabilitiesViewer::responseReceivedSlot(const QString& keyword){
    _paths.remove(keyword);
    _progressArea->updateProgress(_unit->artifacts_count() - _paths.size());

    updateGeometry();
    adjustSize();

    searchNext();
}

void ExUVulnerabilitiesViewer::delayNextSearch(){
    QTimer::singleShot(30 * 1000, this, [this]() {
        searchNext();
    });
}

void ExUVulnerabilitiesViewer::searchNext(){
    if(!_paths.isEmpty()){
        QString fullpath = _paths.begin().key();
        QString path = _paths.begin().value();
        _cveModel->search(fullpath, path);
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

