#ifndef CVELISTMODEL_H
#define CVELISTMODEL_H

#include <QAbstractListModel>
#include <QJsonObject>
#include <QNetworkAccessManager>

struct CVEEntry {
    QStringList keywords;   // e.g. "openssl"
    QString id;        // e.g. "CVE-2021-1234"
    QJsonObject details; // The JSON details once loaded; empty initially
};

bool operator==(const CVEEntry& entry, const QString& id);


class CVEListModel : public QAbstractListModel{
    Q_OBJECT

public:
    enum Roles {
        KeywordRole = Qt::UserRole + 1,
        IdRole,
        DetailsRole
    };
    Q_ENUM(Roles)

    explicit CVEListModel(QNetworkAccessManager* network, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
public:
    void search(const QString& keyword);
private:
    void replyReceived(const QString &keyword, QNetworkReply* reply);
    void fetchCVEDetails(const QString& keyword, const QString& cve_id);
    void updateDetails(const QString& keyword, const QString& cve_id, const QJsonObject &details);
private:
    QList<CVEEntry>        _entries;
    QSet<QString>          _cve_ids;
    QNetworkAccessManager* _network;
signals:
    void searchFinished(const QString& keyword);
};

#endif // CVELISTMODEL_H
