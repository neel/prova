#ifndef PROVA_ALIGN_GRAPH_H
#define PROVA_ALIGN_GRAPH_H

#include <loga/fwd.h>
#include <ostream>
#include <map>
#include <loga/segment.h>
#include <boost/graph/adjacency_list.hpp>

namespace prova::loga{

class graph{
    struct edge_props {
        double weight;
        int    slide;
    };

    using segment_collection_type   = std::vector<segment>;
    using graph_type                = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, boost::no_property, edge_props>;
    using vertex_type               = boost::graph_traits<graph_type>::vertex_descriptor;
    using edge_type                 = boost::graph_traits<graph_type>::edge_descriptor;

    segment_collection_type _segments;
    graph_type  _graph;
    vertex_type _S, _T;
    std::map<vertex_type, std::size_t> _vertices;
    segment _start;
    segment _finish;
public:
    explicit graph(segment_collection_type&& segments, segment&& start, segment&& finish);
    void build();
    std::ostream& print(std::ostream& stream);
    path shortest_path();
public:
    using iterator = segment_collection_type::iterator;
    using const_iterator = segment_collection_type::const_iterator;
    using value_type = segment;
    using size_type  = segment_collection_type::size_type;
    using reference_type = std::add_lvalue_reference_t<segment>;
    using const_reference_type = std::add_const_t<reference_type>;

    const_iterator begin() const { return _segments.cbegin(); }
    const_iterator end() const { return _segments.cend(); }
    size_type size() const { return _segments.size(); }
};

}

#endif // PROVA_ALIGN_GRAPH_H
