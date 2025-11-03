#include <loga/token.h>
#include <loga/collection.h>
#include <loga/tokenized_collection.h>
#include <loga/tokenized_alignment.h>
#include <loga/alignment.h>
#include <loga/tokenized_distance.h>
#include <loga/cluster.h>
#include <loga/path.h>
#include <loga/tokenized_group.h>
#include <loga/group.h>
#include <loga/tokenized_multi_alignment.h>
#include <loga/multi_alignment.h>
#include <iostream>
#include <filesystem>
#include <cereal/archives/portable_binary.hpp>
#include <igraph/igraph.h>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>

class parsed{
    std::size_t _id;
    std::size_t _cluster;
    prova::loga::tokenized_multi_alignment::interval_set _intervals;

public:
    inline explicit parsed(std::size_t id, std::size_t cluster, prova::loga::tokenized_multi_alignment::interval_set&& intervals): _id(id), _cluster(cluster), _intervals(std::move(intervals)) {}
    const prova::loga::tokenized_multi_alignment::interval_set& intervals() const { return _intervals; }
    std::size_t id() const { return _id; }
    std::size_t cluster() const { return _cluster; }
};

struct pattern_sequence {
    struct segment {
        prova::loga::tokenized _tokens;
        prova::loga::zone      _tag;

        explicit segment(const prova::loga::zone& tag): _tag(tag), _tokens(prova::loga::tokenized::nothing()) {}
        segment(const prova::loga::zone& tag, const std::string& str): _tag(tag),  _tokens(str) {}

        segment& operator=(const std::string& str) {
            _tokens = prova::loga::tokenized(str);
            return *this;
        }

        const prova::loga::zone& tag() const noexcept { return _tag; }
        const prova::loga::tokenized& tokens() const noexcept { return _tokens; }

        std::size_t hash() const { return std::hash<std::string>{}(_tokens.raw()); }
    };

    std::vector<segment> _segments;

    void add(segment&& s) { _segments.push_back(std::move(s)); }

    auto begin() noexcept { return _segments.begin(); }
    auto end() noexcept { return _segments.end(); }

    auto begin() const noexcept { return _segments.begin(); }
    auto end() const noexcept { return _segments.end(); }

    const segment& at(std::size_t i) const { return _segments.at(i); }

    std::size_t size() const noexcept { return _segments.size(); }
};


// void shadow_graph(const std::vector<pattern_sequence>& pseqs, std::size_t total_segments, const std::string& phase2_graphml_file_path) {
//     struct segment_vertex{
//         std::size_t _cluster;
//         std::size_t _segment;
//         std::string _type;
//         std::size_t _count   = 1;

//         segment_vertex(): _cluster(0), _segment(0) {}
//         segment_vertex(std::size_t cluster, std::size_t segment): _cluster(cluster), _segment(segment) {}
//     };

//     struct segment_edge{
//         std::string _type;
//         double _sim = 0;
//         std::size_t _count = 1;
//     };

//     using directed_graph_type = boost::adjacency_list<boost::setS, boost::vecS, boost::directedS, segment_vertex, segment_edge>;
//     directed_graph_type graph;
//     using vertex_type = boost::graph_traits<directed_graph_type>::vertex_descriptor;
//     using edge_type   = boost::graph_traits<directed_graph_type>::edge_descriptor;

//     std::vector<vertex_type> V(total_segments);
//     std::map<std::string, vertex_type> vertex_map;
//     std::size_t count_c = 0;
//     for(const auto& pseq: pseqs) {
//         vertex_type initial = boost::add_vertex(graph);
//         graph[initial]._type = "initial";
//         graph[initial]._cluster = count_c;
//         graph[initial]._segment = 0;

//         std::size_t count_s = 0;
//         vertex_type last;
//         for(auto it = pseq.begin(); it != pseq.end(); ++it) {
//             // std::cout << seq.tag() << seq.tokens().raw();
//             // create vertex
//             const std::string& segment_str = pseqs.at(count_c).at(count_s).tokens().raw();
//             vertex_type u; // generic vertex for a segment
//             auto vmap_it = vertex_map.find(segment_str);
//             if(vmap_it == vertex_map.end()) {
//                 u = boost::add_vertex(graph);
//                 graph[u]._type = "generic";
//                 graph[u]._cluster = count_c;
//                 graph[u]._segment = count_s;
//                 vertex_map.insert(std::make_pair(segment_str, u));
//             } else {
//                 u = vmap_it->second;
//                 graph[u]._count++;
//             }

//             vertex_type v = boost::add_vertex(graph); // specialized vertex for this sequence
//             graph[v]._type = "special";
//             {
//                 edge_type is_a_e;
//                 bool specialization_v_inserted;
//                 std::tie(is_a_e, specialization_v_inserted) = boost::add_edge(u, v, graph);
//                 assert(specialization_v_inserted);
//                 graph[is_a_e]._type = "is-a";
//                 std::tie(is_a_e, specialization_v_inserted) = boost::add_edge(v, u, graph);
//                 assert(specialization_v_inserted);
//                 graph[is_a_e]._type = "is-a";
//             }

