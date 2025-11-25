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
#include <loga/graph.h>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/copy.hpp>
#include <boost/algorithm/string/join.hpp>
#include <loga/community.h>
#include <loga/automata.h>
#include <loga/pattern_sequence.h>
#include <loga/outliers.h>

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

template <typename WeightT>
struct knn_digraph{
    using weight_type = std::conditional_t<std::is_integral_v<WeightT>, std::size_t, double>;
    using matrix_type = arma::Mat<weight_type>;

    struct vertex_property{
        std::size_t id;
    };

    struct edge_property{
        weight_type weight;
    };

    using digraph_type   = boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS, vertex_property, edge_property>;
    using vertex_type    = boost::graph_traits<digraph_type>::vertex_descriptor;
    using edge_type      = boost::graph_traits<digraph_type>::edge_descriptor;

    using undigraph_type = boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS, vertex_property, edge_property>;
    using unvertex_type  = boost::graph_traits<undigraph_type>::vertex_descriptor;
    using unedge_type    = boost::graph_traits<undigraph_type>::edge_descriptor;

    digraph_type _graph;
    std::vector<vertex_type> _vertices;

    knn_digraph(std::size_t N){
        _vertices.resize(N);
        for(std::size_t i = 0; i != N; ++i){
            vertex_type v = boost::add_vertex(_graph);
            _graph[v].id = i;
            _vertices[i] = v;
        }
    }

    const digraph_type& graph() const { return _graph; }

    void update(const matrix_type& coverage, const matrix_type& captures, std::size_t K, bool soft) {
        assert(coverage.n_rows == coverage.n_cols);
        assert(captures.n_rows == captures.n_cols);
        std::size_t N = coverage.n_rows;

        for(std::size_t i = 0; i != N; ++i){
            std::vector<std::pair<std::size_t, WeightT>> weights;
            for(std::size_t j = 0; j != N; ++j){
                if(i == j) continue;

                auto w = coverage(i, j) * captures(i, j);
                weights.push_back(std::make_pair(j, w));
            }
            std::sort(weights.begin(), weights.end(), [](const std::pair<std::size_t, WeightT>& l, const std::pair<std::size_t, WeightT>& r){
                return l.second > r.second;
            });
            // if !soft then neighbourhood is limited to size k which implies that degree of a vertex may
            //     eventually be much less than k due to presence of a reverse edge.
            // if soft then neighbourhood is unrestricted at first to enforce consistent neighbourhood size
            //     the k is enforced as a break condition
            const auto neighbourhood = std::span(weights.cbegin(), soft ? N-1 : std::min(K, N - 1));
            std::size_t counter = 0;
            for(const auto& [j, w]: neighbourhood) {
                vertex_type u = _vertices[i];
                vertex_type v = _vertices[j];

                if(w > 0) {
                    auto [e, inserted] = boost::add_edge(u, v, _graph);
                    if (inserted) {
                        _graph[e].weight = w;
                        ++counter;
                    } else {
                        _graph[e].weight = std::min(_graph[e].weight, w);
                    }
                }

                if(soft && counter >= K) {
                    break;
                }
            }
        }
    }

    undigraph_type make_undirected() const {
        undigraph_type g;
        boost::copy_graph(_graph, g);
        return g;
    }

    std::size_t weekly_connected_components(std::map<vertex_type, int>& component_map) const {
        std::vector<int> comp;
        undigraph_type g = make_undirected();
        comp.resize(boost::num_vertices(g));
        auto compmap = boost::make_iterator_property_map(comp.begin(), get(boost::vertex_index, g));
        std::size_t n_components = boost::connected_components(g, compmap);
        for(std::size_t i = 0; i != comp.size(); ++i) {
            unvertex_type v = boost::vertex(i, g);
            std::size_t id = g[v].id;
            vertex_type v_di = _vertices.at(id);
            component_map.insert(std::make_pair(v_di, comp.at(i)));
        }
        return n_components;
    }

};



