#ifndef PROVA_ALIGN_ALIGNMENT_H
#define PROVA_ALIGN_ALIGNMENT_H

#include <loga/fwd.h>
#include <map>
#include <cstddef>
#include <vector>
#include <boost/graph/adjacency_list.hpp>
#include <loga/index.h>
#include <loga/segment.h>
#include <loga/path.h>
#include <loga/collection.h>

namespace prova::loga{

class alignment{
    struct edge_props {
        double weight;
        int    slide;
    };

    using memo_type                 = std::map<index, std::size_t>;
    using segment_collection_type   = std::vector<segment>;
    using const_iterator            = collection::const_iterator;
    using graph_type                = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, boost::no_property, edge_props>;
    using vertex_type               = boost::graph_traits<graph_type>::vertex_descriptor;

    const collection& _collection;
    memo_type  _memo;
public:
    using key_type = std::pair<std::uint32_t, std::uint32_t>;
    struct pair_hash{
        std::uint64_t operator()(const key_type& key) const noexcept;
    };
    using matrix_type = std::unordered_map<key_type, prova::loga::path, pair_hash>;
public:
    explicit alignment(const collection& collection);
    const collection& inputs() const;

    /**
     * @brief bubble accross the kD tensor starting from the given index through the diagonal (all 1) line
     * @param idx
     * @param threshold
     * @param carry
     * @pre expects idx.size() == inputs.size()
     */
    void bubble(const index& idx, std::size_t threshold, std::size_t carry);

    /**
     * @brief bubble from all floor points in the kD tensor
     * @param threshold
     * @return
     */
    graph bubble_all(std::size_t threshold = 1);

    /**
     * @brief bubble accross a matrix starting from the given index through the diagonal line
     * @param u
     * @param v
     * @param idx
     * @param threshold
     * @param carry
     * @pre expects the idx.size() == 2
     */
    void bubble_pairwise(const_iterator u, const_iterator v, const index& idx, memo_type& memo, std::size_t threshold, std::size_t carry) const;

    /**
     * @brief Takes the first one as the base and others as the reference
     * @param u
     * @param v
     * @param threshold
     */
    void bubble_all_pairwise(prova::loga::alignment::matrix_type& mat, std::size_t threshold = 1, std::size_t threads = 0);

    const memo_type& memo() const { return _memo; }
};

}

#endif // PROVA_ALIGN_ALIGNMENT_H
