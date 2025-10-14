#ifndef PROVA_ALIGN_TOKENIZED_DISTANCE_H
#define PROVA_ALIGN_TOKENIZED_DISTANCE_H

#include <loga/tokenized_alignment.h>
#include <armadillo>
#include <igraph/igraph.h>

namespace prova::loga{

class tokenized_distance{
public:
    using key_type             = prova::loga::tokenized_alignment::key_type;
    using path_matrix_type     = prova::loga::tokenized_alignment::matrix_type;
    using distance_matrix_type = arma::mat;
public:
    struct vertex_props {
        std::size_t id;
    };

    struct edge_props {
        double weight;
    };

    using directed_graph_type   = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,  vertex_props,  edge_props>;
    using undirected_graph_type = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, vertex_props, edge_props>;
private:
    const path_matrix_type& _paths;
    std::size_t             _count;
    distance_matrix_type    _distances;
    distance_matrix_type    _structural_distances;
public:
    explicit tokenized_distance(const path_matrix_type& paths, std::size_t count);
    double dist(const prova::loga::tokenized_collection& collection, std::size_t i, std::size_t j) const;
    double dist_structural(const prova::loga::tokenized_collection& collection, std::size_t i, std::size_t j) const;
    void compute(const prova::loga::tokenized_collection& collection, std::size_t threads = 0);

    directed_graph_type knn_digraph(std::size_t k) const;
    undirected_graph_type knn_graph(std::size_t k, bool soft = true) const;

    std::ostream& print_graphml(std::ostream& stream) const;
    bool computed() const;
public:
    const distance_matrix_type& matrix() const;
};

void bgl_to_igraph(const tokenized_distance::undirected_graph_type& G_in, igraph_t* G_out, igraph_vector_t* E_out);
void leiden_membership(igraph_t* G, const igraph_vector_t* W, arma::Row<std::size_t>& L, double resolution = -1.0,  double beta = 0.01);

}

#endif // PROVA_ALIGN_TOKENIZED_DISTANCE_H
