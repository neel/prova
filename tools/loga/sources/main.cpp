#include <loga/collection.h>
#include <loga/alignment.h>
#include <loga/distance.h>
#include <loga/cluster.h>
#include <loga/group.h>
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
//     << "[Sun Dec 04 04:51:08 2005] [notice] jk2_init() Found child 6725 in scoreboard slot 10"
//     << "[Sun Dec 04 04:51:09 2005] [notice] jk2_init() Found child 6726 in scoreboard slot 8"
//     << "[Sun Dec 04 04:51:09 2005] [notice] jk2_init() Found child 6728 in scoreboard slot 6"
//     << "[Sun Dec 04 04:51:37 2005] [notice] jk2_init() Found child 6736 in scoreboard slot 10"
//     << "[Sun Dec 04 04:51:38 2005] [notice] jk2_init() Found child 6733 in scoreboard slot 7"
//     << "[Sun Dec 04 04:51:38 2005] [notice] jk2_init() Found child 6734 in scoreboard slot 9"
//     << "[Sun Dec 04 04:52:04 2005] [notice] jk2_init() Found child 6738 in scoreboard slot 6"
//     << "[Sun Dec 04 04:52:04 2005] [notice] jk2_init() Found child 6741 in scoreboard slot 9"
//     << "[Sun Dec 04 04:52:05 2005] [notice] jk2_init() Found child 6740 in scoreboard slot 7"
//     << "[Sun Dec 04 04:52:05 2005] [notice] jk2_init() Found child 6737 in scoreboard slot 8"
//     << "[Sun Dec 04 04:52:36 2005] [notice] jk2_init() Found child 6748 in scoreboard slot 6"
//     << "[Sun Dec 04 04:52:36 2005] [notice] jk2_init() Found child 6744 in scoreboard slot 10"
//     << "[Sun Dec 04 04:52:36 2005] [notice] jk2_init() Found child 6745 in scoreboard slot 8"
//     << "[Sun Dec 04 04:53:05 2005] [notice] jk2_init() Found child 6750 in scoreboard slot 7"
//     << "[Sun Dec 04 04:53:05 2005] [notice] jk2_init() Found child 6751 in scoreboard slot 9"
//     << "[Sun Dec 04 04:53:05 2005] [notice] jk2_init() Found child 6752 in scoreboard slot 10"
//     << "[Sun Dec 04 04:53:29 2005] [notice] jk2_init() Found child 6754 in scoreboard slot 8"
//     << "[Sun Dec 04 04:53:29 2005] [notice] jk2_init() Found child 6755 in scoreboard slot 6"
//     << "[Sun Dec 04 04:53:40 2005] [notice] jk2_init() Found child 6756 in scoreboard slot 7"
//     << "[Sun Dec 04 04:54:15 2005] [notice] jk2_init() Found child 6763 in scoreboard slot 10"
//     << "[Sun Dec 04 04:54:15 2005] [notice] jk2_init() Found child 6766 in scoreboard slot 6"
//     << "[Sun Dec 04 04:54:15 2005] [notice] jk2_init() Found child 6767 in scoreboard slot 7"
//     << "[Sun Dec 04 04:54:15 2005] [notice] jk2_init() Found child 6765 in scoreboard slot 8"
//     << "[Sun Dec 04 04:54:20 2005] [notice] jk2_init() Found child 6768 in scoreboard slot 9"
//     << "[Sun Dec 04 04:56:52 2005] [notice] jk2_init() Found child 8527 in scoreboard slot 10"
//     << "[Sun Dec 04 04:56:52 2005] [notice] jk2_init() Found child 8533 in scoreboard slot 8"
//     << "[Sun Dec 04 04:57:20 2005] [notice] jk2_init() Found child 8536 in scoreboard slot 6"
//     << "[Sun Dec 04 04:57:20 2005] [notice] jk2_init() Found child 8539 in scoreboard slot 7"
//     << "[Sun Dec 04 04:57:49 2005] [notice] jk2_init() Found child 8541 in scoreboard slot 9"
//     << "[Sun Dec 04 04:58:45 2005] [notice] jk2_init() Found child 8547 in scoreboard slot 10"
//     << "[Sun Dec 04 04:59:28 2005] [notice] jk2_init() Found child 8554 in scoreboard slot 6"
//     << "[Sun Dec 04 04:59:27 2005] [notice] jk2_init() Found child 8553 in scoreboard slot 8"
//     << "[Sun Dec 04 05:00:03 2005] [notice] jk2_init() Found child 8560 in scoreboard slot 7"
//     << "[Sun Dec 04 05:00:13 2005] [notice] jk2_init() Found child 8565 in scoreboard slot 9"
//     << "[Sun Dec 04 05:00:13 2005] [notice] jk2_init() Found child 8573 in scoreboard slot 10"
//     << "[Sun Dec 04 05:01:20 2005] [notice] jk2_init() Found child 8584 in scoreboard slot 7"
//     << "[Sun Dec 04 05:01:20 2005] [notice] jk2_init() Found child 8587 in scoreboard slot 9"
//     << "[Sun Dec 04 05:02:14 2005] [notice] jk2_init() Found child 8603 in scoreboard slot 10"
//     << "[Sun Dec 04 05:02:14 2005] [notice] jk2_init() Found child 8605 in scoreboard slot 8"
//     << "[Sun Dec 04 05:04:03 2005] [notice] jk2_init() Found child 8764 in scoreboard slot 10"
//     << "[Sun Dec 04 05:04:03 2005] [notice] jk2_init() Found child 8765 in scoreboard slot 11"
//     ;

