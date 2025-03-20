#ifndef CVELISTMODEL_H
#define CVELISTMODEL_H

#include <QAbstractListModel>
#include <QJsonObject>
#include <QNetworkAccessManager>

struct CVEEntry {
    QString keyword;   // e.g. "openssl"
    QString id;        // e.g. "CVE-2021-1234"
    QJsonObject details; // The JSON details once loaded; empty initially
};


class CVEListModel : public QAbstractListModel{
    Q_OBJECT

public:
    enum Roles {
        KeywordRole = Qt::UserRole + 1,
        IdRole,
        DetailsRole
    };
    Q_ENUM(Roles)

    explicit CVEListModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
public:
    Q_INVOKABLE int addCveEntry(const QString &keyword, const QString &id);
    Q_INVOKABLE void updateDetails(int row, const QJsonObject &details);
    int findCveEntry(const QString &keyword, const QString &id) const;
private:
    QList<CVEEntry>        _entries;
    QNetworkAccessManager* _network;
};

#endif // CVELISTMODEL_H