//             graph[v]._cluster = count_c;
//             graph[v]._segment = count_s;

//             if(it != pseq.begin()) {
//                 // create edge
//                 // must be the previously added vertex
//                 auto [e, exists] = boost::edge(last, v, graph);
//                 if(!exists) {
//                     bool inserted;
//                     std::tie(e, inserted) = boost::add_edge(last, v, graph);
//                     assert(inserted);
//                     graph[e]._type = "chain";
//                     graph[e]._sim   = 1;
//                 } else {
//                     graph[e]._count++;
//                 }
//             } else {
//                 auto [e, exists] = boost::edge(initial, v, graph);
//                 if(!exists) {
//                     bool inserted;
//                     std::tie(e, inserted) = boost::add_edge(initial, v, graph);
//                     assert(inserted);
//                     graph[e]._type = "chain";
//                     graph[e]._sim   = 1;
//                 } else {
//                     graph[e]._count++;
//                 }
//             }

//             last = v;
//             V.push_back(v);

//             ++count_s;
//         }

//         auto [e, exists] = boost::edge(last, initial, graph);
//         if(!exists) {
//             bool inserted;
//             std::tie(e, inserted) = boost::add_edge(last, initial, graph);
//             assert(inserted);
//             graph[e]._type = "chain";
//             graph[e]._sim   = 1;
//         } else {
//             graph[e]._count++;
//         }
//         // std::cout << std::endl;
//         ++count_c;
//     }

//     // for(auto i = V.cbegin(); i != V.cend(); ++i) {
//     //     for(auto j = V.cbegin(); j != V.cend(); ++j) {
//     //         if(i != j) {
//     //             auto u = graph[*i];
//     //             auto v = graph[*j];

//     //             const auto& l_seg = pseqs.at(u._cluster).at(u._segment);
//     //             const auto& r_seg = pseqs.at(v._cluster).at(v._segment);

//     //             const std::string& l_seg_str = l_seg.tokens().raw();
//     //             const std::string& r_seg_str = r_seg.tokens().raw();

//     //             auto [_, exists] = boost::edge(*i, *j, graph);
//     //             if(!exists){
//     //                 std::size_t max_similarity = std::max(l_seg_str.size(), r_seg_str.size());

//     //                 auto [e, inserted] = boost::add_edge(*i, *j, graph);
//     //                 double similarity = prova::loga::lcs(l_seg_str.cbegin(), l_seg_str.cend(), r_seg_str.cbegin(), r_seg_str.cend());
//     //                 if(similarity > 0) {
//     //                     graph[e]._chain = false;
//     //                     graph[e]._sim = similarity/max_similarity;
//     //                 }
//     //             }
//     //         }
//     //     }
//     // }

//     auto vertex_label_map = boost::make_function_property_map<vertex_type>(
//         [&](const vertex_type& v) -> std::string {
//             std::size_t c = graph[v]._cluster;
//             if(graph[v]._type == "initial") return std::format("<{}>", c);

//             std::size_t s = graph[v]._segment;

//             const auto& seq = pseqs.at(c);
//             const auto& seg = seq.at(s);

//             const std::string& label = seg.tokens().raw();
//             return (label == " ") ? "<SPACE>" : label;
//         }
//     );

//     auto edge_weight_map = boost::make_function_property_map<edge_type>(
//         [&](const edge_type& e) -> double {
//             return graph[e]._count;
//         }
//     );

//     auto vertex_weight_map = boost::make_function_property_map<vertex_type>(
//         [&](const vertex_type& v) -> double {
//             return graph[v]._count;
//         }
//     );

//     auto vertex_type_map = boost::make_function_property_map<vertex_type>(
//         [&](const vertex_type& v) -> std::string {
//             return graph[v]._type;
//         }
//     );

//     auto edge_type_map = boost::make_function_property_map<edge_type>(
//         [&](const edge_type& e) -> std::string {
//             return graph[e]._type;
//         }
//     );

//     boost::dynamic_properties properties;
//     properties.property("label",  vertex_label_map);
//     properties.property("weight", edge_weight_map);
//     properties.property("weight", vertex_weight_map);
//     properties.property("type", edge_type_map);
//     properties.property("type", vertex_type_map);

//     std::ofstream stream(phase2_graphml_file_path);
//     boost::write_graphml(stream, graph, properties, true);

//     // If Initial initial vertex c_j is reachable from initial vertex c_i using the same path as c_i -> c_i cycle then {c_{i}, c_{j}} are same and should be merged.
// }

