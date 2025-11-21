#ifndef PROVA_ALIGN_COMMUNITY_H
#define PROVA_ALIGN_COMMUNITY_H

#include <boost/graph/connected_components.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/copy.hpp>
#include <boost/algorithm/string/join.hpp>
#include <igraph/igraph.h>
#include <armadillo>

namespace prova::loga {

template <typename GraphT>
struct bgl_helper;

template <typename OutEdgeList, typename VertexList, typename VertexProperties, typename EdgeProperties, typename GraphProperties, typename EdgeList>
struct bgl_helper<boost::adjacency_list<OutEdgeList, VertexList, boost::directedS, VertexProperties, EdgeProperties, GraphProperties, EdgeList>>{
    using is_directed = std::true_type;
    using undirected_type = boost::adjacency_list<OutEdgeList, VertexList, boost::undirectedS, VertexProperties, EdgeProperties, GraphProperties, EdgeList>;
    using uvertex_descriptor = typename boost::graph_traits<undirected_type>::vertex_descriptor;
};

template <typename OutEdgeList, typename VertexList, typename VertexProperties, typename EdgeProperties, typename GraphProperties, typename EdgeList>
struct bgl_helper<boost::adjacency_list<OutEdgeList, VertexList, boost::undirectedS, VertexProperties, EdgeProperties, GraphProperties, EdgeList>>{
    using is_directed = std::false_type;
    using undirected_type = boost::adjacency_list<OutEdgeList, VertexList, boost::undirectedS, VertexProperties, EdgeProperties, GraphProperties, EdgeList>;
    using uvertex_descriptor = typename boost::graph_traits<undirected_type>::vertex_descriptor;
};

template <typename OutEdgeList, typename VertexList, typename VertexProperties, typename EdgeProperties, typename GraphProperties, typename EdgeList>
struct bgl_helper<boost::adjacency_list<OutEdgeList, VertexList, boost::bidirectionalS, VertexProperties, EdgeProperties, GraphProperties, EdgeList>>{
    using is_directed = std::true_type;
    using undirected_type = boost::adjacency_list<OutEdgeList, VertexList, boost::undirectedS, VertexProperties, EdgeProperties, GraphProperties, EdgeList>;
    using uvertex_descriptor = typename boost::graph_traits<undirected_type>::vertex_descriptor;
};

template <typename GraphT, typename MergeF>
bgl_helper<GraphT>::undirected_type make_undirected(
    const GraphT& G_in,
    std::map<
        typename bgl_helper<GraphT>::uvertex_descriptor,
        typename boost::graph_traits<GraphT>::vertex_descriptor
    >& rvmap,
    MergeF&& merge_f
    )
{
    using graph_type    = GraphT;
    using ugraph_type   = typename bgl_helper<graph_type>::undirected_type;
    using vertex_type   = typename boost::graph_traits<graph_type>::vertex_descriptor;
    using edge_type     = typename boost::graph_traits<graph_type>::edge_descriptor;
    using edge_iterator = typename boost::graph_traits<graph_type>::edge_iterator;
    using uvertex_type  = typename boost::graph_traits<ugraph_type>::vertex_descriptor;

    ugraph_type G_out;
    std::map<vertex_type, uvertex_type> vmap;
    for (auto vp = vertices(G_in); vp.first != vp.second; ++vp.first) {
        vertex_type  v = *vp.first;
        uvertex_type u = boost::add_vertex(G_out);

        G_out[u] = G_in[v];
        vmap [v] = u;
        rvmap[u] = v;
    }

    edge_iterator ei, ei_end;
    for (boost::tie(ei, ei_end) = edges(G_in); ei != ei_end; ++ei) {
        edge_type e = *ei;
        vertex_type u = source(e, G_in);
        vertex_type v = target(e, G_in);
        if (u == v) continue;

        uvertex_type un_u = vmap[u];
        uvertex_type un_v = vmap[v];

        auto [edge_uv, exists_uv] = boost::edge(u, v, G_in);
        auto [edge_vu, exists_vu] = boost::edge(v, u, G_in);

        if(exists_uv && exists_vu) {
            const auto& props_uv = G_in[edge_uv];
            const auto& props_vu = G_in[edge_vu];

            // auto weight = std::min(props_uv.weight, props_vu.weight);
            auto weight = merge_f(props_uv.weight, props_vu.weight);

            auto [edge, exists] = boost::edge(un_u, un_v, G_out);
            if(!exists){
                std::tie(edge, exists) = boost::add_edge(un_u, un_v, G_out);
                G_out[edge] = props_uv;
                G_out[edge].weight = weight;
            } else {
                if(weight < G_out[edge].weight) {
                    G_out[edge].weight = weight;
                }
            }
            assert(exists);
            assert(G_out[edge].weight <= weight);
        } else if(exists_uv || exists_vu) {
            const auto& props = exists_uv ? G_in[edge_uv] : G_in[edge_vu];
            auto [edge, exists] = boost::edge(un_u, un_v, G_out);
            if(!exists){
                std::tie(edge, exists) = boost::add_edge(un_u, un_v, G_out);
                G_out[edge] = props;
                G_out[edge].weight = props.weight;
            } else {
                if(props.weight < G_out[edge].weight) {
                    G_out[edge].weight = props.weight;
                }
            }
            assert(exists);
            assert(G_out[edge].weight <= props.weight);
        } else {
            // !exists_uv && !exists_vu
            // Nothing to do
        }
    }

    return G_out;
}

template <typename GraphT>
bgl_helper<GraphT>::undirected_type make_undirected(
    const GraphT& G_in,
    std::map<
        typename bgl_helper<GraphT>::uvertex_descriptor,
        typename boost::graph_traits<GraphT>::vertex_descriptor
        >& rvmap
    ){
    return make_undirected(G_in, rvmap, [](auto l, auto r){
        return std::min(l, r);
    });
}


template <typename GrapthT>
std::vector<typename boost::graph_traits<GrapthT>::vertex_descriptor> to_igraph(const GrapthT& G_in, igraph_t* G_out, igraph_vector_t* E_out){
    using graph_type    = GrapthT;
    using vertex_type   = typename boost::graph_traits<graph_type>::vertex_descriptor;
    using edge_type     = typename boost::graph_traits<graph_type>::edge_descriptor;
    using edge_iterator = typename boost::graph_traits<graph_type>::edge_iterator;
    using map_type      = std::unordered_map<vertex_type, std::size_t>;
    using vector_type   = std::vector<vertex_type>;

    std::size_t V_count = boost::num_vertices(G_in);

    map_type vmap;
    vector_type vertices_ordered(V_count, boost::graph_traits<graph_type>::null_vertex());

    std::size_t idx = 0;
    for (auto vp = vertices(G_in); vp.first != vp.second; ++vp.first, ++idx) {
        vmap[*vp.first] = idx;
        vertices_ordered[idx] = *vp.first;
    }

    igraph_vector_int_t edgelist;
    igraph_vector_int_init(&edgelist, 0);
    igraph_vector_init(E_out, 0);

    edge_iterator ei, ei_end;
    for (boost::tie(ei, ei_end) = ::boost::edges(G_in); ei != ei_end; ++ei) {
        edge_type e = *ei;
        vertex_type u = ::boost::source(e, G_in);
        vertex_type v = ::boost::target(e, G_in);
        if (u == v) continue;       // exclude self loops

        double w = G_in[e].weight;  // expect weight parameter

        igraph_vector_int_push_back(&edgelist, vmap.at(u));
        igraph_vector_int_push_back(&edgelist, vmap.at(v));
        igraph_vector_push_back(E_out, w);
    }

    igraph_create(G_out, &edgelist, static_cast<::igraph_integer_t>(V_count), bgl_helper<graph_type>::is_directed::value ? IGRAPH_DIRECTED : IGRAPH_UNDIRECTED);
    igraph_vector_int_destroy(&edgelist);

    return vertices_ordered;
}

enum class community_detection_algorithm{
    louvain, leiden, weakly_connected, strongly_connected
};

void igraph_louvain(igraph_t* G, const igraph_vector_t* W, arma::Row<std::size_t> &L, double resolution = 1.0);
void igraph_leiden (igraph_t* G, const igraph_vector_t* W, arma::Row<std::size_t> &L, double resolution = 1.0);

template <typename GraphT>
void detect_communities_igraph(const GraphT& G_in, community_detection_algorithm algo, std::map<typename boost::graph_traits<GraphT>::vertex_descriptor, std::size_t>& vlabels){
    using graph_type    = GraphT;
    using vertex_type   = typename boost::graph_traits<graph_type>::vertex_descriptor;
    using labels_type   = std::map<vertex_type, std::size_t>;

    igraph_t igraph;
    igraph_vector_t edges;
    std::vector<vertex_type> vmap = to_igraph(G_in, &igraph, &edges);

    std::size_t V_count = boost::num_vertices(G_in);
    arma::Row<std::size_t> labels;
    labels.resize(V_count);

    if(algo == community_detection_algorithm::louvain) {
        assert(!bgl_helper<GraphT>::is_directed::value);
        igraph_louvain(&igraph, &edges, labels);
    } else if(algo == community_detection_algorithm::leiden){
        igraph_leiden(&igraph, &edges, labels);
    }

    for(std::size_t i = 0; i != V_count; ++i) {
        std::size_t vid = i;
        std::size_t cid = labels[i];
        vertex_type v   = vmap.at(vid);
        vlabels[v]      = cid;
    }

    igraph_destroy(&igraph);
    igraph_vector_destroy(&edges);
}

template <typename GraphT>
std::size_t _detect_communities(const GraphT& G_in, community_detection_algorithm algo, std::map<typename boost::graph_traits<GraphT>::vertex_descriptor, std::size_t>& vlabels){
    using graph_type    = GraphT;
    using vertex_type   = typename boost::graph_traits<graph_type>::vertex_descriptor;
    using labels_type   = std::map<vertex_type, std::size_t>;

    detect_communities_igraph(G_in, algo, vlabels);

    std::set<std::size_t> unique_labels;
    for(const auto& [v, l]: vlabels) {
        unique_labels.insert(l);
    }

    std::size_t nunique_labels = unique_labels.size();

    std::vector<vertex_type> isolated_vertices;
    for(auto i = vlabels.begin(); i != vlabels.end(); ++i) {
        vertex_type v = i->first;
        std::size_t in  = boost::in_degree(v, G_in);
        std::size_t out = boost::out_degree(v, G_in);
        std::size_t deg = in + out;
        if(deg == 0) {
            std::size_t new_label = nunique_labels++;
            i->second = new_label;
            unique_labels.insert(new_label);
        }
    }

    return unique_labels.size();
}

template <typename GraphT>
std::size_t detect_communities(const GraphT& G_in, community_detection_algorithm algo, std::map<typename boost::graph_traits<GraphT>::vertex_descriptor, std::size_t>& vlabels){
    using graph_type    = GraphT;
    using vertex_type   = typename boost::graph_traits<graph_type>::vertex_descriptor;
    using labels_type   = std::map<vertex_type, std::size_t>;

    if(algo == community_detection_algorithm::louvain) {
        using directed_tag = typename boost::graph_traits<graph_type>::directed_category;
        constexpr bool is_directed_v = boost::is_convertible<directed_tag, boost::directed_tag>::value;
        if(!is_directed_v) {
            return _detect_communities(G_in, algo, vlabels);
        } else {
            using ugraph_type  = typename bgl_helper<GraphT>::undirected_type;
            using uvertex_type = typename boost::graph_traits<ugraph_type>::vertex_descriptor;
            using ulabels_type = std::map<uvertex_type, std::size_t>;
            using uvmap_type   = std::map<uvertex_type, vertex_type>;

            ulabels_type ulabels;
            uvmap_type   uvmap;

            ugraph_type G_local = make_undirected(G_in, uvmap);
            std::size_t count = _detect_communities(G_local, algo, ulabels);
            for(auto [u, c]: ulabels) {
                auto v = uvmap[u];
                vlabels[v] = c;
            }
            return count;
        }
    } else if(algo == community_detection_algorithm::leiden){
        return _detect_communities(G_in, algo, vlabels);
    } else if(algo == community_detection_algorithm::strongly_connected) {
        std::vector<int> comp;
        comp.resize(boost::num_vertices(G_in));
        auto compmap = boost::make_iterator_property_map(comp.begin(), get(boost::vertex_index, G_in));
        std::size_t n_components = boost::connected_components(G_in, compmap);
        for(std::size_t i = 0; i != comp.size(); ++i) {
            vertex_type v = boost::vertex(i, G_in);
            std::size_t id = G_in[v].id;
            vlabels.insert(std::make_pair(v, comp.at(i)));
        }
        return n_components;
    } else if(algo == community_detection_algorithm::weakly_connected){
        // Not implemented yet
    }

    return 0;
}

}

#endif // PROVA_ALIGN_COMMUNITY_H
