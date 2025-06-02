#ifndef DIRECTORYTRIE_H
#define DIRECTORYTRIE_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>

class DirectoryTrie{
public:
    DirectoryTrie();
    ~DirectoryTrie();

    void insert(const QStringList &segments);


    void clear();

    DirectoryTrie(const DirectoryTrie &)            = delete;
    DirectoryTrie &operator=(const DirectoryTrie &) = delete;
    DirectoryTrie(DirectoryTrie &&)                 = default;
    DirectoryTrie &operator=(DirectoryTrie &&)      = default;

    QStringList suffixes(std::size_t level) const;

private:
    struct Node {
        Node*                parent = 0x0;
        QString              segment;
        std::map<QString,Node*>  children;
        bool                 isEnd = false;
        ~Node();

        QStringList tail(const Node* top) const;
        std::size_t tail(QStringList& list, const Node* top) const;
        std::size_t tails(QList<QStringList>& list, const Node* top) const;
        QStringList tails(const Node* top) const;
    };

    /**
     * @brief list all junctions (nodes that have > 1 children) at the given level
     * level 0 junction implies there are no junctions before that.
     * level 1 junction implies there are one or more level 0 junctions before that
     * @param level
     * @return
     */
    std::size_t junctions(Node* root, std::size_t level, QList<Node*>& j) const;

    Node* _root;
};
#endif // DIRECTORYTRIE_H
