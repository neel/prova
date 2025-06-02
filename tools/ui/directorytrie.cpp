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

std::size_t DirectoryTrie::Node::tails(QList<QStringList>& list, const Node* top) const {
    std::size_t len = 0;
    if(isEnd) {
        list << tail(top);
        len += 1;
    } else {
        for(const auto& pair: children) {
            len += pair.second->tails(list, top);
        }
    }
    return len;
}

QStringList DirectoryTrie::Node::tails(const Node* top) const {
    QList<QStringList> list;
    tails(list, top);
    QStringList results;
    for(const QStringList& p: std::as_const(list)) {
        results << p.join('/');
    }
    return results;
}

std::size_t DirectoryTrie::Node::tail(QStringList& list, const Node* top) const {
    std::size_t len = 0;
    if(this != top && parent != 0x0) {
        len += parent->tail(list, top);
    }
    list << segment;
    len += 1;
    return len;
}

QStringList DirectoryTrie::Node::tail(const Node* top) const {
    QStringList list;
    tail(list, top);
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
}

QStringList DirectoryTrie::suffixes(std::size_t level) const {
    QList<Node*> j;
    junctions(_root, level, j);
    QStringList list;
    for(const Node* n: std::as_const(j)) {
        list << n->tails(n);
    }
    return list;
}

std::size_t DirectoryTrie::junctions(Node* root, std::size_t level, QList<Node*>& j) const {
    std::size_t count= 0;
    for(const auto& pair: root->children) {
        if(pair.second->children.size() > 0){
            // pair.second is a junction
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
    }
    return count;
}
