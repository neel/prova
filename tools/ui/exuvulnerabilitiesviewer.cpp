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

ExUVulnerabilitiesViewer::ExUVulnerabilitiesViewer(QWidget *parent): QWidget(parent), ui(new Ui::ExUVulnerabilitiesViewer){
    _network = new QNetworkAccessManager(this);
    ui->setupUi(this);

    _quickWidget = new QQuickWidget(this);
    _quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    _quickWidget->engine()->addImportPath("qrc:/x");
    _quickWidget->setSource(QUrl("qrc:/x/CVE/CVEResults.qml"));

    ui->centralLayout->addWidget(_quickWidget);

    connect(this, &ExUVulnerabilitiesViewer::jsonReady, this, &ExUVulnerabilitiesViewer::updateJsonData);
}

ExUVulnerabilitiesViewer::~ExUVulnerabilitiesViewer(){
    delete ui;
}

void ExUVulnerabilitiesViewer::request(const QString &keyword){
    QString url = QString("https://cve.mitre.org/cgi-bin/cvekey.cgi?keyword=%1").arg(keyword);
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20100101 Firefox/10.0");

    QNetworkReply* reply = _network->get(request);
    connect(reply, &QNetworkReply::readyRead, this, &ExUVulnerabilitiesViewer::replyReceived);
}

void ExUVulnerabilitiesViewer::updateJsonData(const QVariant& data){
    QObject* root = _quickWidget->rootObject();
    if (root) {
        bool ok = QMetaObject::invokeMethod(root, "addCveData", Q_ARG(QVariant, data));
        if (!ok)
            qWarning() << "Failed to invoke addCveData on QML root object.";
        else
            qDebug() << "Inserted new CVE record into QML ListModel.";
    } else {
        qWarning() << "Root object not found!";
    }

    qDebug() << "Updating CVE Viewer with JSON";
}

void ExUVulnerabilitiesViewer::replyReceived(){
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        QByteArray responseData = reply->readAll();
        QString responseString = QString::fromUtf8(responseData);

        QStringList results;
        const QString lookup = "https://www.cve.org/CVERecord?id=CVE-";
        QRegularExpression regex("https://www\\.cve\\.org\\/CVERecord\\?id=(\\w+-\\w+-\\w+)");

        QRegularExpressionMatch match;
        QStringList lines = responseString.split("\n");
        for (const QString &line : lines) {
            if (line.contains(lookup)) {
                match = regex.match(line);
                if (match.hasMatch()) {
                    QString id = match.captured(1);
                    results << id;
                }
            }
        }

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Network error: " << reply->errorString();
        }

        for(const QString& result: results){
            if(_cves.contains(result))
                continue;
            _cves.insert(result);
            QNetworkRequest request;
            request.setUrl(QUrl(QString("https://cveawg.mitre.org/api/cve/%1").arg(result)));
            request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20100101 Firefox/10.0");
            QNetworkReply* reply = _network->get(request);
            connect(reply, &QNetworkReply::readyRead, this, &ExUVulnerabilitiesViewer::cveReplyReceived);
        }

        reply->deleteLater();
    }
}

void ExUVulnerabilitiesViewer::cveReplyReceived(){
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        std::cout << "JSON reply received" << std::endl;
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        qDebug() << jsonDoc;
         if (!jsonDoc.isNull() && jsonDoc.isObject()) {
            QVariant jsonData = jsonDoc.object().toVariantMap();
            emit jsonReady(jsonData);
        }

    }
}
