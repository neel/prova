#ifndef PROVA_ALIGN_BIO_ALIGNMENT_H
#define PROVA_ALIGN_BIO_ALIGNMENT_H

#include <loga/fwd.h>
#include <map>
#include <cstddef>
#include <vector>
#include <boost/graph/adjacency_list.hpp>
#include <loga/index.h>
#include <loga/segment.h>
#include <loga/path.h>
#include <loga/collection.h>
#include <cereal/types/utility.hpp>
#include <cereal/types/unordered_map.hpp>

namespace prova::loga{

class alignment{
    struct edge_props {
        double weight;
        int    slide;
    };

    using segment_collection_type   = std::vector<segment>;
    using const_iterator            = collection::const_iterator;
    using graph_type                = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, boost::no_property, edge_props>;
    using vertex_type               = boost::graph_traits<graph_type>::vertex_descriptor;

    const collection& _collection;
public:
    using key_type = std::pair<std::uint32_t, std::uint32_t>;
    struct pair_hash{
        std::uint64_t operator()(const key_type& key) const noexcept;
    };
    using matrix_type = std::unordered_map<key_type, prova::loga::path, pair_hash>;
public:
    explicit alignment(const collection& collection);
    const collection& inputs() const;

    void bubble_all_pairwise(prova::loga::alignment::matrix_type& mat, std::size_t threshold = 1, std::size_t threads = 0);
};

}

#endif // PROVA_ALIGN_BIO_ALIGNMENT_H
