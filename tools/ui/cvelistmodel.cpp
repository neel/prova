#include "cvelistmodel.h"

CVEListModel::CVEListModel(QObject* parent): QAbstractListModel(parent){
    _network = new QNetworkAccessManager{this};
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