// void generic_graph(const std::vector<pattern_sequence>& pseqs, std::size_t total_segments, const std::string& phase2_graphml_file_path) {
//     struct segment_vertex{
//         std::string _str;
//         std::string _type;
//         std::size_t _count   = 1;

//         segment_vertex() {}
//         segment_vertex(const std::string& type): _type(type) {}
//         segment_vertex(const std::string& type, const std::string& str): _type(type), _str(str) {}
//     };

//     struct segment_edge{
//         std::string _type;
//         std::size_t _count = 1;

//         segment_edge() {}
//         segment_edge(const std::string& type): _type(type) {}
//     };

//     using directed_graph_type = boost::adjacency_list<boost::setS, boost::vecS, boost::directedS, segment_vertex, segment_edge>;
//     directed_graph_type graph;
//     using vertex_type = boost::graph_traits<directed_graph_type>::vertex_descriptor;
//     using edge_type   = boost::graph_traits<directed_graph_type>::edge_descriptor;

//     std::vector<vertex_type> V(total_segments);
//     std::map<std::string, vertex_type> vertex_map;
//     std::size_t count_c = 0;
//     for(const auto& pseq: pseqs) {
//         vertex_type initial = boost::add_vertex(graph);
//         graph[initial]._type = "initial";
//         graph[initial]._str = std::format("<{}>", count_c);

//         std::size_t count_s = 0;
//         vertex_type last;
//         for(auto it = pseq.begin(); it != pseq.end(); ++it) {
//             const std::string& segment_str = pseqs.at(count_c).at(count_s).tokens().raw();
//             vertex_type u; // generic vertex for a segment
//             auto vmap_it = vertex_map.find(segment_str);
//             if(vmap_it == vertex_map.end()) {
//                 u = boost::add_vertex(graph);
//                 graph[u]._type = "generic";
//                 graph[u]._str = segment_str;
//                 vertex_map.insert(std::make_pair(segment_str, u));
//             } else {
//                 u = vmap_it->second;
//                 graph[u]._count++;
//             }

//             vertex_type l = (it == pseq.begin()) ? initial : last;
//             auto [e, exists] = boost::edge(l, u, graph);
//             if(!exists) {
//                 bool inserted;
//                 std::tie(e, inserted) = boost::add_edge(l, u, graph);
//                 assert(inserted);
//                 graph[e]._type = "chain";
//             } else {
//                 graph[e]._count++;
//             }

//             last = u;
//             V.push_back(u);

//             ++count_s;
//         }

//         auto [e, exists] = boost::edge(last, initial, graph);
//         if(!exists) {
//             bool inserted;
//             std::tie(e, inserted) = boost::add_edge(last, initial, graph);
//             assert(inserted);
//             graph[e]._type = "chain";
//         } else {
//             graph[e]._count++;
//         }
//         ++count_c;
//     }

//     // for(auto i = V.cbegin(); i != V.cend(); ++i) {
//     //     for(auto j = V.cbegin(); j != V.cend(); ++j) {
//     //         if(i != j) {
//     //             auto u = graph[*i];
//     //             auto v = graph[*j];

//     //             const auto& l_seg = pseqs.at(u._cluster).at(u._segment);
//     //             const auto& r_seg = pseqs.at(v._cluster).at(v._segment);

//     //             const std::string& l_seg_str = l_seg.tokens().raw();
//     //             const std::string& r_seg_str = r_seg.tokens().raw();

//     //             auto [_, exists] = boost::edge(*i, *j, graph);
//     //             if(!exists){
//     //                 std::size_t max_similarity = std::max(l_seg_str.size(), r_seg_str.size());

//     //                 auto [e, inserted] = boost::add_edge(*i, *j, graph);
//     //                 double similarity = prova::loga::lcs(l_seg_str.cbegin(), l_seg_str.cend(), r_seg_str.cbegin(), r_seg_str.cend());
//     //                 if(similarity > 0) {
//     //                     graph[e]._chain = false;
//     //                     graph[e]._sim = similarity/max_similarity;
//     //                 }
//     //             }
//     //         }
//     //     }
//     // }

//     auto vertex_label_map = boost::make_function_property_map<vertex_type>(
//         [&](const vertex_type& v) -> std::string {
//             const std::string& label = graph[v]._str;
//             return (label == " ") ? "<SPACE>" : label;
//         }
//     );

//     auto edge_weight_map = boost::make_function_property_map<edge_type>(
//         [&](const edge_type& e) -> double {
//             return graph[e]._count;
//         }
//     );

//     auto vertex_weight_map = boost::make_function_property_map<vertex_type>(
//         [&](const vertex_type& v) -> double {
//             return graph[v]._count;
//         }
//     );

//     auto vertex_type_map = boost::make_function_property_map<vertex_type>(
//         [&](const vertex_type& v) -> std::string {
//             return graph[v]._type;
//         }
//     );

