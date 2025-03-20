#include "cvelistmodel.h"
#include <QNetworkReply>

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
        return entry.keyword;
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
        { KeywordRole, "keyword" },
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

    QNetworkReply* reply = _network->get(request);
    connect(reply, &QNetworkReply::readyRead, [this, reply, keyword](){
        replyReceived(keyword, reply);
    });
}

void CVEListModel::replyReceived(const QString &keyword, QNetworkReply *reply){
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

    if(results.empty()){
        // replyEmpty(keyword);
    }

    for(const QString& result: results){
        // if(_cves.contains(result))
        //     continue;
        // _cves.insert(result);
        QNetworkRequest request;
        request.setUrl(QUrl(QString("https://cveawg.mitre.org/api/cve/%1").arg(result)));
        request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20100101 Firefox/10.0");
        QNetworkReply* reply = _network->get(request);
        connect(reply, &QNetworkReply::readyRead, [this, reply, keyword](){
            // cveReplyReceived(keyword, reply);
        });

        addCveEntry(keyword, result);
    }

    reply->deleteLater();
}

int CVEListModel::addCveEntry(const QString &keyword, const QString &id){
    // Check if we already have it
    int row = findCveEntry(keyword, id);
    if (row != -1) {
        return row; // Already in the list, just return that row
    }

    // Insert a new entry
    beginInsertRows(QModelIndex(), _entries.size(), _entries.size());
    CVEEntry entry;
    entry.keyword = keyword;
    entry.id = id;
    // entry.details is empty initially
    _entries.append(entry);
    endInsertRows();

    return _entries.size() - 1; // row of the newly added entry
}

void CVEListModel::updateDetails(int row, const QJsonObject &details){
    if (row < 0 || row >= _entries.size())
        return;
    _entries[row].details = details;
    // Emit dataChanged so that QML sees the updated role
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {DetailsRole});
}

int CVEListModel::findCveEntry(const QString &keyword, const QString &id) const{
    for (int i = 0; i < _entries.size(); i++) {
        const CVEEntry &entry = _entries.at(i);
        if (entry.keyword == keyword && entry.id == id) {
            return i;
        }
    }
    return -1;
}
