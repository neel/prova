#include "exumodel.h"
#include "prova/process.h"
#include "prova/session.h"
#include "prova/artifact.h"
#include "prova/execution_unit.h"

ExUModel::ExUModel(QObject *parent): QAbstractItemModel(parent), _header_level(0) {}

QModelIndex ExUModel::index(int row, int column, const QModelIndex& parent) const{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if (!parent.isValid()) {
        // Top-level, representing execution units
        const prova::execution_unit* exuptr = _units.at(row).get();
        auto node = std::make_shared<tree_node>(0, (void*)exuptr);
        _nodes.push_back(node);
        return createIndex(row, column, node.get());
    } else {
        auto parentNode = static_cast<tree_node*>(parent.internalPointer());
        if (parentNode->level == 0) {
            // First child level (root session of execution unit)
            const prova::execution_unit* exuptr = static_cast<const prova::execution_unit*>(parentNode->data);
            const prova::session* root_session = exuptr->root().get();
            auto node = std::make_shared<tree_node>(1, (void*)root_session);
            node->parent = parentNode;
            node->row = row;
            _nodes.push_back(node);
            return createIndex(row, column, node.get());
        } else {
            // Subsequent child levels (sessions)
            const prova::session* parentSession = static_cast<const prova::session*>(parentNode->data);
            const prova::session* childSession = parentSession->_children.at(row).get();
            auto node = std::make_shared<tree_node>(2, (void*)childSession);
            node->parent = parentNode;
            node->row = row;
            _nodes.push_back(node);
            return createIndex(row, column, node.get());
        }
    }
}

QModelIndex ExUModel::parent(const QModelIndex& index) const{
    if (!index.isValid())
        return QModelIndex();

    auto* childNode = static_cast<tree_node*>(index.internalPointer());
    if (childNode->level == 0) {
        // Top-level nodes have no parent
        return QModelIndex();
    } else if (childNode->level == 1 || childNode->level == 2) {
        return createIndex(childNode->parent->row, 0, childNode->parent);
    }


    return QModelIndex();
}


int ExUModel::rowCount(const QModelIndex &parent) const{
    if (!parent.isValid()) {
        return _units.size(); // Top-level rows: number of execution units
    } else {
        auto* node = static_cast<tree_node*>(parent.internalPointer());
        if (node->level == 0) {
            return 1;
        } else if(node->level >= 1) {
            auto sess = static_cast<const prova::session*>(node->data);
            return sess->_children.size();
        }
    }
    return 0;
}

int ExUModel::columnCount(const QModelIndex &parent) const{
    return 4;
}

QVariant ExUModel::data(const QModelIndex &index, int role = Qt::DisplayRole) const {
    if (!index.isValid())
        return QVariant();

    auto node = static_cast<tree_node*>(index.internalPointer());
    if (!node) {
        return QVariant(); // Guard against null pointer access
    }

    if (role == Qt::DisplayRole) {
        if (node->level == 0) {
            const prova::execution_unit* exuptr = static_cast<const prova::execution_unit*>(node->data);
            switch (index.column()) {
                case 0: return QVariant(index.row());
                case 1: return QVariant(exuptr->process()->pid());
                case 2: return QVariant(static_cast<unsigned int>(exuptr->artifacts_count()));
                case 3: return QString::fromStdString(exuptr->process()->exe());
                default: return QVariant();
            }
        } else {
            const prova::session* session = static_cast<const prova::session*>(node->data);
            const prova::artifact::ptr& artifact = session->_artifact;
            std::string artifact_name = deriveArtifactName(artifact);

            switch (index.column()) {
                case 0: return QVariant(session->first_id());
                case 1: return QVariant(session->last_id());
                case 2: return QVariant(static_cast<unsigned int>(session->_children.size()));
                case 3: return QString::fromStdString(artifact_name);
                default: return QVariant();
            }
        }
    }
    return QVariant();
}

QVariant ExUModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            if(_header_level == 0){
                switch (section) {
                    case 0: return QStringLiteral("ExU");
                    case 1: return QStringLiteral("PID");
                    case 2: return QStringLiteral("Artifacts");
                    case 3: return QStringLiteral("Exe");
                    default: return QVariant();
                }
            } else {
                switch (section) {
                  case 0: return QStringLiteral("Begin");
                  case 1: return QStringLiteral("End");
                  case 2: return QStringLiteral("Children");
                  case 3: return QStringLiteral("Artifact");
                  default: return QVariant();
                }
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

std::string ExUModel::deriveArtifactName(const prova::artifact::ptr& artifact) const {
    const auto& properties = artifact->properties();
    std::string artifact_name;

    // Choose artifact name based on subtype
    if (artifact->subtype() == "file") {
        artifact_name = properties["path"];
    } else if (artifact->subtype() == "network socket") {
        artifact_name = std::format("{}:{}", properties["remote address"].get<std::string>(), properties["remote port"].get<std::string>());
    } else if (artifact->subtype() == "unnamed pipe") {
        artifact_name = std::format("{} w{} -> r{}", properties["_key"].get<std::string>(), properties["write fd"].get<std::string>(), properties["read fd"].get<std::string>());
    } else if (artifact->subtype() == "directory" || artifact->subtype() == "character device") {
        artifact_name = properties["path"];
    } else {
        artifact_name = "Unknown";
    }
    return artifact_name;
}

void ExUModel::updateHeaderLevel(std::size_t level){
    _header_level = level;
}
