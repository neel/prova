#ifndef PROVA_ALIGN_TOKENIZED_ALIGNMENT_H
#define PROVA_ALIGN_TOKENIZED_ALIGNMENT_H

#include <vector>
#include <loga/segment.h>
#include <boost/graph/adjacency_list.hpp>
#include <loga/tokenized_collection.h>
#include <cereal/types/utility.hpp>
#include <cereal/types/unordered_map.hpp>

namespace prova::loga{

class tokenized_alignment{
    struct edge_props {
        double weight;
        int    slide;
    };

    using memo_type                 = std::map<index, std::size_t>;
    using segment_collection_type   = std::vector<segment>;
    using const_iterator            = tokenized_collection::const_iterator;
    using graph_type                = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, boost::no_property, edge_props>;
    using vertex_type               = boost::graph_traits<graph_type>::vertex_descriptor;

    const tokenized_collection& _collection;
public:
    using key_type = std::pair<std::uint32_t, std::uint32_t>;
    struct pair_hash{
        std::uint64_t operator()(const key_type& key) const noexcept;
    };
    using matrix_type = std::unordered_map<key_type, prova::loga::path, pair_hash>;
public:
    explicit tokenized_alignment(const tokenized_collection& collection);
    const tokenized_collection& inputs() const;

    void bubble_pairwise(const_iterator u, const_iterator v, const index& idx, memo_type& memo, std::size_t threshold, std::size_t carry) const;
    void bubble_all_pairwise(prova::loga::tokenized_alignment::matrix_type& mat, std::size_t threshold = 1, std::size_t threads = 0);
};

}

#endif // PROVA_ALIGN_TOKENIZED_ALIGNMENT_H
