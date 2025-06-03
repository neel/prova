#include "directorytrie.h"
#include <QtGlobal>      // qDeleteAll
#include <QVector>
#include <QHash>

// ---------- Node --------------------------------------------------------------
DirectoryTrie::Node::~Node(){
    for(auto& pair: children) {
        delete pair.second;
        pair.second = 0x0;
    }
}

bool DirectoryTrie::Node::reachable_from(const Node* junction) const{
    if (this == junction) {
        return true;
    }
    if(junction == 0x0) {
        return false;
    }
    const Node* p = parent;
    while(p && p != junction){
        p = p->parent;
    }
    return (p == junction);
}

std::size_t DirectoryTrie::Node::tails(QList<QStringList>& list, const Node* junction) const {
    if(junction == 0x0) junction = this;
    std::size_t len = 0;
    if(children.size() == 0) {
        list << tail(junction);
        len += 1;
    } else {
        for(const auto& pair: children) {
            len += pair.second->tails(list, junction);
        }
    }
    return len;
}

std::size_t DirectoryTrie::Node::tails(QMap<QString, QStringList>& mapping, const Node* junction) const {
    if(junction == 0x0) junction = this;
    std::size_t len = 0;
    if(isEnd) {
        mapping.insert(fullPath, tail(junction));
        len += 1;
    }

    for(const auto& pair: children) {
        len += pair.second->tails(mapping, junction);
    }
    return len;
}

QStringList DirectoryTrie::Node::tails() const {
    QList<QStringList> list;
    if(isEnd) {
        list << tail(parent);
        tails(list, parent);
    } else {
        tails(list, this);
    }
    QStringList results;
    for(const QStringList& p: std::as_const(list)) {
        results << p.join('/');
    }
    return results;
}

QMap<QString, QString> DirectoryTrie::Node::tailsMap() const {
    QMap<QString, QStringList> mapping;
    if(isEnd) {
        mapping.insert(fullPath, tail(parent));
        tails(mapping, parent);
    } else {
        tails(mapping, this);
    }
    QMap<QString, QString> results;
    for(auto i = mapping.begin(), end = mapping.end(); i != end; ++i) {
        results.insert(i.key(), "/"+i.value().join('/'));
    }
    return results;
}

std::size_t DirectoryTrie::Node::tail(QStringList& list, const Node* junction) const {
    std::size_t len = 0;
    if(this != junction && parent != 0x0) {
        len += parent->tail(list, junction);
    }
    list << segment;
    len += 1;
    return len;
}



QStringList DirectoryTrie::Node::tail(const Node* junction) const {
    QStringList list;
    tail(list, junction);
    return list;
}

// ---------- DirectoryTrie -----------------------------------------------------
DirectoryTrie::DirectoryTrie() : _root(new Node) {}
DirectoryTrie::~DirectoryTrie() { delete _root; }

void DirectoryTrie::clear() {
    delete _root;
    _root = new Node;
}

void DirectoryTrie::insert(const QStringList& segments){
    Node *cur = _root;
    for (const QString &seg : segments) {
        Node* child = 0x0;
        if(cur->children.contains(seg)){
            child = cur->children[seg];
        }
        if (!child) {
            child          = new Node;
            child->segment = seg;
            child->parent  = cur;
            cur->children.insert(std::make_pair(seg, child));
        }
        cur = child;
    }
    cur->isEnd = true;
    cur->fullPath = "/"+segments.join("/");
}

QStringList DirectoryTrie::suffixes(std::size_t level) const {
    QList<Node*> j = junctions(level);
    QStringList list;
    for(const Node* n: std::as_const(j)) {
        list << n->tails();
    }
    return list;
}

QMap<QString, QString> DirectoryTrie::suffixesMap(std::size_t level) const {
    QList<Node*> j = junctions(level);
    QMap<QString, QString> mapping;
    for(const Node* n: std::as_const(j)) {
        mapping.insert(n->tailsMap());
    }
    return mapping;
}

std::size_t DirectoryTrie::junctions(Node* root, std::size_t level, QList<Node*>& j) const {
    std::size_t count= 0;
    for(const auto& pair: root->children) {
        if(pair.second->children.size() > 1) {
            if(level == 0) {
                j << pair.second;
                ++count;
            } else {
                count += junctions(pair.second, level -1, j);
            }
        } else {
            count += junctions(pair.second, level, j);
        }
    }

    return count;
}

QList<DirectoryTrie::Node *> DirectoryTrie::junctions(std::size_t level) const{
    QList<DirectoryTrie::Node *> list;
    junctions(_root, level, list);

    // All junctions should be reachable from the root unless there exists a single path
    // starting from the root that does not share a common node with any other paths
    // if there are more than such paths that j will already contain it.
    // therefore if there is a junction in j which is not reachable from root then
    // create a fake node equivalent of root that has a single children which is the disconnected path

    for(const auto& pair: _root->children) {
        int reachable = 0;
        for(const Node* node: list) {
            if(node->reachable_from(pair.second)){
                reachable++;
            }
        }

        if(reachable == 0) {
            list << pair.second;
        }
    }

    return list;
}
