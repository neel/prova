#ifndef EXUMODEL_H
#define EXUMODEL_H

#include <QAbstractItemModel>
#include <memory>
#include <QVector>
#include <prova/artifact.h>

namespace prova{
struct execution_unit;
}

class ExUModel : public QAbstractItemModel{
    Q_OBJECT

public:
  struct tree_node{
    using ptr = std::shared_ptr<tree_node>;

    inline tree_node(std::uint8_t l, void* d): level(l), data(d), parent(0x0), row(0) {}

    std::uint8_t level;
    void* data;
    tree_node* parent;
    std::uint32_t row;
  };

  public:
    explicit ExUModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    void updateHeaderLevel(std::size_t level);
  public:
    void add_unit(std::shared_ptr<prova::execution_unit> unit);
    const std::shared_ptr<prova::execution_unit>& unit(std::size_t i) const;
    std::string deriveArtifactName(const prova::artifact::ptr& artifact) const;
  private:
    std::vector<std::shared_ptr<prova::execution_unit>> _units;
    mutable QVector<tree_node::ptr> _nodes;
    std::size_t _header_level;
};

#endif // EXUMODEL_H
