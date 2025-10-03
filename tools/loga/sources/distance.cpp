#include <loga/distance.h>
#include <thread>
#include <boost/asio.hpp>
#include <armadillo>
#include <boost/graph/adjacency_list.hpp>
#include <boost/property_map/function_property_map.hpp>
#include <boost/property_map/dynamic_property_map.hpp>
#include <boost/graph/graphml.hpp>

prova::loga::distance::distance(const path_matrix_type &paths, std::size_t count): _paths(paths), _count(count) {}

double prova::loga::distance::dist(const collection &collection, std::size_t i, std::size_t j) const{
    if(i == j) {
        return 0.0;
    } else {
        key_type key{i, j};
        try{
            const prova::loga::path& path = _paths.at(key);
            double score = path.score(collection);
            if((i == 45 && j == 46)|| (i == 46 && j == 45)) {
                std::cout << std::format("{}, {} - > {}", i, j, 1-score) << std::endl;
            }
            // return 1-score;
            arma::rowvec vec(path.size());
            std::size_t counter = 0;
            for(const prova::loga::segment& s: path) {
                vec[counter++] = s.length();
            }
            double length = arma::norm(vec, 2);
            arma::rowvec utopia(1);
            utopia[0] = path.begin()->base(collection).size();
            return arma::norm(utopia) - length;
        } catch(const std::out_of_range& ex) {
            std::cout << std::format("key {}x{} out of range", i, j) << std::endl;
            throw ex;
        }
    }
}

// double prova::loga::distance::dist_sequential(std::size_t i, std::size_t j) const {
//     if(i == j) {
//         return 0.0;
//     } else {
//         // key_type key{i, j};
//         try{
//             arma::Row<double> scores_i(_count, arma::fill::zeros);
//             arma::Row<double> scores_j(_count, arma::fill::zeros);
//             for(std::size_t t = 0; t < _count; ++t){
//                 if(i == t) {
//                     scores_i[t] = 0;
//                     continue;
//                 }
//                 key_type key{i, t};
//                 const prova::loga::path& path = _paths.at(key);
//                 std::vector<double> lengths;
//                 for(const prova::loga::segment& s: path) {
//                     double length = s.length();
//                     length = std::pow(length, 2);
//                     lengths.push_back(length);
//                 }
//                 double total = std::accumulate(lengths.cbegin(), lengths.cend(), 0);
//                 double norm  = std::sqrt(total);
//                 scores_i[t] = norm;
//             }

//             for(std::size_t t = 0; t < _count; ++t){
//                 if(j == t) {
//                     scores_j[t] = 0;
//                     continue;
//                 }
//                 key_type key{t, j};
//                 const prova::loga::path& path = _paths.at(key);
//                 std::vector<double> lengths;
//                 for(const prova::loga::segment& s: path) {
//                     double length = s.length();
//                     length = std::pow(length, 2);
//                     lengths.push_back(length);
//                 }
//                 double total = std::accumulate(lengths.cbegin(), lengths.cend(), 0);
//                 double norm  = std::sqrt(total);
//                 scores_j[t] = norm;
//             }
//             arma::Row<double> diffs(_count, arma::fill::zeros);
//             for(std::size_t t = 0; t < _count; ++t) {
//                 diffs[t] = std::pow(scores_i[t] - scores_j[t], 2);
//             }
//             double total = std::sqrt(arma::sum(diffs));
//             return total / _count;
//         } catch(const std::out_of_range& ex) {
//             std::cout << std::format("key {}x{} out of range", i, j) << std::endl;
//             throw ex;
//         }
//     }
// }

// double prova::loga::distance::dist_neighbourhood(std::size_t i, std::size_t j) const{
//     if(i == j) {
//         return 0.0;
//     } else {
//         key_type key{i, j};
//         try{
//             arma::Row<double> scores_i(_count, arma::fill::zeros);
//             arma::Row<double> scores_j(_count, arma::fill::zeros);
//             for(std::size_t t = 0; t < _count; ++t){
//                 if(i == t) {
//                     scores_i[t] = 1;
//                     continue;
//                 }
//                 key_type key{i, t};
//                 const prova::loga::path& path = _paths.at(key);
//                 scores_i[t] = path.score(); // score is [0, 1]
//             }
//             for(std::size_t t = 0; t < _count; ++t){
//                 if(j == t) {
//                     scores_j[t] = 1;
//                     continue;
//                 }
//                 key_type key{t, j};
//                 const prova::loga::path& path = _paths.at(key);
//                 scores_j[t] = path.score(); // score is [0, 1]
//             }
//             arma::Row<double> diffs(_count, arma::fill::zeros);
//             for(std::size_t t = 0; t < _count; ++t) {
//                 diffs[t] = std::pow(scores_i[t] - scores_j[t], 2);
//             }
//             double total = std::sqrt(arma::sum(diffs));
//             return total / _count;
//         } catch(const std::out_of_range& ex) {
//             std::cout << std::format("key {}x{} out of range", i, j) << std::endl;
//             throw ex;
//         }
//     }
// }


