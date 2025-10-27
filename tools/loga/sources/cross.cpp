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
    std::string graphml_file_path = std::format("{}.1nn.graphml", log_path);

    std::ifstream log(log_path);
    collection.parse(log);


    prova::loga::tokenized_alignment alignment(collection);
    prova::loga::tokenized_alignment::matrix_type all_paths;

    // alignment.bubble_all_pairwise(all_paths, 1);
    if(std::filesystem::exists(archive_file_path)){
        std::ifstream archive_file(archive_file_path);
        cereal::PortableBinaryInputArchive archive(archive_file);
        archive(all_paths);
    } else {
        alignment.bubble_all_pairwise(all_paths, 1);

        std::ofstream archive_file(archive_file_path);
        cereal::PortableBinaryOutputArchive archive(archive_file);
        archive(all_paths);
    }

    struct vertex_props{
        enum class type{
            none,
            segment,
            content
        };

        type _type                = type::none;
        std::size_t _id           = 0;
        prova::loga::index _index = prova::loga::index({0, 0});
        std::size_t _length       = 0;

        inline vertex_props(): _type(type::none) {};
        inline explicit vertex_props(type t): _type(t) {}
    };

    struct edge_props{
        enum class type{
            none,
            belongs,
            ordered,
            sym
        };

        type _type           = type::none;
        std::size_t _counter = 1;

        inline edge_props(): _type(type::none) {};
        inline explicit edge_props(type t): _type(t) {}
    };

    using graph_type  = boost::adjacency_list<boost::setS, boost::vecS, boost::directedS, vertex_props, edge_props>;
    using vertex_type = boost::graph_traits<graph_type>::vertex_descriptor;
    using edge_type   = boost::graph_traits<graph_type>::edge_descriptor;

    graph_type graph;

    vertex_type S = boost::add_vertex(graph);
    vertex_type T = boost::add_vertex(graph);
    graph[S]._type = vertex_props::type::segment;
    graph[T]._type = vertex_props::type::segment;
    graph[S]._length = 0;
    graph[T]._length = 0;

    std::map<std::size_t, vertex_type> content_vertices;
    std::map<std::string, vertex_type> segment_vertices;
    std::map<vertex_type, std::string> vertex_strings;

    vertex_strings[S] = "@S";
    vertex_strings[T] = "@T";

    segment_vertices["@S"] = S;
    segment_vertices["@T"] = T;

    // for(std::size_t i = 0; i < collection.count(); ++i) {
    //     vertex_type v = boost::add_vertex(graph);
    //     graph[v]._type = vertex_props::type::content;
    //     graph[v]._id   = i;
    //     content_vertices.insert(std::make_pair(i, v));
    //     // vertex_strings.insert(std::make_pair(v, collection.at(i).raw()));
    //     vertex_strings.insert(std::make_pair(v, std::to_string(i)));
    // }

    for(const auto& [key, path]: all_paths) {
        std::vector<vertex_type> siblings;
        for(const prova::loga::segment& segment: path) {
            std::string segstr = segment.view(collection);
            if(!segment_vertices.contains(segstr)) {
                vertex_type v = boost::add_vertex(graph);
                segment_vertices.insert(std::make_pair(segstr, v));
                graph[v]._type = vertex_props::type::segment;
                graph[v]._index = segment.start();
                graph[v]._length = segment.length();

                vertex_strings.insert(std::make_pair(v, segment.view(collection)));
            }
            siblings.push_back(segment_vertices.at(segstr));
            // auto res_e_l = boost::add_edge(content_vertices.at(key.first), segment_vertices.at(segstr), graph);
            // if(res_e_l.second) {
            //     graph[res_e_l.first]._type = edge_props::type::belongs;
            // }

            // auto res_e_r = boost::add_edge(content_vertices.at(key.second), segment_vertices.at(segstr), graph);
            // if(res_e_r.second) {
            //     graph[res_e_r.first]._type = edge_props::type::belongs;
            // }
        }

        if(siblings.size() > 1) {
            for(auto it = siblings.begin(); it != siblings.end(); ++it) {
                if(it == siblings.begin()) {
                    auto pair = boost::add_edge(S, *it, graph);
                    if(pair.second) {
                        edge_type e = pair.first;
                        graph[e]._type = edge_props::type::ordered;
                    } else {
                        edge_type e = pair.first;
                        graph[e]._counter++;
                    }
                    continue;
                }

                auto prev = it;
                prev = std::prev(prev);

                auto pair = boost::add_edge(*prev, *it, graph);
                if(pair.second) {
                    edge_type e = pair.first;
                    graph[e]._type = edge_props::type::ordered;
                } else {
                    edge_type e = pair.first;
                    graph[e]._counter++;
                }
            }

            auto pair = boost::add_edge(siblings.back(), T, graph);
            if(pair.second) {
                edge_type e = pair.first;
                graph[e]._type = edge_props::type::ordered;
            } else {
                edge_type e = pair.first;
                graph[e]._counter++;
            }
        }
    }

    auto vertex_type_to_str = [](vertex_props::type t) -> std::string_view {
        switch (t) {
            case vertex_props::type::segment: return "segment";
            case vertex_props::type::content: return "content";
            case vertex_props::type::none:
            default:                          return "none";
        }
    };

    auto edge_type_to_str = [](edge_props::type t) -> std::string_view {
        switch (t) {
            case edge_props::type::belongs: return "belongs";
            case edge_props::type::ordered: return "ordered";
            case edge_props::type::sym:     return "sym";
            case edge_props::type::none:
            default:                        return "none";
        }
    };

    // Vertex property maps
    auto vertex_type_map = boost::make_function_property_map<vertex_type, std::string_view>(
        [&](const vertex_type& v) -> std::string_view {
            return vertex_type_to_str(graph[v]._type);
        }
    );

    auto vertex_id_map = boost::make_function_property_map<vertex_type, std::size_t>(
        [&](const vertex_type& v) -> std::size_t {
            return graph[v]._id;
        }
    );

    auto vertex_len_map = boost::make_function_property_map<vertex_type, std::size_t>(
        [&](const vertex_type& v) -> std::size_t {
            return graph[v]._length;
        }
    );

    auto vertex_label_map = boost::make_function_property_map<vertex_type, std::string>(
        [&](const vertex_type& v) -> std::string {
            return vertex_strings.at(v);
        }
    );

    // Edge property maps
    auto edge_type_map = boost::make_function_property_map<edge_type, std::string_view>(
        [&](const edge_type& e) -> std::string_view {
            return edge_type_to_str(graph[e]._type);
        }
    );
    auto edge_count_map = boost::make_function_property_map<edge_type, std::string>(
        [&](const edge_type& e) -> std::string {
            return std::to_string(graph[e]._counter);
        }
    );

    // Bind to dynamic properties (use distinct names to avoid collisions)
    boost::dynamic_properties properties;
    properties.property("type",   vertex_type_map);   // vertex string
    properties.property("id",     vertex_id_map);     // vertex numeric
    properties.property("length", vertex_len_map);    // vertex numeric
    properties.property("type",   edge_type_map);     // edge string
    properties.property("weight", edge_count_map);   // edge string
    properties.property("label", vertex_label_map);   // edge string

    // Write GraphML
    std::ofstream graphml(graphml_file_path);
    boost::write_graphml(graphml, graph, properties, /*write_property_keys=*/true);

}
