#ifndef CVEPROXYMODEL_H
#define CVEPROXYMODEL_H

#include <QSortFilterProxyModel>


class CVEProxyModel : public QSortFilterProxyModel{
    Q_OBJECT
public:
    explicit CVEProxyModel(QObject* parent = nullptr);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

#endif // CVEPROXYMODEL_H