// constexpr const std::array<std::string, 209> label_names = {
//     // Fruits (40) - Alphabetical
//     "Apple", "Apricot", "Avocado", "Banana", "Blackberry", "Blueberry", "Cantaloupe", "Cherry", "Coconut",
//     "Cranberry", "Date", "Dragonfruit", "Fig", "Grape", "Grapefruit", "Guava", "Honeydew", "Jackfruit", "Kiwi",
//     "Lemon", "Lime", "Lychee", "Mango", "Mandarin", "Mulberry", "Nectarine", "Orange", "Papaya", "Peach",
//     "Pear", "Persimmon", "Pineapple", "Plum", "Pomegranate", "Raspberry", "Starfruit", "Strawberry",
//     "Tangerine", "Watermelon",

//     // Flowers (40) - Alphabetical (with "Heather" replacing the stray "Sweet")
//     "Anemone", "Aster", "Azalea", "Begonia", "Bluebell", "Buttercup", "Camellia", "Carnation", "Chrysanthemum",
//     "Daffodil", "Dahlia", "Daisy", "Foxglove", "Freesia", "Gardenia", "Geranium", "Gladiolus", "Heather",
//     "Hibiscus", "Hyacinth", "Hydrangea", "Iris", "Jasmine", "Lavender", "Lilac", "Lily", "Lotus", "Magnolia",
//     "Marigold", "Orchid", "Pansy", "Peony", "Petunia", "Poppy", "Primrose", "Rose", "Snapdragon", "Sunflower",
//     "Tulip", "Violet",

//     // Animals (40) - Alphabetical
//     "Bear", "Buffalo", "Camel", "Cat", "Cheetah", "Chicken", "Chimpanzee", "Cow", "Deer", "Dog",
//     "Dolphin", "Donkey", "Duck", "Eagle", "Elephant", "Falcon", "Fox", "Giraffe", "Goat", "Goose",
//     "Gorilla", "Hippopotamus", "Horse", "Kangaroo", "Koala", "Leopard", "Lion", "Monkey", "Owl", "Panda",
//     "Penguin", "Pig", "Rabbit", "Rhinoceros", "Sheep", "Shark", "Tiger", "Whale", "Wolf", "Zebra",

//     // Trees (20) - Alphabetical
//     "Ash", "Bamboo", "Baobab", "Birch", "Cedar", "Cypress", "Elm", "Fir", "Maple", "Oak",
//     "Olive", "Palm", "Pine", "Poplar", "Redwood", "Sequoia", "Spruce", "Teak", "Walnut", "Willow",

//     // Insects (20) - Alphabetical
//     "Ant", "Aphid", "Bee", "Beetle", "Butterfly", "Cockroach", "Cricket", "Dragonfly", "Earwig", "Firefly",
//     "Flea", "Fly", "Gnat", "Grasshopper", "Ladybug", "Locust", "Mantis", "Mosquito", "Moth", "Termite",

//     // Fish (20) - Alphabetical
//     "Anchovy", "Bass", "Carp", "Catfish", "Cod", "Eel", "Goldfish", "Haddock", "Halibut", "Herring",
//     "Mackerel", "Perch", "Pike", "Salmon", "Sardine", "Swordfish", "Tilapia", "Trout", "Tuna", "Walleye",

//     // Planets (9) - Alphabetical
//     "Earth", "Jupiter", "Mars", "Mercury", "Neptune", "Pluto", "Saturn", "Uranus", "Venus",

//     // Periodic Table Elements (20) - Alphabetical
//     "Calcium", "Carbon", "Chlorine", "Copper", "Fluorine", "Gold", "Helium", "Hydrogen", "Iron", "Lithium",
//     "Magnesium", "Neon", "Nitrogen", "Oxygen", "Phosphorus", "Potassium", "Silicon", "Silver", "Sodium", "Sulfur"
// };

constexpr const std::array<std::string, 0> label_names = {};

constexpr const std::size_t label_names_max_size = (label_names.size() == 0) ? 4 : std::ranges::max_element(label_names, [](const std::string& l, const std::string& r){
                                                                                                return l.size() < r.size();
                                                                                            }
                                                                                        )->size();

