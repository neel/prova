#ifndef KEYWORDLISTMODEL_H
#define KEYWORDLISTMODEL_H

#include <QSet>
#include <QMap>
#include <QString>
#include <QAbstractListModel>

#include <QSortFilterProxyModel>

class KeywordListModel : public QAbstractListModel{
    Q_OBJECT

public:
    enum Roles {
        KeywordRole = Qt::UserRole + 1,
        CountRole,
        EnabledRole
    };
    Q_ENUM(Roles)

    explicit KeywordListModel(QObject *parent = nullptr);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
public:
    bool disabled(const QString& key) const;
    quint32 keyword(const QString& k) const;
    quint32 add(const QString& key);
public slots:
    void disable(const QString& key, bool flag);
private:
    QMap<QString, quint32> _keywords;
    QSet<QString>          _disabled;
};

// class KeywordListProxyModel : public QSortFilterProxyModel{
//     Q_OBJECT
// public:
//     explicit KeywordListProxyModel(QObject* parent = nullptr);
//     bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
// };

#endif // KEYWORDLISTMODEL_H
