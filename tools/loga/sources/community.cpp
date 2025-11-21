#include <loga/community.h>

void prova::loga::igraph_louvain(igraph_t* G, const igraph_vector_t* W, arma::Row<std::size_t> &L, double resolution){
    assert(igraph_is_directed(G) == 0);

    const std::size_t N = static_cast<std::size_t>(igraph_vcount(G));
    const std::size_t E = static_cast<std::size_t>(igraph_ecount(G));

    double m = 0.0;

    for (igraph_integer_t e = 0; e < (igraph_integer_t)E; ++e) {
        igraph_integer_t u, v;
        igraph_edge(G, e, &u, &v);
        double w = VECTOR(*W)[e];

        m += w;
    }

    if (m <= 0.0) throw std::runtime_error("leiden_membership: total edge weight <= 0");

    igraph_vector_int_t membership;
    igraph_vector_int_init(&membership, N);

    igraph_matrix_int_t memberships;
    igraph_matrix_int_init(&memberships, 0, 0);

    igraph_vector_t modularity;
    igraph_vector_init(&modularity, 0);

    igraph_community_multilevel(G, W, resolution, &membership, &memberships, &modularity);

    igraph_matrix_int_destroy(&memberships);
    igraph_vector_destroy(&modularity);

    L.set_size(N);
    for (std::size_t i = 0; i < N; ++i)
        L[i] = static_cast<std::size_t>(VECTOR(membership)[static_cast<long>(i)]);

    igraph_vector_int_destroy(&membership);
}

void prova::loga::igraph_leiden(igraph_t* G, const igraph_vector_t* W, arma::Row<std::size_t> &L, double resolution) {
    const std::size_t N = static_cast<std::size_t>(igraph_vcount(G));
    const std::size_t E = static_cast<std::size_t>(igraph_ecount(G));

    double m = 0.0;

    for (igraph_integer_t e = 0; e < (igraph_integer_t)E; ++e) {
        igraph_integer_t u, v;
        igraph_edge(G, e, &u, &v);
        double w = VECTOR(*W)[e];

        m += w;
    }

    if (m <= 0.0) throw std::runtime_error("leiden_membership: total edge weight <= 0");

    igraph_vector_int_t membership;
    igraph_vector_int_init(&membership, N);

    igraph_integer_t n_clusters = 0;

    igraph_real_t quality = 0.0;

    igraph_community_leiden_simple(G, const_cast<igraph_vector_t*>(W), IGRAPH_LEIDEN_OBJECTIVE_MODULARITY, resolution, 0, -1, 2, &membership, &n_clusters, &quality);

    L.set_size(N);
    for (std::size_t i = 0; i < N; ++i)
        L[i] = static_cast<std::size_t>(VECTOR(membership)[static_cast<long>(i)]);

    igraph_vector_int_destroy(&membership);
}
