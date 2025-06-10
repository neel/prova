#include "exufilterproxymodel.h"

ExUFilterProxyModel::ExUFilterProxyModel(QObject *parent): QSortFilterProxyModel(parent){

}

void ExUFilterProxyModel::setExeFilter(const QString &exe) {
    if (_exe == exe)
        return;

    _exe = exe;
    invalidateFilter();
}

bool ExUFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    if (_exe.isEmpty())
        return true;

    const QAbstractItemModel *m = sourceModel();
    if (!m)
        return true;

    if (!sourceParent.isValid()) {
        QModelIndex exeIndex = m->index(sourceRow, 3, sourceParent);
        const QString exe = exeIndex.data(Qt::DisplayRole).toString().toLower();
        return exe.contains(_exe.toLower());
    }

    QModelIndex rootParent = sourceParent;
    while (rootParent.parent().isValid()) {
        rootParent = rootParent.parent();
    }

    QModelIndex rootExeIndex = m->index(rootParent.row(), 3, QModelIndex());
    const QString rootExe = rootExeIndex.data(Qt::DisplayRole).toString().toLower();
    return rootExe.contains(_exe.toLower());
}
