#ifndef EXUFILTERPROXYMODEL_H
#define EXUFILTERPROXYMODEL_H

#include <QObject>

#include <QSortFilterProxyModel>

class ExUFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit ExUFilterProxyModel(QObject *parent = nullptr);
    void setExeFilter(const QString &exe);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString _exe;
};

#endif // EXUFILTERPROXYMODEL_H
