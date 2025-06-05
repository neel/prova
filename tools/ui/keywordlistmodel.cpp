#include "keywordlistmodel.h"

KeywordListModel::KeywordListModel(QObject *parent): QAbstractListModel(parent) {}

QVariant KeywordListModel::headerData(int section, Qt::Orientation orientation, int role) const{
    // FIXME: Implement me!
}

int KeywordListModel::rowCount(const QModelIndex &parent) const{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return _keywords.count();
}

QVariant KeywordListModel::data(const QModelIndex &index, int role) const{
    if (!index.isValid() || index.row() < 0 || index.row() >= _keywords.count())
        return QVariant();

    auto begin = _keywords.constBegin();
    auto it = std::next(begin, index.row());
    QString key = it.key();

    if(key.isEmpty()){
        return QVariant();
    }

    switch (role) {
        case KeywordRole: return key;
        case CountRole:   return it.value();
        case EnabledRole: return !disabled(key);
        default:          return {};
    }

    return QVariant();
}

QHash<int, QByteArray> KeywordListModel::roleNames() const{
    static QHash<int, QByteArray> roles {
        { KeywordRole, "keyword" },
        { CountRole,   "count" },
        { EnabledRole, "enabled" }
    };
    return roles;
}

void KeywordListModel::disable(const QString &key, bool flag){
    qDebug() << "Disabling " << flag << key;
    if(flag && !disabled(key)){
        _disabled.insert(key);
    } else if (!flag && disabled(key)){ // enable
        _disabled.remove(key);
    }

    int row = std::distance(_keywords.begin(), _keywords.find(key));
    if (row >= 0){
        emit dataChanged(index(row,0), index(row,0), {EnabledRole});
    }
}

bool KeywordListModel::disabled(const QString &key) const{
    return _disabled.contains(key);
}

quint32 KeywordListModel::keyword(const QString &k) const{
    if(_keywords.count(k)){
        return _keywords[k];
    }
    return 0;
}

quint32 KeywordListModel::add(const QString &key){
    auto begin = _keywords.begin();
    auto it    = _keywords.find(key);

    if(it != _keywords.end()){
        auto distance = std::distance(begin, it);
        quint32 count = ++_keywords[key];
        emit dataChanged(createIndex(distance, 1), createIndex(distance, 1));
        return count;
    } else {
        beginInsertRows(QModelIndex(), _keywords.size(), _keywords.size());
        _keywords.insert(key, 1);
        endInsertRows();
        return 1;
    }
}


// KeywordListProxyModel::KeywordListProxyModel(QObject *parent): QSortFilterProxyModel(parent) {

// }

// bool KeywordListProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
//     QModelIndex idIndex = sourceModel()->index(sourceRow, 0, sourceParent);
//     QModelIndex keywordIndex = sourceModel()->index(sourceRow, 0, sourceParent);

//     bool enabled = sourceModel()->data(idIndex, KeywordListModel::EnabledRole).toBool();
//     return enabled;
// }
