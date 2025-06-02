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

    /* Insert one directory expressed as a list of segments */
    void insert(const QStringList &segments);

    /* Full, untrimmed paths that were inserted */
    QList<QStringList> prefixes() const;

    /* Trimmed version:
         1. chop the *longest* common leading prefix
         2. iterate left-to-right, shortening earlier
            duplicates until the first element of
            every list is unique.                   */
    QList<QStringList> uniquePrefixes() const;

    void clear();

    DirectoryTrie(const DirectoryTrie &)            = delete;
    DirectoryTrie &operator=(const DirectoryTrie &) = delete;
    DirectoryTrie(DirectoryTrie &&)                 = default;
    DirectoryTrie &operator=(DirectoryTrie &&)      = default;

private:
    struct Node
    {
        QString              segment;
        QMap<QString,Node*>  children;
        bool                 isEnd = false;
        ~Node();
    };

    void collect(const Node *node,
                 QStringList &path,
                 QList<QStringList> &out) const;

    Node *root;
};
#endif // DIRECTORYTRIE_H
