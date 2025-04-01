#include "cvelistmodel.h"
#include <QNetworkReply>
#include <QJsonDocument>


bool operator==(const CVEEntry &entry, const QString &id){
    return entry.id == id;
}


CVEListModel::CVEListModel(QNetworkAccessManager* network, QObject* parent): QAbstractListModel(parent){
    _network = network;
}

int CVEListModel::rowCount(const QModelIndex &parent) const{
    if (parent.isValid())
        return 0;

    return _entries.size();
}

QVariant CVEListModel::data(const QModelIndex &index, int role) const{
    if (!index.isValid() || index.row() < 0 || index.row() >= _entries.size())
        return QVariant();

    const CVEEntry &entry = _entries[index.row()];
    switch (role) {
    case KeywordRole:
        return entry.keywords;
    case IdRole:
        return entry.id;
    case DetailsRole:
        return QVariant::fromValue(entry.details.toVariantMap());
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> CVEListModel::roleNames() const{
    static QHash<int, QByteArray> roles {
        { KeywordRole, "keywords" },
        { IdRole,      "id" },
        { DetailsRole, "details" }
    };
    return roles;
}

void CVEListModel::search(const QString &keyword){
    QString url = QString("https://cve.mitre.org/cgi-bin/cvekey.cgi?keyword=%1").arg(keyword);
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20100101 Firefox/10.0");
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

    QNetworkReply* reply = _network->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply, keyword](){
        replyReceived(keyword, reply);
    });
}

void CVEListModel::replyReceived(const QString& keyword, QNetworkReply *reply){
    if(reply->error() != QNetworkReply::NoError){
        qDebug() << "Error " << reply->error() << " while searching for " << keyword;
        return;
    }

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

    emit searchFinished(keyword);

    for(const QString& cve_id: results){
        if(_cve_ids.contains(cve_id)){
            auto index = _entries.indexOf(cve_id);
            if(index != -1){
                _entries[index].keywords << keyword;
                QModelIndex startIndex = createIndex(index, 0);
                QModelIndex endIndex = createIndex(index, 0);
                emit dataChanged(startIndex, endIndex);
            }

            continue;
        }

        fetchCVEDetails(keyword, cve_id);
        _cve_ids.insert(cve_id);
    }

    reply->deleteLater();
}

void CVEListModel::fetchCVEDetails(const QString& keyword, const QString& cve_id) {
    QNetworkRequest request;
    request.setUrl(QUrl(QString("https://cveawg.mitre.org/api/cve/%1").arg(cve_id)));
    request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20100101 Firefox/10.0");
    QNetworkReply* reply = _network->get(request);
    connect(reply, &QNetworkReply::readyRead, [this, reply, keyword, cve_id](){
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        if (!jsonDoc.isNull() && jsonDoc.isObject()) {
            QJsonObject jsonData = jsonDoc.object();
            updateDetails(keyword, cve_id, jsonData);
        }
    });
}

void CVEListModel::updateDetails(const QString& keyword, const QString& cve_id, const QJsonObject &details){
    beginInsertRows(QModelIndex(), _entries.size(), _entries.size());
    CVEEntry entry;
    entry.keywords << keyword;
    entry.id = cve_id;
    entry.details = details;
    _entries.append(entry);
    endInsertRows();
}



