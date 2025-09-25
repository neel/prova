#include <loga/collection.h>
#include <loga/alignment.h>
#include <loga/distance.h>
#include <loga/cluster.h>
#include <loga/group.h>
#include <loga/graph.h>
#include <loga/multi_alignment.h>
#include <iostream>
#include <filesystem>
#include <boost/program_options.hpp>
#include <boost/icl/interval.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/separate_interval_set.hpp>

// int main() {
//     prova::loga::collection collection;
//     // collection.add("Hello W Here I am do you hear me 579");
//     // collection.add("J Here Jx am do you hear me 18303");
//     // collection.add("Hola W Here We are do you hear me");

//     collection
//     << "- 1131566479 2005.11.09 #8# Nov 9 12:01:19 #8#/#8# sshd[19023]: Local disconnected: Connection closed."
//     << "- 1131566479 2005.11.09 #8# Nov 9 12:01:19 #8#/#8# sshd[19023]: connection lost: 'Connection closed.'"
//     ;

//     prova::loga::alignment alignment(collection);
//     auto graph = alignment.bubble_all();
//     graph.build();
//     auto path = graph.shortest_path();
//     path.print(std::cout) << std::endl;

//     return 0;

//     prova::loga::alignment::matrix_type paths;
//     alignment.bubble_all_pairwise(paths, 2);

//     prova::loga::multi_alignment malign(collection, paths, 0);
//     prova::loga::multi_alignment::region_map regions = malign.align();

//     malign.print_regions_string(regions, std::cout) << std::endl;
//     malign.print_regions(regions, std::cout) << std::endl;

//     // prova::loga::multi_alignment::region_map cpc_adjusted_regions;
//     // for(auto& candidate: regions) {
//     //     std::vector<std::pair<prova::loga::multi_alignment::region_type, prova::loga::zone>> updated_regions;
//     //     std::size_t candidate_id = candidate.first;
//     //     auto& intervals = candidate.second;
//     //     prova::loga::zone previous_tag;
//     //     std::size_t first = true;
//     //     for(auto& z: intervals) {
//     //         auto& region = z.first;
//     //         prova::loga::zone tag = *z.second.cbegin();
//     //         if(!first && tag == prova::loga::zone::constant && previous_tag == prova::loga::zone::constant){
//     //             auto region = updated_regions.back().first;
//     //             region = prova::loga::multi_alignment::region_type::left_open(region.upper(), region.upper());
//     //             updated_regions.push_back(std::make_pair(region, prova::loga::zone::placeholder));
//     //         }
//     //         updated_regions.push_back(std::make_pair(region, tag));
//     //         previous_tag = tag;
//     //         first = false;
//     //     }
//     //     prova::loga::multi_alignment::interval_set updated_intervals;
//     //     for(const auto& pair: updated_regions) {
//     //         std::set<prova::loga::zone> zones;
//     //         zones.insert(pair.second);
//     //         updated_intervals.add(std::make_pair(pair.first, zones));
//     //     }
//     //     cpc_adjusted_regions.insert(std::make_pair(candidate_id, updated_intervals));
//     // }
//     // regions = cpc_adjusted_regions;
//     // std::cout << "-- cpc --" << std::endl;
//     // malign.print_regions_string(regions, std::cout) << std::endl;
//     // malign.print_regions(regions, std::cout) << std::endl;
//     std::cout << "-- word boundary --" << std::endl;
//     regions = malign.fixture_word_boundary(regions);
//     malign.print_regions_string(regions, std::cout) << std::endl;
//     malign.print_regions(regions, std::cout) << std::endl;

//     return 0;
// }

class parsed{
    std::size_t _id;
    std::size_t _cluster;
    prova::loga::multi_alignment::interval_set _intervals;

public:
    inline explicit parsed(std::size_t id, std::size_t cluster, prova::loga::multi_alignment::interval_set&& intervals): _id(id), _cluster(cluster), _intervals(std::move(intervals)) {}
    const prova::loga::multi_alignment::interval_set& intervals() const { return _intervals; }
    std::size_t id() const { return _id; }
    std::size_t cluster() const { return _cluster; }
};

