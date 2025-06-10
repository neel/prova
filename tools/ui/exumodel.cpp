#include "exumodel.h"
#include "prova/process.h"
#include "prova/session.h"
#include "prova/artifact.h"
#include "prova/execution_unit.h"
#include <QBrush>

ExUModel::ExUModel(QObject *parent): QAbstractItemModel(parent), _header_level(0) {}

QModelIndex ExUModel::index(int row, int column, const QModelIndex& parent) const{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if (!parent.isValid()) {
        // Top-level, representing execution units
        if (row >= static_cast<int>(_units.size()))
            return QModelIndex();
        const prova::execution_unit* exuptr = _units.at(row).get();
        return createIndex(row, column, (void*)exuptr);
    } else {
        void* parentPtr = parent.internalPointer();

        // Determine what type of parent we have
        // We need to distinguish between execution_unit and session pointers
        // This is tricky - we'll use the level information stored in the parent

        // Check if parent is an execution unit (top level)
        bool parentIsExU = false;
        for (const auto& unit : _units) {
            if (unit.get() == parentPtr) {
                parentIsExU = true;
                break;
            }
        }

        if (parentIsExU) {
            // Parent is execution unit, child should be root session
            const prova::execution_unit* exuptr = static_cast<const prova::execution_unit*>(parentPtr);
            if (row == 0) { // Only one root session per execution unit
                const prova::session* root_session = exuptr->root().get();
                return createIndex(row, column, (void*)root_session);
            }
        } else {
            // Parent is a session, children are child sessions
            const prova::session* parentSession = static_cast<const prova::session*>(parentPtr);
            if (row < static_cast<int>(parentSession->_children.size())) {
                const prova::session* childSession = parentSession->_children.at(row).get();
                return createIndex(row, column, (void*)childSession);
            }
        }
    }

    return QModelIndex();
}

QModelIndex ExUModel::parent(const QModelIndex& index) const{
    if (!index.isValid())
        return QModelIndex();

    void* childPtr = index.internalPointer();

    // Check if this is an execution unit (top level) - has no parent
    for (size_t i = 0; i < _units.size(); ++i) {
        if (_units[i].get() == childPtr) {
            return QModelIndex(); // Top level item
        }
    }

    // This must be a session - find its parent
    const prova::session* childSession = static_cast<const prova::session*>(childPtr);

    // Check if this is a root session (parent is execution unit)
    for (size_t i = 0; i < _units.size(); ++i) {
        if (_units[i]->root().get() == childSession) {
            // Parent is execution unit at index i
            return createIndex(static_cast<int>(i), 0, (void*)_units[i].get());
        }
    }

    // This is a child session - need to find parent session
    // This is complex because we need to traverse the tree
    // For now, let's implement a helper to find parent session
    for (size_t exuIdx = 0; exuIdx < _units.size(); ++exuIdx) {
        const prova::session* parentSession = findParentSession(_units[exuIdx]->root().get(), childSession);
        if (parentSession) {
            return createIndex(getSessionRowInParent(parentSession, childSession), 0, (void*)parentSession);
        }
    }

    return QModelIndex();
}

int ExUModel::rowCount(const QModelIndex &parent) const{
    if (!parent.isValid()) {
        return static_cast<int>(_units.size()); // Top-level rows: number of execution units
    } else {
        void* parentPtr = parent.internalPointer();

        // Check if parent is execution unit
        for (const auto& unit : _units) {
            if (unit.get() == parentPtr) {
                return 1; // Each execution unit has exactly one root session
            }
        }

        // Parent must be a session
        const prova::session* session = static_cast<const prova::session*>(parentPtr);
        return static_cast<int>(session->_children.size());
    }
}

int ExUModel::columnCount(const QModelIndex &parent) const{
    Q_UNUSED(parent)
    return 4;
}

bool ExUModel::hasChildren(const QModelIndex &parent) const {
    if (!parent.isValid()) {
        // Root level - has children if there are execution units
        return !_units.empty();
    }

    void* parentPtr = parent.internalPointer();

    // Check if parent is execution unit
    for (const auto& unit : _units) {
        if (unit.get() == parentPtr) {
            return true; // Execution units always have at least the root session
        }
    }

    // Parent must be a session
    const prova::session* session = static_cast<const prova::session*>(parentPtr);
    return !session->_children.empty();
}

QVariant ExUModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    void* nodePtr = index.internalPointer();
    if (!nodePtr) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        // Check if this is an execution unit
        for (size_t i = 0; i < _units.size(); ++i) {
            if (_units[i].get() == nodePtr) {
                // This is an execution unit
                const prova::execution_unit* exuptr = static_cast<const prova::execution_unit*>(nodePtr);
                switch (index.column()) {
                case 0: return QVariant(static_cast<int>(i));
                case 1: return QVariant(exuptr->process()->pid());
                case 2: return QVariant(static_cast<unsigned int>(exuptr->artifacts_count()));
                case 3: return QString::fromStdString(exuptr->process()->exe());
                default: return QVariant();
                }
            }
        }

        // This must be a session
        const prova::session* session = static_cast<const prova::session*>(nodePtr);
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

    std::function<bool (const prova::session&)> lambda_unclosed_session;
    lambda_unclosed_session = [&lambda_unclosed_session](const prova::session& session){
        if(session.first_id() == session.last_id()) {
            return true;
        }
        for(auto it = session.children_begin(); it != session.children_end(); ++it) {
            const std::shared_ptr<prova::session>& child = *it;
            if(lambda_unclosed_session(*child)){
                return true;
            }
        }
        return false;
    };

    if (role == Qt::ForegroundRole) {
        // Check if this is an execution unit
        for (const auto& unit : _units) {
            if (unit.get() == nodePtr) {
                const prova::execution_unit* exuptr = static_cast<const prova::execution_unit*>(nodePtr);
                std::shared_ptr<prova::session> session_ptr = exuptr->root();
                if(lambda_unclosed_session(*session_ptr)) {
                    return QBrush(Qt::red);
                }
                return QVariant();
            }
        }

        // This must be a session
        const prova::session* session = static_cast<const prova::session*>(nodePtr);
        if(lambda_unclosed_session(*session)) {
            return QBrush(Qt::red);
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
        else if (orientation == Qt::Vertical) {
            return QString::number(section);
        }
    }
    return QVariant();
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

// Helper methods
const prova::session* ExUModel::findParentSession(const prova::session* root, const prova::session* target) const {
    for (const auto& child : root->_children) {
        if (child.get() == target) {
            return root;
        }
        const prova::session* found = findParentSession(child.get(), target);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

int ExUModel::getSessionRowInParent(const prova::session* parent, const prova::session* child) const {
    for (size_t i = 0; i < parent->_children.size(); ++i) {
        if (parent->_children[i].get() == child) {
            return static_cast<int>(i);
        }
    }
    return 0;
}
