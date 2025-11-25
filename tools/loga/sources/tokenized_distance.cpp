#include <loga/tokenized_distance.h>
#include <thread>
#include <boost/asio.hpp>
#include <armadillo>
#include <boost/graph/adjacency_list.hpp>
#include <boost/property_map/function_property_map.hpp>
#include <boost/property_map/dynamic_property_map.hpp>
#include <boost/graph/graphml.hpp>
#include <loga/path.h>

prova::loga::tokenized_distance::tokenized_distance(const path_matrix_type &paths, std::size_t count): _paths(paths), _count(count) {}

// double prova::loga::tokenized_distance::dist(const tokenized_collection &collection, std::size_t i, std::size_t j) const{
//     if(i == j) {
//         return 0.0;
//     } else {
//         key_type key{i, j};
//         try{
//             const prova::loga::path& path = _paths.at(key);
//             double score = path.score(collection); // / std::max(collection.at(i).raw().size(), collection.at(j).raw().size());
//             return score;

//             if((i == 45 && j == 46)|| (i == 46 && j == 45)) {
//                 std::cout << std::format("{}, {} - > {}", i, j, 1-score) << std::endl;
//             }
//             // return 1-score;
//             arma::rowvec vec(path.size());
//             std::size_t counter = 0;
//             for(const prova::loga::segment& s: path) {
//                 vec[counter++] = s.length();
//             }
//             double length = arma::norm(vec, 2);
//             arma::rowvec utopia(1);
//             utopia[0] = std::ranges::max(path.finish());
//             return arma::norm(utopia) - length;
//         } catch(const std::out_of_range& ex) {
//             std::cout << std::format("key {}x{} out of range", i, j) << std::endl;
//             throw ex;
//         }
//     }
// }

double prova::loga::tokenized_distance::dist_structural(const tokenized_collection &collection, std::size_t i, std::size_t j) const {
    if(i == j) {
        return 0.0;
    } else {
        const auto& l_structure = collection.at(i).structure();
        const auto& r_structure = collection.at(j).structure();
        double distance = prova::loga::levenshtein_distance(l_structure.cbegin(), l_structure.cend(), r_structure.cbegin(), r_structure.cend());
        // double max = std::max(l_structure.size(), r_structure.size());
        // double d = static_cast<double>(distance)/static_cast<double>(max);
        // return d;
        return distance;
    }
}

// double prova::loga::tokenized_distance::dist_lcs(const tokenized_collection &collection, std::size_t i, std::size_t j) const {
//     if(i == j) {
//         return 0.0;
//     } else {
//         const std::string& l_str = collection.at(i).raw();
//         const std::string& r_str = collection.at(j).raw();
//         double distance = prova::loga::lcs(l_str.cbegin(), l_str.cend(), r_str.cbegin(), r_str.cend());
//         // double max = std::max(l_structure.size(), r_structure.size());
//         // double d = static_cast<double>(distance)/static_cast<double>(max);
//         // return d;
//         return distance;
//     }
// }

void prova::loga::tokenized_distance::compute(const prova::loga::tokenized_collection &collection, std::size_t threads){
    _distances.set_size(_count, _count);
    _distances.zeros();
    _structural_distances.set_size(_count, _count);
    _structural_distances.zeros();

    unsigned int T = !threads ? std::thread::hardware_concurrency() : threads;

    std::condition_variable observer;
    std::mutex mutex;
    std::atomic_uint32_t jobs_completed = 0;

    std::cout << "Computing distance matrix " << std::format("{}x{}", _count, _count)  << std::endl;

    boost::asio::thread_pool pool(T);
    for (std::size_t i = 0; i < _count; ++i) {
        boost::asio::post(pool, [this, i,  &jobs_completed, &observer, &collection]() {
            for (std::size_t j = 0; j < _count; ++j) {
                // double d_alignment          = dist(collection, i, j);
                double d_structural         = dist_structural(collection, i, j);
                // _distances(i, j)            = d_alignment;
                _structural_distances(i, j) = d_structural;
            }
            jobs_completed.fetch_add(1);
            observer.notify_one();
        });
    }

    std::uint32_t printed = 0;
    while (printed < _count) {
        std::unique_lock<std::mutex> lock(mutex);
        observer.wait(lock, [&]{
            return jobs_completed.load() > printed;
        });

        const auto upto = jobs_completed.load();
        while (printed < upto) {
            ++printed;
            // std::cout << std::format("\rDistance Matrix rows {}/{}", printed, _count) << std::flush;

            double percent = ((double)printed / (double)_count) * 10.0f;
            std::string progress(20, '=');
            std::fill(progress.begin()+(((int)percent)*2), progress.end(), '-');
            std::cout << std::format("\r{:.2f}% {}", percent*10, progress) << std::flush;
        }
    }

    pool.join();

    // for (std::size_t i = 0; i < _count; ++i) {
    //     auto row = _distances.row(i);
    //     std::set<double> distances_in_row;
    //     for(double d: row){
    //         distances_in_row.insert(d);
    //     }
    //     double min = 0;
    //     for(double d: distances_in_row) {
    //         if(d > 0) {
    //             min = d;
    //             break;
    //         }
    //     }
    //     for(std::size_t j = 0; j < _count; ++j) {
    //         double v = row[j];
    //         if(v != 0){
    //             row[j] = v - min;
    //         }
    //     }
    // }

    std::cout << std::endl << std::endl << std::flush;
    std::cout.flush();
}