//     auto edge_type_map = boost::make_function_property_map<edge_type>(
//         [&](const edge_type& e) -> std::string {
//             return graph[e]._type;
//         }
//     );

//     boost::dynamic_properties properties;
//     properties.property("label",  vertex_label_map);
//     properties.property("weight", edge_weight_map);
//     properties.property("weight", vertex_weight_map);
//     properties.property("type", edge_type_map);
//     properties.property("type", vertex_type_map);

//     std::ofstream stream(phase2_graphml_file_path);
//     boost::write_graphml(stream, graph, properties, true);

//     // If Initial initial vertex c_j is reachable from initial vertex c_i using the same path as c_i -> c_i cycle then {c_{i}, c_{j}} are same and should be merged.
// }

int main(int argc, char** argv) {
    boost::program_options::variables_map vm;
    try{
        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
            ("help,h",            "Print help message")
            ("input",             boost::program_options::value<std::string>(),                "Log file path")
        ;
        boost::program_options::positional_options_description p;
        p.add("input", 1);

        auto options = boost::program_options::command_line_parser(argc, argv).options(desc).positional(p).run();
        boost::program_options::store(options, vm);
        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << "Usage: " << argv[0] << " <path>\n";
            std::cout << desc << "\n";
            return 0;
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    if (!vm.count("input")) {
        throw boost::program_options::error("missing required positional argument <path>");
    }

    prova::loga::tokenized_collection collection;
    std::string log_path = vm["input"].as<std::string>();

    std::string archive_file_path = std::format("{}.1g.paths",    log_path);
    std::string dist_file_path    = std::format("{}.lev.dmat",    log_path);
    std::string graphml_file_path = std::format("{}.1nn.graphml", log_path);
    std::string phase2_graphml_file_path = std::format("{}.p2.graphml", log_path);
    std::string phase1_res_file_path = std::format("{}.p1.res", log_path);

    std::ifstream log(log_path);
    collection.parse(log);

    prova::loga::tokenized_alignment::matrix_type all_paths;
    if(std::filesystem::exists(archive_file_path)){
        std::ifstream archive_file(archive_file_path);
        cereal::PortableBinaryInputArchive archive(archive_file);
        archive(all_paths);
    } else {
        std::ofstream archive_file(archive_file_path);
        cereal::PortableBinaryOutputArchive archive(archive_file);
        archive(all_paths);
    }
    prova::loga::tokenized_distance distance(all_paths, collection.count());
    if(std::filesystem::exists(dist_file_path)){
        std::ifstream dist_file(dist_file_path);
        if(!distance.load(dist_file)){
            std::cout << "failed to load dmat" << std::endl;
            return 1;
        }
    } else {
        distance.compute(collection);
        std::ofstream dist_file(dist_file_path);
        if(!distance.save(dist_file)){
            std::cout << "failed to save dmat" << std::endl;
            return 1;
        }
    }
    prova::loga::tokenized_distance::distance_matrix_type dmat = distance.matrix();
    assert(dmat.n_rows == collection.count());
    assert(dmat.n_cols == collection.count());

    auto bgl_graph = distance.knn_graph(1, false);
    {
        std::ofstream graphml(graphml_file_path);
        prova::loga::print_graphml(bgl_graph, graphml);
    }
    igraph_t igraph;
    igraph_vector_t edges;
    prova::loga::bgl_to_igraph(bgl_graph, &igraph, &edges);
    prova::loga::cluster::labels_type labels(collection.count());
    prova::loga::leiden_membership(&igraph, &edges, labels);

    const std::array<std::string, 209> label_names = {
        // Fruits (40) - Alphabetical
        "Apple", "Apricot", "Avocado", "Banana", "Blackberry", "Blueberry", "Cantaloupe", "Cherry", "Coconut",
        "Cranberry", "Date", "Dragonfruit", "Fig", "Grape", "Grapefruit", "Guava", "Honeydew", "Jackfruit", "Kiwi",
        "Lemon", "Lime", "Lychee", "Mango", "Mandarin", "Mulberry", "Nectarine", "Orange", "Papaya", "Peach",
        "Pear", "Persimmon", "Pineapple", "Plum", "Pomegranate", "Raspberry", "Starfruit", "Strawberry",
        "Tangerine", "Watermelon",

        // Flowers (40) - Alphabetical (with "Heather" replacing the stray "Sweet")
        "Anemone", "Aster", "Azalea", "Begonia", "Bluebell", "Buttercup", "Camellia", "Carnation", "Chrysanthemum",
        "Daffodil", "Dahlia", "Daisy", "Foxglove", "Freesia", "Gardenia", "Geranium", "Gladiolus", "Heather",
        "Hibiscus", "Hyacinth", "Hydrangea", "Iris", "Jasmine", "Lavender", "Lilac", "Lily", "Lotus", "Magnolia",
        "Marigold", "Orchid", "Pansy", "Peony", "Petunia", "Poppy", "Primrose", "Rose", "Snapdragon", "Sunflower",
        "Tulip", "Violet",

        // Animals (40) - Alphabetical
        "Bear", "Buffalo", "Camel", "Cat", "Cheetah", "Chicken", "Chimpanzee", "Cow", "Deer", "Dog",
        "Dolphin", "Donkey", "Duck", "Eagle", "Elephant", "Falcon", "Fox", "Giraffe", "Goat", "Goose",
        "Gorilla", "Hippopotamus", "Horse", "Kangaroo", "Koala", "Leopard", "Lion", "Monkey", "Owl", "Panda",
        "Penguin", "Pig", "Rabbit", "Rhinoceros", "Sheep", "Shark", "Tiger", "Whale", "Wolf", "Zebra",

        // Trees (20) - Alphabetical
        "Ash", "Bamboo", "Baobab", "Birch", "Cedar", "Cypress", "Elm", "Fir", "Maple", "Oak",
        "Olive", "Palm", "Pine", "Poplar", "Redwood", "Sequoia", "Spruce", "Teak", "Walnut", "Willow",

        // Insects (20) - Alphabetical
        "Ant", "Aphid", "Bee", "Beetle", "Butterfly", "Cockroach", "Cricket", "Dragonfly", "Earwig", "Firefly",
        "Flea", "Fly", "Gnat", "Grasshopper", "Ladybug", "Locust", "Mantis", "Mosquito", "Moth", "Termite",

        // Fish (20) - Alphabetical
        "Anchovy", "Bass", "Carp", "Catfish", "Cod", "Eel", "Goldfish", "Haddock", "Halibut", "Herring",
        "Mackerel", "Perch", "Pike", "Salmon", "Sardine", "Swordfish", "Tilapia", "Trout", "Tuna", "Walleye",

        // Planets (9) - Alphabetical
        "Earth", "Jupiter", "Mars", "Mercury", "Neptune", "Pluto", "Saturn", "Uranus", "Venus",

        // Periodic Table Elements (20) - Alphabetical
        "Calcium", "Carbon", "Chlorine", "Copper", "Fluorine", "Gold", "Helium", "Hydrogen", "Iron", "Lithium",
        "Magnesium", "Neon", "Nitrogen", "Oxygen", "Phosphorus", "Potassium", "Silicon", "Silver", "Sodium", "Sulfur"
    };
    const std::size_t label_names_max_size = std::ranges::max_element(label_names, [](const std::string& l, const std::string& r){return l.size() < r.size();})->size();

    std::map<std::size_t, parsed> parsed_log, patterns;
    prova::loga::tokenized_group group(collection, labels);
    std::cout << "Clusters: " << group.labels() << std::endl;
    for(std::size_t c = 0; /*c != group.labels()*/; ++c) {
        if(c == group.labels()) {
            if(group.unclustered() == 0){
                break;
            } else {
                prova::loga::tokenized_group::label_proxy proxy = group.proxy(std::numeric_limits<std::size_t>::max());
                std::cout << std::format("Unlabeled: ({})", proxy.count()) << std::endl;
                for(std::size_t i = 0; i != proxy.count(); ++i) {
                    auto v = proxy.at(i);
                    std::cout << v.str() << std::endl;
                }
                break;
            }
        }
        prova::loga::tokenized_group::label_proxy proxy = group.proxy(c);
        std::size_t count = proxy.count();

        std::string cluster_name = (c < std::numeric_limits<std::size_t>::max()) ? ((c < label_names.size()) ? label_names.at(c) : std::format("C{}", c)) : " Failed ";
        std::cout << std::endl << prova::loga::colors::bright_yellow << "◪" << prova::loga::colors::reset << " Label: " << cluster_name << std::format(" ({})", count) << std::endl;
        // std::cout << std::resetiosflags(std::ios::showbase) << std::right << std::setw(3) << "*" << min_matched_id << "|" << "\033[4m" << collection.at(min_matched_id) << "\033[0m" << std::resetiosflags(std::ios::showbase) << std::endl;

        prova::loga::tokenized_collection subcollection;
        std::vector<std::size_t> references;                                    // global ids of all items belonging to the same cluster
        for(std::size_t i = 0; i < count; ++i) {
            prova::loga::tokenized_group::label_proxy::value v = proxy.at(i);   // v: {str, id} where the id is the global id and the str is the string (not token list) value of the item
            const std::string& str = v.str();
            subcollection.add(str);
            references.push_back(v.id());
        }

        std::vector<std::string> outliers;
        std::vector<double> confidence;
        arma::vec lof(count, arma::fill::none);
        if(count > 2){
            std::size_t K = std::min<std::size_t>(3, count -1);
            arma::mat local_distances(count, count, arma::fill::zeros);
            arma::vec kdist(count, arma::fill::none);
            {
                std::condition_variable observer;
                std::mutex mutex;
                std::atomic_uint32_t jobs_completed = 0;
                boost::asio::thread_pool pool(std::thread::hardware_concurrency());
                for (std::size_t i = 0; i < count; ++i) {
                    boost::asio::post(pool, [i,  count, &subcollection, &local_distances, &kdist, K, &jobs_completed, &observer]() {
                        for (std::size_t j = i + 1; j < count; ++j) {
                            const std::string& si = subcollection.at(i).raw();
                            const std::string& sj = subcollection.at(j).raw();

                            // std::size_t lcs_len  = prova::loga::lcs(si.cbegin(), si.cend(), sj.cbegin(), sj.cend());
                            // std::size_t lcs_dist = std::max(si.size(), sj.size()) - lcs_len;
                            // local_distances(i,j) = local_distances(j,i) = lcs_dist;

                            // local_distances(i,j) = local_distances(j,i) = dmat(i, j);

                            std::size_t lev_dist = prova::loga::levenshtein_distance(si.cbegin(), si.cend(), sj.cbegin(), sj.cend());
                            local_distances(i,j) = local_distances(j,i) = lev_dist;// /std::max(si.size(), sj.size());
                        }
                        // arma::rowvec sorted = arma::sort(local_distances.row(i), "ascend");
                        // kdist(i) = sorted(K);
                        jobs_completed.fetch_add(1);
                        observer.notify_one();
                    });
                }

                std::uint32_t printed = 0;
                while (printed < count) {
                    std::unique_lock<std::mutex> lock(mutex);
                    observer.wait(lock, [&]{
                        return jobs_completed.load() > printed;
                    });

                    const auto upto = jobs_completed.load();
                    while (printed < upto) {
                        ++printed;
                        // std::cout << std::format("\rDistance Matrix rows {}/{}", printed, _count) << std::flush;

                        double percent = ((double)printed / (double)count) * 10.0f;
                        std::string progress(20, '|');
                        std::fill(progress.begin()+(((int)percent)*2), progress.end(), '~');
                        std::cout << std::format("\r{:.2f}% {}", percent*10, progress) << std::flush;
                    }
                }

                pool.join();
                std::cout << std::endl << std::endl << std::flush;
                std::cout.flush();
            }
            {
                boost::asio::thread_pool pool(std::thread::hardware_concurrency());
                for (std::size_t i = 0; i < count; ++i) {
                    boost::asio::post(pool, [i, K, &local_distances, &kdist]() {
                        arma::rowvec sorted = arma::sort(local_distances.row(i), "ascend");
                        kdist(i) = sorted(K);
                    });
                }
                pool.join();
            }

            arma::field<arma::uvec> neighbours(count);  // each entry can have a different length
            for (std::size_t i = 0; i < count; ++i) {
                arma::uvec idx = arma::find(local_distances.row(i).t() <= kdist(i));
                idx = idx(arma::find(idx != i));
                neighbours(i) = idx;
                assert(neighbours(i).n_elem >= K);
            }

            auto reach = [&](std::size_t x, std::size_t y) {
                return std::max(kdist(y), local_distances(x, y));
            };

            arma::vec lrd(count, arma::fill::none);
            for (arma::uword i = 0; i < count; ++i) {
                const arma::uvec& locals_i = neighbours(i);
                arma::vec r(locals_i.n_elem);
                for (std::size_t t = 0; t < locals_i.n_elem; ++t)
                    r(t) = reach(i, locals_i(t));

                double mrd = arma::mean(r);
                lrd(i) = 1.0 / std::max(mrd, std::numeric_limits<double>::epsilon());
            }

            for (arma::uword i = 0; i < count; ++i) {
                const arma::uvec& N = neighbours(i);
                double avgNbr = arma::mean(lrd(N));
                lof(i) = avgNbr / lrd(i);
            }

            // std::cout << lof << std::endl;
            std::vector<std::size_t> excluded;
            for (arma::uword i = 0; i < count; ++i) {
                if(lof(i) >= 1.1f){
                    excluded.push_back(i);
                }
                confidence.push_back(lof(i));
            }

            for(auto it = excluded.rbegin(); it != excluded.rend(); ++it) {
                std::size_t index = *it;
                outliers.push_back(subcollection.at(index).raw());
                subcollection.remove(index);
                references.erase(references.begin() + index);
                confidence.erase(confidence.begin() + index);
            }
        }

        std::size_t base = 0;
        std::size_t cache_hits = 0;
        prova::loga::tokenized_alignment::matrix_type paths;
        std::size_t ref_c_i = 0;
        for(auto ref_i = references.cbegin(); ref_i != references.cend(); ++ref_i) {
            std::size_t ref_c_j = 0;
            for(auto ref_j = references.cbegin(); ref_j != references.cend(); ++ref_j) {
                if(ref_i != ref_j){
                    auto global_key = std::make_pair(*ref_i, *ref_j);
                    auto it = all_paths.find(global_key);
                    if(it != all_paths.cend()) {
                        if(ref_c_i == base) {
                            auto local_key = std::make_pair(ref_c_i, ref_c_j);
                            paths.insert(std::make_pair(local_key, it->second));
                            ++cache_hits;
                        }
                    }
                }
                ++ref_c_j;
            }
            ++ref_c_i;
        }
        std::cout << std::format("Cache hits {}/{}", cache_hits, references.size()-1) << std::endl;

        prova::loga::tokenized_alignment subalignment(subcollection);
        subalignment.bubble_all_pairwise(paths, subcollection.begin(), 1);

        for(const auto& [key, path]: paths) {
            auto global_key = std::make_pair(references.at(key.first), references.at(key.second));
            if(!all_paths.contains(global_key)) {
                all_paths.insert(std::make_pair(global_key, path));
            }
        }

        prova::loga::tokenized_multi_alignment malign(subcollection, paths, base);
        // prova::loga::tokenized_multi_alignment::region_map regions = malign.align();

        prova::loga::tokenized_multi_alignment::region_map regions = malign.align(paths.begin(), paths.end(), [&all_paths, &references, &subcollection, &subalignment](std::size_t id /* id is the local id of the item in the cluster*/){
            auto ref_it = references.begin();
            std::advance(ref_it, id);
            prova::loga::tokenized_alignment::matrix_type local_paths;

            auto ref_i = ref_it;
            std::size_t ref_c_j = 0;
            std::size_t local_cache_hits = 0;
            for(auto ref_j = references.cbegin(); ref_j != references.cend(); ++ref_j) {
                if(ref_i != ref_j){
                    auto global_key = std::make_pair(*ref_i, *ref_j);
                    auto it = all_paths.find(global_key);
                    if(it != all_paths.cend()) {
                        auto local_key = std::make_pair(id, ref_c_j);
                        local_paths.insert(std::make_pair(local_key, it->second));
                        ++local_cache_hits;
                    }
                }
                ++ref_c_j;
            }

            std::cout << std::format("Cache hits {}/{}", local_cache_hits, references.size()-1) << std::endl;
            prova::loga::tokenized_alignment::memo_type memo;
            auto pivot = subcollection.begin();
            std::advance(pivot, id);
            subalignment.bubble_all_pairwise_ref(local_paths, pivot, 1);

            for(const auto& [key, path]: local_paths) {
                auto global_key = std::make_pair(references.at(key.first), references.at(key.second));
                if(!all_paths.contains(global_key)) {
                    all_paths.insert(std::make_pair(global_key, path));
                }
            }

            auto max_elem = std::ranges::max_element(local_paths, [](const auto& pair_l, const auto& pair_r){
                return pair_l.second.size() < pair_r.second.size();
            });
            return max_elem->second;
        });

        // regions = malign.fixture_word_boundary(regions);
        // malign.print_regions_string(regions, std::cout) << std::endl;
        const auto& base_zones = regions.at(0);
        std::size_t placeholder_count = 0;
        std::cout << std::right << std::setw(5) << "     " << prova::loga::colors::bright_yellow << "●" << prova::loga::colors::reset << " " << std::resetiosflags(std::ios::showbase);
        for(const auto& z: base_zones) {
            prova::loga::zone tag = *z.second.cbegin(); // set has only one item
            std::size_t offset = z.first.lower();
            std::size_t len = z.first.upper()-z.first.lower();
            std::string substr = subcollection.at(0).subset(offset, len).view();
            if(tag == prova::loga::zone::constant)
                std::cout << substr;
            else {
                const auto& color = prova::loga::colors::palette.at(placeholder_count % prova::loga::colors::palette.size());
                std::cout << color << std::format("${}", placeholder_count) << prova::loga::colors::reset;
                ++placeholder_count;
            }
        }
        std::cout << prova::loga::colors::reset;
        std::cout << std::endl << "------------------------------------------" << std::endl;
        for(const auto& [id, zones]: regions) {
            std::cout << std::right << std::setw(5) << id;
            if(count > 2) {
                std::cout << ":" << std::fixed << std::setprecision(2) << std::abs(confidence.at(id) - 1.0f);
            }
            std::cout << "|" << std::resetiosflags(std::ios::showbase);
            prova::loga::tokenized_multi_alignment::print_interval_set(zones, subcollection.at(id), std::cout);
            std::cout << std::endl;
        }

        if(outliers.size() > 0){
            std::cout << prova::loga::colors::bright_red << "    Excluded " << outliers.size() << std::fixed << std::setprecision(2) << lof.t() << prova::loga::colors::reset << std::endl;
            for(const std::string& outlier: outliers){
                std::cout << prova::loga::colors::bright_red << "    X| " << prova::loga::colors::reset << outlier << std::endl;
            }
            std::cout << std::endl;
        }

        auto pattern_zones = base_zones;
        parsed cluster_pattern(references.at(0), c, std::move(pattern_zones));
        patterns.insert(std::make_pair(c, cluster_pattern));
        // for(auto& [id, zones]: regions) {
        //     parsed p(id, c, std::move(zones));
        //     parsed_log.emplace(std::make_pair(id, p));
        // }
    }

    {
        std::ofstream archive_file(archive_file_path);
        cereal::PortableBinaryOutputArchive archive(archive_file);
        archive(all_paths);
    }

    std::cout << "----------------------------" << std::endl;
    std::cout << "Summary: " << std::format("{} clusters", group.labels()) << std::endl;
    std::cout << "----------------------------" << std::endl;

    std::ofstream phase1_res(phase1_res_file_path);
    std::size_t total_segments = 0;
    std::vector<pattern_sequence> pseqs;
    for(const auto& [c, p]: patterns) {
        std::string cluster_name = (c < std::numeric_limits<std::size_t>::max()) ? ((c < label_names.size()) ? label_names.at(c) : std::format("C{}", c)) : " Failed ";
        const auto& base_zones = p.intervals();
        std::size_t placeholder_count = 0;
        std::cout << std::setw(label_names_max_size) << cluster_name << " " << prova::loga::colors::bright_yellow << "●" << prova::loga::colors::reset << " " << std::resetiosflags(std::ios::showbase);
        pattern_sequence pseq;
        for(const auto& z: base_zones) {
            prova::loga::zone tag = *z.second.cbegin(); // set has only one item
            std::size_t offset = z.first.lower();
            std::size_t len = z.first.upper()-z.first.lower();
            std::string substr = collection.at(p.id()).subset(offset, len).view();
            pattern_sequence::segment seg(tag);
            if(tag == prova::loga::zone::constant){
                std::cout << substr;
                phase1_res << substr;
                seg = substr;
            } else {
                const auto& color = prova::loga::colors::palette.at(placeholder_count % prova::loga::colors::palette.size());
                std::cout << color << std::format("${}", placeholder_count) << prova::loga::colors::reset;
                ++placeholder_count;
                seg = "$";
                phase1_res << "$";
            }
            ++total_segments;
            pseq.add(std::move(seg));
        }
        std::cout << std::endl;
        phase1_res << std::endl;
        pseqs.emplace_back(std::move(pseq));
    }

    // Build a hypergraph considering each of these patterns as a vertex
    // There will be multiple edges between a pair of vertices
    // each edge will designate relation between two segments and associated attributes
    // the objective is to find out generialized segments of one pattern that dominates other's

    // shadow_graph(pseqs, total_segments, phase2_graphml_file_path);
    // generic_graph(pseqs, total_segments, phase2_graphml_file_path);

    // std::multimap<std::size_t, std::size_t> path_hashes;
    // std::set<std::size_t> uniqs;
    // std::size_t candidate = 0;
    // for(const auto& pseq: pseqs) {
    //     std::size_t path_hash = 0;
    //     for(auto it = pseq.begin(); it != pseq.end(); ++it) {
    //         boost::hash_combine(path_hash, it->hash());
    //     }
    //     path_hashes.insert(std::make_pair(path_hash, candidate));
    //     uniqs.insert(path_hash);
    //     ++candidate;
    // }

    // std::size_t last_hash = std::numeric_limits<std::size_t>::max();

    // std::cout << "----------------------------" << std::endl;
    // std::cout << "Concise Summary L1: " << std::format("{}", uniqs.size()) << std::endl;
    // std::cout << "----------------------------" << std::endl;

    // for (const auto& [path_hash, candidate] : path_hashes) {
    //     if (path_hash == last_hash)
    //         continue; // skip duplicates with same key
    //     last_hash = path_hash;

    //     // Representative pattern
    //     const pattern_sequence& pseq = pseqs.at(candidate);

    //     // Print pattern contents again (same style as above)
    //     std::cout << std::setw(16)
    //               << std::format("{:x}", path_hash) << " "
    //               << prova::loga::colors::bright_yellow << "●"
    //               << prova::loga::colors::reset << " ";

    //     std::size_t placeholder_count = 0;
    //     for (const auto& seg : pseq) {
    //         if (seg.tag() == prova::loga::zone::constant) {
    //             std::cout << seg.tokens().raw();
    //         } else {
    //             const auto& color = prova::loga::colors::palette.at(placeholder_count % prova::loga::colors::palette.size());
    //             std::cout << color << std::format("${}", placeholder_count) << prova::loga::colors::reset;
    //             ++placeholder_count;
    //         }
    //     }
    //     std::cout << std::endl;
    // }

    return 0;
}