void prova::loga::distance::compute(const collection &collection, std::size_t threads){
    _distances.set_size(_count, _count);
    _distances.zeros();

    unsigned int T = !threads ? std::thread::hardware_concurrency() : threads;

    std::condition_variable observer;
    std::mutex mutex;
    std::atomic_uint32_t jobs_completed = 0;

    std::cout << "Computing distance matrix " << std::format("{}x{}", _count, _count)  << std::endl;

    boost::asio::thread_pool pool(T);
    for (std::size_t i = 0; i < _count; ++i) {
        boost::asio::post(pool, [this, i,  &jobs_completed, &observer, &collection]() {
            for (std::size_t j = 0; j < _count; ++j) {
                _distances(i, j) = dist(collection, i, j);
                // _distances(i, j) = dist_neighbourhood(i, j);
                // _distances(i, j) = dist_sequential(i, j);
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
            std::cout << std::format("\rDistance Matrix rows {}/{}", printed, _count) << std::flush;
        }
    }

    pool.join();

    for (std::size_t i = 0; i < _count; ++i) {
        auto row = _distances.row(i);
        std::set<double> distances_in_row;
        for(double d: row){
            distances_in_row.insert(d);
        }
        double min = 0;
        for(double d: distances_in_row) {
            if(d > 0) {
                min = d;
                break;
            }
        }
        for(std::size_t j = 0; j < _count; ++j) {
            double v = row[j];
            if(v != 0){
                row[j] = v - min;
            }
        }
    }

    std::cout << std::endl << std::endl << std::flush;
    std::cout.flush();
}

std::ostream& prova::loga::distance::print_graphml(std::ostream& stream) const {
    struct edge_props {
        double weight;
    };
    using graph_type  = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, boost::no_property, edge_props>;
    using vertex_type = boost::graph_traits<graph_type>::vertex_descriptor;
    using edge_type   = boost::graph_traits<graph_type>::edge_descriptor;

    graph_type graph;
    std::vector<vertex_type> V(_count);
    std::map<vertex_type, std::size_t> Vi;
    for(std::size_t i = 0; i != _count; ++i){
        V[i] = boost::add_vertex(graph);
        Vi.insert(std::make_pair(V[i], i));
    }

    for(std::size_t i = 0; i != _count; ++i){
        std::vector<std::pair<std::size_t, double>> weights;
        for(std::size_t j = 0; j != _count; ++j){
            auto w = _distances(i, j);
            weights.push_back(std::make_pair(j, w));
        }
        std::sort(weights.begin(), weights.end(), [](const std::pair<std::size_t, double>& l, const std::pair<std::size_t, double>& r){
            return l.second < r.second;
        });
        const auto neighbourhood = std::span(weights.cbegin(), 5);
        for(const auto& [j, w]: neighbourhood) {
            auto pair = boost::add_edge(V[i], V[j], graph);
            if(pair.second) {
                graph_type::edge_descriptor e = pair.first;
                graph[e].weight = w;
            }
        }
    }
    auto vertex_label_map = boost::make_function_property_map<vertex_type>(
        [&](const vertex_type& v) -> std::string {
            return boost::lexical_cast<std::string>(Vi.at(v));
        }
    );

    auto edge_weight_map = boost::make_function_property_map<edge_type>(
        [&](const edge_type& e) -> double {
            double w = graph[e].weight;
            if(w == 0) {
                return std::numeric_limits<double>::epsilon();
            }
            return w;
        }
    );

    boost::dynamic_properties properties;
    properties.property("label", vertex_label_map);
    properties.property("weight", edge_weight_map);

    boost::write_graphml(stream, graph, properties, true);
    return stream;
}

bool prova::loga::distance::computed() const {
    return _paths.size() > 0;
}

const prova::loga::distance::distance_matrix_type &prova::loga::distance::matrix() const { return _distances; }