bool prova::loga::tokenized_distance::load(std::istream& stream) {
    return _structural_distances.load(stream);
}

bool prova::loga::tokenized_distance::save(std::ostream& stream) const  {
    return _structural_distances.save(stream);
}

prova::loga::tokenized_distance::directed_graph_type prova::loga::tokenized_distance::knn_digraph(std::size_t k) const {
    // auto similarities = _distances;
    // similarities.each_row([](auto row){
    //     row = row.max() - row;
    // });

    auto structural_similarities = _structural_distances;
    structural_similarities.each_row([](auto row){
        row = row.max() - row;
    });

    directed_graph_type graph;

    using vertex_type = boost::graph_traits<directed_graph_type>::vertex_descriptor;
    using edge_type   = boost::graph_traits<directed_graph_type>::edge_descriptor;

    std::vector<vertex_type> V(_count);
    for(std::size_t i = 0; i != _count; ++i){
        vertex_type v = boost::add_vertex(graph);
        graph[v].id = i;
        V[i] = v;
    }

    for(std::size_t i = 0; i != _count; ++i){
        std::vector<std::pair<std::size_t, double>> weights;
        for(std::size_t j = 0; j != _count; ++j){
            if(i == j) continue;

            auto w = /*similarities(i, j) +*/ structural_similarities(i, j);
            weights.push_back(std::make_pair(j, w));
        }
        std::sort(weights.begin(), weights.end(), [](const std::pair<std::size_t, double>& l, const std::pair<std::size_t, double>& r){
            return l.second > r.second;
        });
        const auto neighbourhood = std::span(weights.cbegin(), std::min(k, _count - 1));
        for(const auto& [j, w]: neighbourhood) {
            auto pair = boost::add_edge(V[i], V[j], graph);
            if(pair.second) {
                edge_type e = pair.first;
                graph[e].weight = w;
            }
        }
    }

    return graph;
}

prova::loga::tokenized_distance::undirected_graph_type prova::loga::tokenized_distance::knn_graph(std::size_t k, bool soft) const {
    // { transform distances into similarities
    auto similarities = _distances;
    similarities.each_row([](arma::rowvec& row){
        row = row.max() - row;
    });

    auto structural_similarities = _structural_distances;
    structural_similarities.each_row([](arma::rowvec& row){
        row = row.max() - row;
    });
    // }

    undirected_graph_type graph;

    using vertex_type = boost::graph_traits<undirected_graph_type>::vertex_descriptor;
    using edge_type   = boost::graph_traits<undirected_graph_type>::edge_descriptor;

    std::vector<vertex_type> V(_count);
    for(std::size_t i = 0; i != _count; ++i){
        vertex_type v = boost::add_vertex(graph);
        graph[v].id = i;
        V[i] = v;
    }

    for(std::size_t i = 0; i != _count; ++i){
        std::vector<std::pair<std::size_t, double>> weights;
        for(std::size_t j = 0; j != _count; ++j){
            if(i == j) continue;

            auto w = /*similarities(i, j) +*/ structural_similarities(i, j);
            weights.push_back(std::make_pair(j, w));
        }
        std::sort(weights.begin(), weights.end(), [](const std::pair<std::size_t, double>& l, const std::pair<std::size_t, double>& r){
            return l.second > r.second;
        });
        // if !soft then neighbourhood is limited to size k which implies that degree of a vertex may
        //     eventually be much less than k due to presence of a reverse edge.
        // if soft then neighbourhood is unrestricted at first to enforce consistent neighbourhood size
        //     the k is enforced as a break condition
        const auto neighbourhood = std::span(weights.cbegin(), soft ? _count-1 : std::min(k, _count - 1));
        std::size_t counter = 0;
        for(const auto& [j, w]: neighbourhood) {
            vertex_type u = V[i];
            vertex_type v = V[j];

            auto [e, inserted] = boost::add_edge(u, v, graph);
            if (inserted) {
                graph[e].weight = w;
                ++counter;
            } else {
                graph[e].weight = std::min(graph[e].weight, w);
            }

            if(soft && counter >= k) {
                break;
            }
        }
    }

    return graph;
}

// std::ostream& prova::loga::tokenized_distance::print_graphml(std::ostream& stream) const {
//     directed_graph_type graph = knn_digraph(5);