//     prova::loga::alignment alignment(collection);
//     prova::loga::alignment::matrix_type paths;
//     alignment.bubble_all_pairwise(paths, 2);

//     prova::loga::multi_alignment malign(collection, paths, 0);
//     prova::loga::multi_alignment::region_map regions = malign.align();

//     malign.print_regions_string(regions, std::cout) << std::endl;
//     std::cout << "-----" << std::endl;
//     regions = malign.fixture_word_boundary(regions);
//     malign.print_regions_string(regions, std::cout) << std::endl;

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
    try{
        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "Print help message")
            ("input", boost::program_options::value<std::string>(), "Input path");

        boost::program_options::positional_options_description p;
        p.add("input", 1);

        boost::program_options::variables_map vm;
        auto options = boost::program_options::command_line_parser(argc, argv).options(desc).positional(p).run();
        boost::program_options::store(options, vm);
        boost::program_options::notify(vm);

        if (vm.count("help") || !vm.count("input")) {
            std::cout << "Usage: " << argv[0] << " <path>\n";
            std::cout << desc << "\n";
            return 0;
        }

        input_path = vm["input"].as<std::string>();
        std::cout << "Input path: " << input_path << "\n";
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
    cluster.compute(0.1, 3);
    prova::loga::cluster::labels_type labels = cluster.labels();
    // std::cout << "labels " << labels.size() << std::endl;

    std::map<std::size_t, parsed> parsed_log;
    prova::loga::group group(collection, labels);
    for(std::uint32_t c = 0; c != group.labels(); ++c) {
        std::cout << "Group: " << c << std::endl;
        prova::loga::group::label_proxy proxy = group.proxy(c);
        std::size_t count = proxy.count();
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

        std::cout << "base: " << collection.at(min_matched_id) << " " << min_matched << std::endl;

        prova::loga::multi_alignment malign(collection, paths, proxy.mask(), min_matched_id);
        prova::loga::multi_alignment::region_map regions = malign.align();
        regions = malign.fixture_word_boundary(regions);
        // malign.print_regions_string(regions, std::cout) << std::endl;
        for(auto& [id, zones]: regions) {
            parsed p(id, c, std::move(zones));
            parsed_log.emplace(std::make_pair(id, p));
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
        return l > r;
    });

    std::cout << "largest_cluster_name: " << largest_cluster_name << " " << num_clusters << std::endl;

    for(const auto& [id, p]: parsed_log) {
        std::string cluster_name = (p.cluster() < fruits.size()) ? fruits.at(p.cluster()) : std::format("C{}", p.cluster());
        std::cout << std::right << std::setw(4) << id << " -> " << std::left << std::setw(largest_cluster_name.size()+1) << cluster_name << " | " << std::resetiosflags(std::ios::showbase);
        prova::loga::multi_alignment::print_interval_set(p.intervals(), collection.at(id), std::cout);
        std::cout << std::endl;
    }
}