int main(int argc, char** argv) {
    std::filesystem::path input_path;
    boost::program_options::variables_map vm;
    try{
        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
            ("help,h",         "Print help message")
            ("input",          boost::program_options::value<std::string>(), "Input path")
            ("fixtures,f",     boost::program_options::value<bool>()->default_value(true)->implicit_value(true), "Apply all fixtures (default: on)")
            ("consolidated,c", boost::program_options::value<bool>()->default_value(true)->implicit_value(true), "Enable consolidated view (default: on)")
            ("clusterwise,k",  boost::program_options::value<bool>()->default_value(false)->implicit_value(false), "Enable clusterwise view (default: off)");

        boost::program_options::positional_options_description p;
        p.add("input", 1);

        auto options = boost::program_options::command_line_parser(argc, argv).options(desc).positional(p).run();
        boost::program_options::store(options, vm);
        boost::program_options::notify(vm);

        if (vm.count("help") || !vm.count("input")) {
            std::cout << "Usage: " << argv[0] << " <path>\n";
            std::cout << desc << "\n";
            return 0;
        }

        input_path = vm["input"].as<std::string>();
        std::cout << "Log file: " << input_path << "\n";
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    prova::loga::collection collection;
    std::ifstream log(input_path);
    collection.parse(log);

    prova::loga::alignment alignment(collection);

    prova::loga::alignment::matrix_type paths;
    alignment.bubble_all_pairwise(paths, 2);

    prova::loga::distance distance(paths, collection.count());
    distance.compute();
    prova::loga::distance::distance_matrix_type dmat = distance.matrix();
    // std::cout << "dmat " << dmat.size() << std::endl;

    prova::loga::cluster cluster(dmat);
    cluster.compute(0.1, 2);
    prova::loga::cluster::labels_type labels = cluster.labels();
    // std::cout << "labels " << labels.size() << std::endl;

    std::map<std::size_t, parsed> parsed_log;
    prova::loga::group group(collection, labels);
    for(std::uint32_t c = 0; c != group.labels(); ++c) {
        prova::loga::group::label_proxy proxy = group.proxy(c);
        std::size_t count = proxy.count();
        std::cout << "Group: " << c << std::format(" ({})", count) << std::endl;
        std::size_t min_matched = std::numeric_limits<std::size_t>::max();
        std::size_t min_matched_id = std::numeric_limits<std::size_t>::max();
        for(std::size_t i = 0; i != count; ++i) {
            auto v = proxy.at(i);
            // std::cout << v.str() << std::endl;

            for(const auto& [k, path]: paths) {
                if(k.first == v.id()) {
                    std::size_t matched_chars = path.matched();
                    if(matched_chars < min_matched) {
                        min_matched = matched_chars;
                        min_matched_id = v.id();
                    }
                }
            }
        }

        if(min_matched_id == std::numeric_limits<std::size_t>::max()){
            std::cout << "Unexpected " << __LINE__ << std::endl;
            continue;
        }

        std::cout << "base: " << collection.at(min_matched_id) << " " << min_matched_id << std::endl;

        prova::loga::multi_alignment malign(collection, paths, proxy.mask(), min_matched_id);
        prova::loga::multi_alignment::region_map regions = malign.align();
        if(vm["fixtures"].as<bool>()) {
            regions = malign.fixture_word_boundary(regions);
        }
        if(vm["clusterwise"].as<bool>()) {
            malign.print_regions_string(regions, std::cout) << std::endl;
        }

        for(auto& [id, zones]: regions) {
            parsed p(id, c, std::move(zones));
            parsed_log.emplace(std::make_pair(id, p));
        }
    }
    for(std::size_t i = 0; i != collection.count(); ++i) {
        if(!parsed_log.contains(i)){
            prova::loga::multi_alignment::interval_set intervals;
            std::set<prova::loga::zone> zones;
            zones.insert(prova::loga::zone::constant);
            auto region = prova::loga::multi_alignment::region_type::right_open(0, collection.at(i).size());
            intervals.add(std::make_pair(region, zones));
            parsed p(i, -1, std::move(intervals));
            parsed_log.emplace(std::make_pair(i, p));
        }

    }

    const std::array<std::string, 20> fruits = {
        "Apple", "Banana", "Orange", "Mango", "Grape", "Pineapple", "Strawberry", "Cherry", "Peach", "Pear",
        "Watermelon", "Papaya", "Coconut", "Lemon", "Lime", "Kiwi", "Plum", "Apricot", "Pomegranate", "Guava"
    };

    std::size_t num_clusters = group.labels();
    auto begin = fruits.cbegin();
    auto end = begin;
    std::advance(end, num_clusters);
    std::string largest_cluster_name = *std::max_element(begin, end, [](const std::string& l, const std::string& r){
        return l < r;
    });

    std::cout << "largest_cluster_name: " << largest_cluster_name << " " << num_clusters << std::endl;

    if(vm["consolidated"].as<bool>()) {
        for(const auto& [id, p]: parsed_log) {
            std::string cluster_name = p.cluster() < std::numeric_limits<std::size_t>::max() ? ((p.cluster() < fruits.size()) ? fruits.at(p.cluster()) : std::format("C{}", p.cluster())) : " ";
            std::cout << std::right << std::setw(4) << id << " -> " << std::left << std::setw(largest_cluster_name.size()) << cluster_name << " | " << std::resetiosflags(std::ios::showbase);
            prova::loga::multi_alignment::print_interval_set(p.intervals(), collection.at(id), std::cout);
            std::cout << std::endl;
        }
    }
}
