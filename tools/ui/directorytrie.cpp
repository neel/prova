#include "directorytrie.h"
#include <QtGlobal>      // qDeleteAll
#include <QVector>
#include <QHash>

// ---------- Node --------------------------------------------------------------
DirectoryTrie::Node::~Node()
{
    qDeleteAll(children);
}

// ---------- DirectoryTrie -----------------------------------------------------
DirectoryTrie::DirectoryTrie() : root(new Node) {}
DirectoryTrie::~DirectoryTrie() { delete root; }

void DirectoryTrie::clear()
{
    delete root;
    root = new Node;
}

void DirectoryTrie::insert(const QStringList &segments)
{
    Node *cur = root;
    for (const QString &seg : segments)
    {
        Node *&child = cur->children[seg];
        if (!child) {
            child          = new Node;
            child->segment = seg;
        }
        cur = child;
    }
    cur->isEnd = true;
}

void DirectoryTrie::collect(const Node *node,
                            QStringList &path,
                            QList<QStringList> &out) const
{
    if (node->isEnd)
        out.append(path);

    for (auto it = node->children.constBegin();
         it != node->children.constEnd(); ++it)
    {
        path.append(it.key());
        collect(it.value(), path, out);
        path.removeLast();
    }
}

QList<QStringList> DirectoryTrie::prefixes() const
{
    QList<QStringList> out;
    QStringList path;
    collect(root, path, out);
    return out;
}

// ---------- NEW:  shortest non-colliding lists -------------------------------
static int longestCommonPrefixLen(const QList<QStringList>& v)
{
    if (v.isEmpty()) return 0;
    int len = v.first().size();
    for (const QStringList &sl : v)
        len = qMin(len, sl.size());

    int k = 0;
    for (; k < len; ++k)
    {
        const QString &probe = v.first().at(k);
        bool allMatch = true;
        for (const QStringList &sl : v)
            if (sl.at(k) != probe) { allMatch = false; break; }
        if (!allMatch) break;
    }
    return k;               // number of segments that are identical in all
}

QList<QStringList> DirectoryTrie::uniquePrefixes() const
{
    QList<QStringList> result = prefixes();          // full, untrimmed copies
    if (result.isEmpty()) return result;

    /* 1. Strip the longest common root shared by *all* paths */
    const int lcp = longestCommonPrefixLen(result);
    if (lcp > 0) {
        for (QStringList &sl : result)
            sl.erase(sl.begin(), sl.begin() + lcp);
    }

    /* 2. Sequentially ensure each first segment is unique.
          When we hit a clash (same first segment seen earlier),
          we shorten the EARLIER path until the clash disappears. */
    QHash<QString,int> firstToIndex;   // first segment → earliest index

    for (int i = 0; i < result.size(); ++i)
    {
        while (!result[i].isEmpty())
        {
            const QString first = result[i].front();

            if (!firstToIndex.contains(first)) {
                firstToIndex.insert(first, i);
                break;                          // unique – keep as-is
            }

            int prev = firstToIndex.value(first);   // clash with an earlier path
            if (result[prev].size() > 1) {
                result[prev].pop_front();           // shorten the earlier path
                firstToIndex.remove(first);         // its first seg changed
                i = qMin(i, prev) - 1;              // recheck both paths
                break;
            }
            /* If the earlier path can’t be shortened (length 1),
               try shortening the current one instead. */
            if (result[i].size() > 1) {
                result[i].pop_front();
                continue;                           // re-evaluate clash
            }
            break;      // impossible to resolve; leave duplicates as-is
        }
    }
    return result;
}