template <typename Stream>
Stream& print_interval_set(Stream& stream, const prova::loga::tokenized_multi_alignment::interval_set& base_zones, const prova::loga::tokenized& base){
    std::size_t placeholder_count = 0;
    for(const auto& z: base_zones) {
        prova::loga::zone tag = *z.second.cbegin(); // set has only one item
        std::size_t offset = z.first.lower();
        std::size_t len = z.first.upper()-z.first.lower();
        std::string substr = base.subset(offset, len).view();
        if(tag == prova::loga::zone::constant)
            std::cout << substr;
        else {
            const auto& color = prova::loga::colors::palette.at(placeholder_count % prova::loga::colors::palette.size());
            std::cout << color << std::format("${}", placeholder_count) << prova::loga::colors::reset;
            ++placeholder_count;
        }
    }
    return stream;
}

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

    std::string log_name  = vm["input"].as<std::string>();
    std::string base_name = log_name+"_d";
    std::filesystem::path output_dir(base_name);
    {
        std::error_code ec;
        if (!std::filesystem::exists(output_dir) && !std::filesystem::create_directories(output_dir, ec)) {
            throw std::runtime_error( "Failed to create output directory '" + output_dir.string() + "': " + ec.message());
        }
    }

    std::filesystem::path archive_file_path        = output_dir / std::format("{}.1g.paths",           log_name);
    std::filesystem::path dist_file_path           = output_dir / std::format("{}.lev.dmat",           log_name);
    std::filesystem::path graphml_file_path        = output_dir / std::format("{}.1nn.graphml",        log_name);
    std::filesystem::path labels_file_path         = output_dir / std::format("{}.labels",             log_name);
    std::filesystem::path automata_dot_file_path   = output_dir / std::format("{}.automata.dot",       log_name);
    std::filesystem::path components_file_path     = output_dir / std::format("{}.components",         log_name);
    std::filesystem::path phase2_graphml_file_path = output_dir / std::format("{}.components.graphml", log_name);

    prova::loga::tokenized_collection collection;
    std::ifstream log(log_name);
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
    arma::mat dmat = distance.matrix();
    assert(dmat.n_rows == collection.count());
    assert(dmat.n_cols == collection.count());

    auto bgl_graph = distance.knn_graph(1, false);
    {
        std::ofstream graphml(graphml_file_path);
        prova::loga::print_graphml(bgl_graph, graphml);
    }
    arma::Row<std::size_t> labels(collection.count());
    if(std::filesystem::exists(labels_file_path)){
        std::ifstream labels_file(labels_file_path);
        if(!labels.load(labels_file)){
            std::cout << "failed to load labels" << std::endl;
            return 1;
        }
    } else {
        igraph_t igraph;
        igraph_vector_t edges;
        prova::loga::bgl_to_igraph(bgl_graph, &igraph, &edges);
        prova::loga::leiden_membership(&igraph, &edges, labels);
        std::ofstream labels_file(labels_file_path);
        if(!labels.save(labels_file)){
            std::cout << "failed to save labels" << std::endl;
            return 1;
        }
    }

    bool phase_2 = false;
    if(std::filesystem::exists(components_file_path)){
        arma::Row<std::size_t> components(collection.count());
        std::ifstream components_file(components_file_path);
        if(!components.load(components_file)){
            std::cout << "failed to load components" << std::endl;
            return 1;
        } else {
            phase_2 = true;
            labels = components;
            std::cout << "Loaded components" << std::endl;
        }
    }

    std::map<std::size_t, parsed> patterns;
    prova::loga::tokenized_group group(collection, labels);
    std::cout << "Clusters: " << group.labels() << std::endl;
    std::map<std::size_t, prova::loga::tokenized_multi_alignment::interval_set> cluster_patterns;
    std::map<std::size_t, prova::loga::tokenized> cluster_samples;
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
            lof = prova::loga::outliers::lof(subcollection);

            // std::cout << lof << std::endl;
            std::vector<std::size_t> excluded;
            for (arma::uword i = 0; i < count; ++i) {
                if(lof(i) >= 1.2f){
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
        subalignment.bubble_all_pairwise(paths, subcollection.begin(), 1, false);

        for(const auto& [key, path]: paths) {
            auto global_key = std::make_pair(references.at(key.first), references.at(key.second));
            if(!all_paths.contains(global_key)) {
                all_paths.insert(std::make_pair(global_key, path));
            }
        }

        prova::loga::tokenized_multi_alignment malign(subcollection, paths, base);
        prova::loga::tokenized_multi_alignment::region_map regions = malign.align();
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

        cluster_patterns.insert(std::make_pair(c, base_zones));
        cluster_samples.insert(std::make_pair(c, subcollection.at(0)));
    }

    {
        std::ofstream archive_file(archive_file_path);
        cereal::PortableBinaryOutputArchive archive(archive_file);
        archive(all_paths);
    }

    std::cout << "----------------------------" << std::endl;
    std::cout << "Summary: " << std::format("{} clusters", group.labels()) << std::endl;
    std::cout << "----------------------------" << std::endl;

    // std::ofstream phase1_res(phase1_res_file_path);
    std::size_t total_segments = 0;
    std::vector<prova::loga::pattern_sequence> pseqs;
    for(const auto& [c, p]: patterns) {
        std::string cluster_name = (c < std::numeric_limits<std::size_t>::max()) ? ((c < label_names.size()) ? label_names.at(c) : std::format("C{}", c)) : " Failed ";
        const auto& base_zones = p.intervals();
        std::size_t placeholder_count = 0;
        std::cout << std::setw(label_names_max_size) << cluster_name << " " << prova::loga::colors::bright_yellow << "●" << prova::loga::colors::reset << " " << std::resetiosflags(std::ios::showbase);
        prova::loga::pattern_sequence pseq;
        for(const auto& z: base_zones) {
            prova::loga::zone tag = *z.second.cbegin(); // set has only one item
            std::size_t offset = z.first.lower();
            std::size_t len = z.first.upper()-z.first.lower();
            std::string substr = collection.at(p.id()).subset(offset, len).view();

            prova::loga::pattern_sequence::segment seg(tag);
            if(tag == prova::loga::zone::constant){
                std::cout << substr;
                // phase1_res << substr;
                seg = substr;
                ++total_segments;
            } else {
                const auto& color = prova::loga::colors::palette.at(placeholder_count % prova::loga::colors::palette.size());
                std::cout << color << std::format("${}", placeholder_count) << prova::loga::colors::reset;
                ++placeholder_count;
                // seg = "$";
                // phase1_res << "$";
            }
            pseq.add(std::move(seg));
        }
        std::cout << std::endl;
        // phase1_res << std::endl;
        pseqs.emplace_back(std::move(pseq));
    }

    // prova::loga::automata::thompson_digraph_type graph;
    // std::vector<prova::loga::automata::vertex_type> starts, finishes;
    // for(std::size_t i = 0; i < pseqs.size(); ++i) {
    //     auto [start, finish] = prova::loga::automata::thompson_graph(graph, pseqs.at(i), i);
    //     starts.push_back(start);
    //     finishes.push_back(finish);
    // }
    // const pattern_sequence& pseq_ref  = pseqs.at(7);
    // auto v = prova::loga::automata::directional_partial_generialize(graph, pseq_ref, starts.at(1), 7);
    // auto vp = graph[v.last_v];

    prova::loga::automata automata(pseqs.cbegin(), pseqs.cend());
    automata.build();
    arma::Mat<std::size_t> coverage, capture;
    automata.generialize(coverage, capture);
    std::ofstream astream(automata_dot_file_path);
    automata.graphviz(astream);
    using knn_graph_type = knn_digraph<std::size_t>;
    knn_graph_type knn(pseqs.size());
    knn.update(coverage, capture, 1, false);
    {
        auto cluster_graph = knn.graph();
        std::ofstream graphml_knn(phase2_graphml_file_path);
        prova::loga::print_graphml(cluster_graph, graphml_knn);
    }
    std::vector<int> comp;
    knn_graph_type::digraph_type digraph = knn.graph();

    std::map<knn_graph_type::vertex_type, std::size_t> vertex_components_map;
    std::size_t num_components = prova::loga::detect_communities(digraph, prova::loga::community_detection_algorithm::leiden, vertex_components_map);

    // std::map<knn_graph_type::vertex_type, int> vertex_components_map;
    // std::size_t num_components = knn.weekly_connected_components(vertex_components_map);
    std::multimap<std::size_t, knn_graph_type::vertex_type> components_vertex_map;
    for(const auto& [v, c]: vertex_components_map) {
        components_vertex_map.insert(std::make_pair(c, v));
    }

    std::cout << "----------------------------" << std::endl;
    std::cout << "Summary: " << std::format("{} components", num_components) << std::endl;
    std::cout << "----------------------------" << std::endl;
    // print_pattern(std::cout, pseqs.at(1));
    // std::cout << std::endl;
    // print_pattern(std::cout, pseqs.at(9));
    // std::cout << std::endl;
    // a.directional_partial_generialize(a._graph, pseqs.at(9), a._terminals.at(1).first, 9);

    // auto test_1 = a.merge(1);
    // print_pattern(std::cout, test_1);
    // std::cout << std::endl;

    for (auto it = components_vertex_map.cbegin(); it != components_vertex_map.cend(); ) {
        int component_id = it->first;
        std::cout << prova::loga::colors::bright_yellow << "◈" << prova::loga::colors::reset << " T" << component_id << " ";

        auto range = components_vertex_map.equal_range(component_id);
        std::size_t count = std::distance(range.first, range.second);

        std::cout << std::endl;

        if (count == 1) {
            // single-vertex component
            std::size_t vi = range.first->second;
            auto v = boost::vertex(vi, digraph);
            std::size_t cluster = digraph[v].id;
            const auto& pat    = cluster_patterns.at(cluster);
            const auto& sample = cluster_samples.at(cluster);
            // std::cout << "    ";
            print_interval_set(std::cout, pat, sample);
            std::cout << std::endl;
            it = range.second;
            continue;
        }

        // collect vertex indices of this component
        std::vector<knn_graph_type::vertex_type> verts;
        verts.reserve(count);
        for (auto jt = range.first; jt != range.second; ++jt) {
            verts.push_back(jt->second);                                         // vertex index
        }

        // sort by in-degree (for undirected graphs, in_degree == degree)
        std::sort(verts.begin(), verts.end(), [&](knn_graph_type::vertex_type u, knn_graph_type::vertex_type v) {
                                                      return boost::in_degree(u, digraph) > boost::in_degree(v, digraph);
                                                  });
        std::set<std::size_t> ref_members;
        for (std::size_t vi : verts) {
            auto v = boost::vertex(vi, digraph);
            std::size_t cluster = digraph[v].id;                // recover cluster id
            ref_members.insert(cluster);
        }

        // now iterate in sorted order
        std::size_t first_i = verts.at(0);
        auto first_v = boost::vertex(first_i, digraph);
        std::size_t first_c = digraph[first_v].id;
        prova::loga::pattern_sequence merged = automata.merge(first_c, ref_members);
        // std::cout << "    " << std::setw(4) << ": ";
        print_pattern(std::cout, merged);
        std::cout << std::endl;
        std::cout << "-----------------------------";

        {
            std::set<std::size_t> members = ref_members;
            members.insert(first_c);
            std::string component_subgraph = output_dir / std::format("{}.component.{}.automata.dot", log_name, first_c);
            std::ofstream component_subgraph_stream(component_subgraph);
            prova::loga::automata::thompson_digraph_type subgraph;
            automata.subgraph(ref_members, subgraph);
            prova::loga::automata::graphviz(component_subgraph_stream, subgraph);
        }

        std::cout << std::endl;
        for (std::size_t vi : verts) {
            auto v = boost::vertex(vi, digraph);
            std::size_t cluster = digraph[v].id;                // recover cluster id
            const prova::loga::pattern_sequence& pat = pseqs.at(cluster);
            std::cout << "    " << std::setw(4) << cluster << ": ";
            print_pattern(std::cout, pat);
            std::cout << std::endl;
        }

        it = range.second;
    }

    prova::loga::cluster::labels_type components(collection.count());
    for (auto it = components_vertex_map.cbegin(); it != components_vertex_map.cend(); ) {
        int component_id = it->first;
        auto range = components_vertex_map.equal_range(component_id);
        for (auto jt = range.first; jt != range.second; ++jt) {
            std::size_t cluster = jt->second;
            prova::loga::tokenized_group::label_proxy proxy = group.proxy(cluster);
            std::size_t count = proxy.count();                     // global ids of all items belonging to the same cluster
            for(std::size_t i = 0; i < count; ++i) {
                prova::loga::tokenized_group::label_proxy::value v = proxy.at(i);   // v: {str, id} where the id is the global id and the str is the string (not token list) value of the item
                std::size_t global_id = v.id();
                components[global_id] = component_id;
            }
        }
        it = range.second;
    }
    std::ofstream components_file(components_file_path);
    if(!components.save(components_file)){
        std::cout << "failed to save components" << std::endl;
        return 1;
    }

    std::cout << "Plase 1 completed run loga again for phase 2" << std::endl;

    if(phase_2) {
        return 0;
    }

    // arma::imat cluster_distances;
    // cluster_distances.set_size(pseqs.size(), pseqs.size());
    // auto cluster_graph = constant_component_graph::apply(pseqs, cluster_distances, 10);
    // std::ofstream stream(phase2_graphml_file_path);
    // constant_component_graph::graphml(stream, cluster_graph);
    // std::multimap<int, std::size_t> components_map;
    // size_t num_components = constant_component_graph::cluster(cluster_graph, components_map);
    // prova::loga::cluster::labels_type components(collection.count());
    // for (auto it = components_map.cbegin(); it != components_map.cend(); ) {
    //     int component_id = it->first;
    //     auto range = components_map.equal_range(component_id);
    //     for (auto jt = range.first; jt != range.second; ++jt) {
    //         std::size_t cluster = jt->second;
    //         prova::loga::tokenized_group::label_proxy proxy = group.proxy(cluster);
    //         std::size_t count = proxy.count();                     // global ids of all items belonging to the same cluster
    //         for(std::size_t i = 0; i < count; ++i) {
    //             prova::loga::tokenized_group::label_proxy::value v = proxy.at(i);   // v: {str, id} where the id is the global id and the str is the string (not token list) value of the item
    //             std::size_t global_id = v.id();
    //             components[global_id] = component_id;
    //         }
    //     }
    //     it = range.second;
    // }
    // std::ofstream components_file(components_file_path);
    // if(!components.save(components_file)){
    //     std::cout << "failed to save components" << std::endl;
    //     return 1;
    // }

    // std::cout << "Number of connected components: " << num_components << std::endl;
    // for (auto it = components_map.cbegin(); it != components_map.cend(); ) {
    //     int component_id = it->first;
    //     std::cout << "Component " << component_id << ": " << std::endl;

    //     auto range = components_map.equal_range(component_id);
    //     std::size_t count = std::distance(range.first, range.second);
    //     if(count == 1) {
    //         std::size_t cluster = range.first->second;
    //         const prova::loga::tokenized_multi_alignment::interval_set& pat = cluster_patterns.at(cluster);
    //         const prova::loga::tokenized& sample = cluster_samples.at(cluster);
    //         print_interval_set(std::cout, pat, sample);
    //         std::cout << std::endl;
    //         it = range.second;

    //         continue;
    //     }

    //     std::vector<pattern_sequence> cluster_pseqs;
    //     for (auto jt = range.first; jt != range.second; ++jt) {
    //         std::size_t cluster = jt->second;

    //         const pattern_sequence& pat = pseqs.at(cluster);
    //         cluster_pseqs.push_back(pat);
    //         print_pattern(std::cout, pat);
    //         std::cout << std::endl;
    //     }


    //     it = range.second;
    // }

    // std::cout << "Plase 1 completed run loga again for phase 2" << std::endl;

    return 0;
}
