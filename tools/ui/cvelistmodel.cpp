#include "cvelistmodel.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include "keywordlistmodel.h"

bool operator==(const CVEEntry &entry, const QString &id){
    return entry.id == id;
}

CVEListModel::CVEListModel(QNetworkAccessManager* network, KeywordListModel &keywordListModel, QObject* parent): QAbstractListModel(parent),
    _network(network), _keywords(keywordListModel), /*_policy(nvd_nist_json)*/ _policy(mitre_html) {}

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

void CVEListModel::search(const QString& fullpath, const QString& keyword){
    qDebug() << "Searching using keyword: " << keyword;
    QString html_search_url = QString("https://cve.mitre.org/cgi-bin/cvekey.cgi?keyword=%1").arg(keyword);
    QString json_search_url = QString("https://services.nvd.nist.gov/rest/json/cves/2.0?keywordSearch=%1&keywordExactMatch").arg(keyword);

    // JSON -> https://services.nvd.nist.gov/rest/json/cves/2.0?keywordSearch=KEYWORD&keywordExactMatch
    // HTML -> https://cve.mitre.org/cgi-bin/cvekey.cgi?keyword=KEYWORD

    QUrl search_url;
    if(_policy == mitre_html) {
        search_url = html_search_url;
    } else if(_policy == nvd_nist_json) {
        search_url = json_search_url;
    } else {
        return;
    }

    qDebug() << "Search using " << search_url;
    QNetworkRequest request;
    request.setUrl(search_url);
    // request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20100101 Firefox/10.0");
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

    if(_policy == nvd_nist_json) {
        request.setRawHeader("apiKey", "8f37611f-f350-494e-8752-6c9ba134fc50");
    }

    QNetworkReply* reply = _network->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply, fullpath](){
        if(_policy == mitre_html)
            replyReceivedHTML(fullpath, reply);
        else if(_policy == nvd_nist_json)
            replyReceivedJSON(fullpath, reply);
    });
}

KeywordListModel &CVEListModel::keywords(){
    return _keywords;
}

void CVEListModel::replyReceivedHTML(const QString& keyword, QNetworkReply *reply){
    if(reply->error() != QNetworkReply::NoError){
        const int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "Error " << reply->error() << " status " << status_code << " while searching for " << keyword;
        for (auto h : reply->rawHeaderPairs()){
            qDebug().noquote() << h.first << ':' << h.second;
        }
        if(status_code == 429) {
            emit searchSlowdown();
        } else {
            emit searchFinished(keyword);
        }
        reply->deleteLater();
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

void CVEListModel::replyReceivedJSON(const QString& keyword, QNetworkReply *reply){
    if(reply->error() != QNetworkReply::NoError){
        const int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "Error " << reply->error() << " status " << status_code << " while searching for " << keyword;
        for (auto h : reply->rawHeaderPairs()){
            qDebug().noquote() << h.first << ':' << h.second;
        }
        if(status_code == 429) {
            emit searchSlowdown();
        } else {
            emit searchFinished(keyword);
        }
        reply->deleteLater();
        return;
    }

    QStringList results;

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    if (!jsonDoc.isNull() && jsonDoc.isObject()) {
        QJsonObject jsonData = jsonDoc.object();
        if(jsonData.contains("vulnerabilities") && jsonData["vulnerabilities"].isArray()) {
            QJsonArray vulnerabilities = jsonData["vulnerabilities"].toArray();
            for(const QJsonValue& vulval: vulnerabilities) {
                QJsonObject vulobj = vulval.toObject();
                if(vulobj.contains("cve") && vulobj["cve"].isObject()){
                    QJsonObject cve = vulobj["cve"].toObject();
                    QString id = cve["id"].toString();
                    results << id;
                }
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

    _keywords.add(keyword);

    endInsertRows();
}



