#include "exumodel.h"
#include "prova/process.h"
#include "prova/execution_unit.h"

ExUModel::ExUModel(QObject *parent): QAbstractTableModel(parent) {}

int ExUModel::rowCount(const QModelIndex &parent) const{
    if (parent.isValid()) {
        return 0;
    }
    return _units.size();
}

int ExUModel::columnCount(const QModelIndex &parent) const{
    if (parent.isValid()) {
        return 0;
    }
    return 4;
}

QVariant ExUModel::data(const QModelIndex &index, int role = Qt::DisplayRole) const {
    if (!index.isValid())
        return QVariant();

    if (index.row() >= _units.size() || index.row() < 0 || index.column() < 0 || index.column() > 3)
        return QVariant();

    if (role == Qt::DisplayRole) {
        const auto& unit = _units.at(index.row());
        switch (index.column()) {
        case 0: return QVariant(index.row());
        case 1: return QVariant(unit->process()->pid());
        case 2: return QVariant(static_cast<unsigned int>(unit->artifacts_count()));
        case 3: return QString::fromStdString(unit->process()->exe());
        default: return QVariant();
        }
    }
    return QVariant();
}

QVariant ExUModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            switch (section) {
            case 0:
                return QStringLiteral("ExU");
            case 1:
                return QStringLiteral("PID");
            case 2:
                return QStringLiteral("Artifacts");
            case 3:
                return QStringLiteral("Exe");
            default:
                return QVariant();
            }
        }
        // Optionally handle vertical headers
        else if (orientation == Qt::Vertical) {
            return QString::number(section);
        }
    }
    return QVariant(); // Return empty QVariant for other roles or orientations if not handled
}

void ExUModel::add_unit(std::shared_ptr<prova::execution_unit> unit){
    int newRowIndex = rowCount();
    beginInsertRows(QModelIndex(), newRowIndex, newRowIndex);
    _units.emplace_back(unit);
    endInsertRows();
}

const std::shared_ptr<prova::execution_unit> &ExUModel::unit(std::size_t i) const{
    return _units.at(i);
}