//     using vertex_type = boost::graph_traits<directed_graph_type>::vertex_descriptor;
//     using edge_type   = boost::graph_traits<directed_graph_type>::edge_descriptor;

//     auto vertex_label_map = boost::make_function_property_map<vertex_type>(
//         [&](const vertex_type& v) -> std::string {
//             return boost::lexical_cast<std::string>(graph[v].id);
//         }
//     );

//     auto edge_weight_map = boost::make_function_property_map<edge_type>(
//         [&](const edge_type& e) -> double {
//             double w = graph[e].weight;
//             if(w == 0) {
//                 return std::numeric_limits<double>::epsilon();
//             }
//             return w;
//         }
//     );

//     boost::dynamic_properties properties;
//     properties.property("label", vertex_label_map);
//     properties.property("weight", edge_weight_map);

//     boost::write_graphml(stream, graph, properties, true);
//     return stream;
// }

bool prova::loga::tokenized_distance::computed() const {
    return _paths.size() > 0;
}

const prova::loga::tokenized_distance::distance_matrix_type &prova::loga::tokenized_distance::matrix() const { return _structural_distances; }


void prova::loga::bgl_to_igraph(const tokenized_distance::undirected_graph_type &G_in, igraph_t* G_out, igraph_vector_t* E_out){
    using graph_type   = tokenized_distance::undirected_graph_type;
    using vertex_type  = boost::graph_traits<graph_type>::vertex_descriptor;
    using edge_type    = boost::graph_traits<graph_type>::edge_descriptor;

    std::size_t V_count = boost::num_vertices(G_in);

    igraph_vector_int_t edgelist;
    igraph_vector_int_init(&edgelist, 0);
    igraph_vector_init(E_out, 0);

    boost::graph_traits<graph_type>::edge_iterator ei, ei_end;
    for (boost::tie(ei, ei_end) = ::boost::edges(G_in); ei != ei_end; ++ei) {
        edge_type e = *ei;
        vertex_type u = ::boost::source(e, G_in);
        vertex_type v = ::boost::target(e, G_in);
        if (u == v) continue;

        double w = G_in[e].weight;

        igraph_vector_int_push_back(&edgelist, static_cast<igraph_integer_t>(u));
        igraph_vector_int_push_back(&edgelist, static_cast<igraph_integer_t>(v));
        igraph_vector_push_back(E_out, w);
    }

    igraph_create(G_out, &edgelist, static_cast<::igraph_integer_t>(V_count), IGRAPH_UNDIRECTED);
    igraph_vector_int_destroy(&edgelist);
}

void prova::loga::leiden_membership(igraph_t* G, const igraph_vector_t* W, arma::Row<std::size_t> &L, double resolution, double beta) {
    const std::size_t n = static_cast<std::size_t>(igraph_vcount(G));
    const std::size_t E = static_cast<std::size_t>(igraph_ecount(G));

    arma::vec strength(n, arma::fill::zeros);

    double m = 0.0;
    for (igraph_integer_t e = 0; e < static_cast<igraph_integer_t>(E); ++e) {
        igraph_integer_t u, v;
        igraph_edge(G, e, &u, &v);
        double w = VECTOR(*W)[e];
        strength[static_cast<std::size_t>(u)] += w;
        strength[static_cast<std::size_t>(v)] += w;
        m += w;
    }

    if (m <= 0.0) throw std::runtime_error("leiden_membership: total edge weight <= 0");

    if (resolution < 0.0) resolution = 1.0 / (2.0 * m);

    igraph_vector_t v_out;
    igraph_vector_init(&v_out, static_cast<long>(n));
    igraph_strength(G, &v_out, igraph_vss_all(), IGRAPH_ALL, IGRAPH_NO_LOOPS, W);

    igraph_vector_int_t membership;
    igraph_vector_int_init(&membership, n);
    igraph_integer_t n_clusters = 0;
    igraph_real_t quality = 0.0;

    // igraph_community_leiden(G, const_cast<igraph_vector_t*>(W), &v_out, 0x0, resolution, beta, 1, -1, &membership, &n_clusters, &quality);
    // igraph_community_leiden_simple(G, const_cast<igraph_vector_t*>(W), IGRAPH_LEIDEN_OBJECTIVE_MODULARITY, resolution, beta, 1, -1, &membership, &n_clusters, &quality);
    igraph_community_leiden_simple(G, const_cast<igraph_vector_t*>(W), IGRAPH_LEIDEN_OBJECTIVE_ER, resolution, beta, 1, -1, &membership, &n_clusters, &quality);

    L.set_size(n);
    for (std::size_t i = 0; i < n; ++i)
        L[i] = static_cast<std::size_t>(VECTOR(membership)[static_cast<long>(i)]);

    igraph_vector_int_destroy(&membership);
    igraph_vector_destroy(&v_out);
}
