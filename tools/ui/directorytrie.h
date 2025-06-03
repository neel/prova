#ifndef DIRECTORYTRIE_H
#define DIRECTORYTRIE_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>

/**
 * @brief Segment–wise directory trie.
 *
 * Every node represents one directory component (not a character).
 * You typically build it with @ref insert and later ask for
 * suffixes that diverge below a chosen "junction level"
 * using @ref suffixes().
 */
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

    /**
     * @brief Return **all suffix strings** that hang under every
     *        junction node found @p level levels below the root.
     *
     * A *junction* is a node with > 1 children.
     * `level==0` ► collect the first set of junctions,
     * `level==1` ► collect junctions *below* the first set
     *
     * @param level  depth of junction set (0-based)
     * @return       list of suffix strings joined with '/'
     */
    QStringList suffixes(std::size_t level) const;

    QMap<QString, QString> suffixesMap(std::size_t level) const;

private:
    struct Node {
        Node*                parent = 0x0;
        QString              segment;
        std::map<QString,Node*>  children;
        bool                 isEnd = false;
        QString              fullPath;

        ~Node();

        /**
         * @brief Is this node reachable from @p junction by climbing parents
         * @param junction  candidate ancestor
         * @return true if @p junction lies on the ancestor chain including @p this itself.
         */
        bool reachable_from(const Node* junction) const;

        /**
         * @brief list of segments starting from the junction till it reaches this node including its own segment.
         * @pre expects junction to be this node or ancestor of this node.
         * @param junction
         * @return
         */
        QStringList tail(const Node* junction) const;
        /**
         * @brief list of segments starting from the junction till it reaches this node including its own segment in the out parameter list.
         * @pre expects junction to be this node or ancestor of this node.
         * @param list
         * @param junction
         * @return number of segments traversed
         */
        std::size_t tail(QStringList& list, const Node* junction) const;
        /**
         * @brief populates the out argument list with the tails otiginating from the this node
         * @param list
         * @param junction defaults to null, if null then uses this as the junction
         * @return
         */
        std::size_t tails(QList<QStringList>& list, const Node* junction = 0x0) const;

        std::size_t tails(QMap<QString, QStringList> &mapping, const Node* junction = 0x0) const;
        /**
         * @brief returns list with the tails otiginating from the this node.
         * @details If top is also a terminal node (may also have children
         *          suggesting existance of a directory along with its subdirectories)
         *          then replaces top with top->parent
         * @param top defaults to null, if null then uses this as the top
         * @return
         */
        QStringList tails() const;

        QMap<QString, QString> tailsMap() const;
    };

    /**
     * @brief list all junctions (nodes that have > 1 children) at the given level
     * level 0 junction implies there are no junctions before that.
     * level 1 junction implies there are one or more level 0 junctions before that
     * @param level
     * @return
     */
    std::size_t junctions(Node* root, std::size_t level, QList<Node*>& j) const;

    QList<Node*> junctions(std::size_t level) const;

    Node* _root;
};
#endif // DIRECTORYTRIE_H
