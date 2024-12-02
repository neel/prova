#ifndef EXUMODEL_H
#define EXUMODEL_H

#include <QAbstractItemModel>
#include <memory>

namespace prova{
struct execution_unit;
}

class ExUModel : public QAbstractTableModel{
    Q_OBJECT

  public:
    explicit ExUModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  public:
    void add_unit(std::shared_ptr<prova::execution_unit> unit);
    const std::shared_ptr<prova::execution_unit>& unit(std::size_t i) const;
  private:
    std::vector<std::shared_ptr<prova::execution_unit>> _units;
};

#endif // EXUMODEL_H
