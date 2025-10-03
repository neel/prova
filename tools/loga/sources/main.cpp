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
#include <cereal/archives/portable_binary.hpp>

#if 1

int main() {
    prova::loga::collection collection;
    // collection.add("Hello W Here I am do you hear me 579");
    // collection.add("J Here Jx am do you hear me 18303");
    // collection.add("Hola W Here We are do you hear me");

    collection
    << "- 1131566463 2005.11.09 cn142 Nov 9 12:01:03 cn142/cn142 ntpd[7467]: synchronized to 10.100.20.250, stratum 3"
    << "- 1131566467 2005.11.09 cn46 Nov 9 12:01:07 cn46/cn46 ntpd[15291]: synchronized to 10.100.16.250, stratum 3"
    << "- 1131566470 2005.11.09 cn661 Nov 9 12:01:10 cn661/cn661 ntpd[18505]: synchronized to 10.100.20.250, stratum 3"
    << "- 1131566473 2005.11.09 cn379 Nov 9 12:01:13 cn379/cn379 ntpd[10573]: synchronized to 10.100.18.250, stratum 3"
    << "- 1131566473 2005.11.09 cn733 Nov 9 12:01:13 cn733/cn733 ntpd[27716]: synchronized to 10.100.20.250, stratum 3"
    << "- 1131566477 2005.11.09 cn543 Nov 9 12:01:17 cn543/cn543 ntpd[13785]: synchronized to 10.100.18.250, stratum 3"
    << "- 1131566479 2005.11.09 bn431 Nov 9 12:01:19 bn431/bn431 ntpd[28723]: synchronized to 10.100.20.250, stratum 3"
    << "- 1131566484 2005.11.09 cn931 Nov 9 12:01:24 cn931/cn931 ntpd[29054]: synchronized to 10.100.20.250, stratum 3"
    ;

    prova::loga::alignment alignment(collection);
    std::size_t lmin = 1;
    prova::loga::alignment::matrix_type paths;
    alignment.bubble_all_pairwise(paths, lmin);

    prova::loga::multi_alignment malign(collection, paths, 0);
    prova::loga::multi_alignment::region_map regions = malign.align();

    malign.print_regions_string(regions, std::cout) << std::endl;
    malign.print_regions(regions, std::cout) << std::endl;

    return 0;
    // prova::loga::multi_alignment::region_map cpc_adjusted_regions;
    // for(auto& candidate: regions) {
    //     std::vector<std::pair<prova::loga::multi_alignment::region_type, prova::loga::zone>> updated_regions;
    //     std::size_t candidate_id = candidate.first;
    //     auto& intervals = candidate.second;
    //     prova::loga::zone previous_tag;
    //     std::size_t first = true;
    //     for(auto& z: intervals) {
    //         auto& region = z.first;
    //         prova::loga::zone tag = *z.second.cbegin();
    //         if(!first && tag == prova::loga::zone::constant && previous_tag == prova::loga::zone::constant){
    //             auto region = updated_regions.back().first;
    //             region = prova::loga::multi_alignment::region_type::left_open(region.upper(), region.upper());
    //             updated_regions.push_back(std::make_pair(region, prova::loga::zone::placeholder));
    //         }
    //         updated_regions.push_back(std::make_pair(region, tag));
    //         previous_tag = tag;
    //         first = false;
    //     }
    //     prova::loga::multi_alignment::interval_set updated_intervals;
    //     for(const auto& pair: updated_regions) {
    //         std::set<prova::loga::zone> zones;
    //         zones.insert(pair.second);
    //         updated_intervals.add(std::make_pair(pair.first, zones));
    //     }
    //     cpc_adjusted_regions.insert(std::make_pair(candidate_id, updated_intervals));
    // }
    // regions = cpc_adjusted_regions;
    // std::cout << "-- cpc --" << std::endl;
    // malign.print_regions_string(regions, std::cout) << std::endl;
    // malign.print_regions(regions, std::cout) << std::endl;
    std::cout << "-- word boundary --" << std::endl;
    regions = malign.fixture_word_boundary(regions);
    malign.print_regions_string(regions, std::cout) << std::endl;
    malign.print_regions(regions, std::cout) << std::endl;

    return 0;
}

#else

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

struct cli_params{
    std::filesystem::path input;
    bool fixtures       = true;
    bool consolidated   = true;
    bool clusterwise    = false;
};

static bool ensure_no_conflict(const boost::program_options::variables_map& vm, const char* positive, const char* negative) {
    // 1. if positive is defaulted then negative overrides -> No conflict
    // 2. if negative is defaulted then positive overrides -> No conflict
    // 3. if both are defaulted then they never conflict (assumption) -> No conflict
    // 4. if none is defaulted then their value should be opposite (pos xor neg = true) -> No conflict
    // 5. otherwise conflict

    const bool pos_defaulted = vm[positive].defaulted();
    const bool neg_defaulted = vm[negative].defaulted();

    if(pos_defaulted && neg_defaulted) {
        assert((vm[positive].as<bool>() ^ vm[negative].as<bool>()) == true);
        return vm[positive].as<bool>();
    }

    if(pos_defaulted || neg_defaulted) {
        if(pos_defaulted) return !vm[negative].as<bool>();
        if(neg_defaulted) return  vm[positive].as<bool>();
    }

    // !pos_defaulted && !neg_defaulted

    if (!(vm[positive].as<bool>() ^ vm[negative].as<bool>())) {
        throw boost::program_options::error(std::format("conflicting options --{} and --{} ", positive, negative));
    }
    return vm[positive].as<bool>();
}

int main(int argc, char** argv) {
    cli_params params;
    boost::program_options::variables_map vm;
    try{
        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
            ("help,h",            "Print help message")
            ("test,t",            boost::program_options::bool_switch()->default_value(false), "Full NxN comparison of N strings")
            ("input",             boost::program_options::value<std::string>(),                "Log file path")

            ("lmin,l",            boost::program_options::value<std::size_t>(),                "Minimum accepted length of a common subsequence")

            ("fixtures,f",        boost::program_options::bool_switch()->default_value(true),  "Apply all fixtures (default: on)")
            ("consolidated,c",    boost::program_options::bool_switch()->default_value(true),  "Enable consolidated view (default: on)")
            ("clusterwise,k",     boost::program_options::bool_switch()->default_value(false), "Enable clusterwise view (default: off)")

            ("no-fixtures,F",     boost::program_options::bool_switch()->default_value(false), "Disable fixtures (alias for --fixtures=false)")
            ("no-consolidated,C", boost::program_options::bool_switch()->default_value(false), "Disable consolidated view (alias for --consolidated=false)")
            ("no-clusterwise,K",  boost::program_options::bool_switch()->default_value(true),  "Disable clusterwise view (alias for --clusterwise=false)")

            // Clustering method (string) — default: dbscan
            ("clustering,m",      boost::program_options::value<std::string>()->default_value("dbscan"), "Clustering method (default: dbscan)")

            // DBSCAN parameters
            ("dbscan-eps",        boost::program_options::value<double>()->default_value(0.1),  "DBSCAN eps (distance threshold, must be > 0)")
            ("dbscan-minpts",     boost::program_options::value<int>()->default_value(3),       "DBSCAN minPts (minimum points in neighborhood, must be >= 1)")
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

    // { conditional requirement for input
    const bool test_mode = vm["test"].as<bool>();
    if (!test_mode && !vm.count("input")) {
        throw boost::program_options::error("missing required positional argument <path> (omit it only with --test)");
    }
    if (test_mode && vm.count("input")) {
        std::cerr << "Note: --test provided; ignoring positional <path>: " << vm["input"].as<std::string>() << "\n";
    }
    // }

    std::size_t lmin = vm["lmin"].as<std::size_t>();

    if(test_mode) {
        std::vector<std::string> lines;
        for (std::string s; std::getline(std::cin, s); ) {
            if (s.empty()) break;
            lines.push_back(std::move(s));
        }
        if (lines.size() < 2) {
            throw std::runtime_error("test mode expects at least two non-empty lines on stdin");
        }

        prova::loga::collection collection;
        for (const std::string& s : lines) {
            collection.add(s);
        }

        std::cout << std::format("Collection {}", collection.count()) << std::endl;
        std::size_t i = 0;
        for(const std::string& candidate: collection) {
            std::cout << std::format("[{}]: ", i++) << candidate << std::endl;
        }
        std::cout << std::endl;

        prova::loga::alignment alignment(collection);
        prova::loga::graph graph = alignment.bubble_all(lmin);
        graph.build();
        {
            std::ofstream graphml("paths.graphml");
            graph.print(graphml, collection);
        }
        prova::loga::path path = graph.shortest_path();
        path.print(std::cout, collection);
        std::cout << std::endl;
        return 0;
    }

    params.input = vm["input"].as<std::string>();
    std::cout << "Input: " << params.input << "\n";

    params.fixtures     = ensure_no_conflict(vm, "fixtures",      "no-fixtures");
    params.consolidated = ensure_no_conflict(vm, "consolidated",  "no-consolidated");
    params.clusterwise  = ensure_no_conflict(vm, "clusterwise",   "no-clusterwise");

    std::filesystem::path archive_file_path = std::format("{}-{}g.paths", params.input.filename().string(), lmin);

    prova::loga::collection collection;
    std::ifstream log(params.input);
    collection.parse(log);

    prova::loga::alignment alignment(collection);

    prova::loga::alignment::matrix_type paths;
    if(std::filesystem::exists(archive_file_path)){
        std::ifstream archive_file(archive_file_path);
        cereal::PortableBinaryInputArchive archive(archive_file);
        archive(paths);
    } else {
        alignment.bubble_all_pairwise(paths, lmin);

        std::ofstream archive_file(archive_file_path);
        cereal::PortableBinaryOutputArchive archive(archive_file);
        archive(paths);
    }

    prova::loga::distance distance(paths, collection.count());
    distance.compute(collection);
    {
        std::ofstream graphml("distances.graphml");
        distance.print_graphml(graphml);
    }
    prova::loga::distance::distance_matrix_type dmat = distance.matrix();
    // std::cout << "dmat " << dmat.size() << std::endl;

    prova::loga::cluster cluster(dmat);

    double dbscan_eps = vm["dbscan-eps"].as<double>();
    int dbscan_minpts = vm["dbscan-minpts"].as<int>();

    if (vm["dbscan-minpts"].as<int>() < 1) {
        throw boost::program_options::error("--dbscan-minpts must be >= 1");
    }

    cluster.compute(dbscan_eps, dbscan_minpts);
    prova::loga::cluster::labels_type labels = cluster.labels();
    // std::cout << "labels " << labels.size() << std::endl;

    const std::array<std::string, 20> fruits = {
        "Apple", "Banana", "Orange", "Mango", "Grape", "Pineapple", "Strawberry", "Cherry", "Peach", "Pear",
        "Watermelon", "Papaya", "Coconut", "Lemon", "Lime", "Kiwi", "Plum", "Apricot", "Pomegranate", "Guava"
    };

    std::map<std::size_t, parsed> parsed_log;
    prova::loga::group group(collection, labels);
    for(std::size_t c = 0; /*c != group.labels()*/; ++c) {
        if(c == group.labels()) {
            if(group.unclustered() == 0){
                break;
            } else {
                prova::loga::group::label_proxy proxy = group.proxy(std::numeric_limits<std::size_t>::max());
                std::cout << std::format("Unlabeled: ({})", proxy.count()) << std::endl;
                for(std::size_t i = 0; i != proxy.count(); ++i) {
                    auto v = proxy.at(i);
                    std::cout << v.str() << std::endl;
                }
                break;
            }
        }
        prova::loga::group::label_proxy proxy = group.proxy(c);
        std::size_t count = proxy.count();
        std::size_t min_matched = std::numeric_limits<std::size_t>::max();
        std::size_t min_matched_id = std::numeric_limits<std::size_t>::max();
        for(std::size_t i = 0; i != count; ++i) {
            auto v = proxy.at(i);
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

        if(params.clusterwise) {
            std::string cluster_name = (c < std::numeric_limits<std::size_t>::max()) ? ((c < fruits.size()) ? fruits.at(c) : std::format("C{}", c)) : " Failed ";
            std::cout << "Label: " << cluster_name << std::format(" ({})", count) << std::endl;
            std::cout << std::resetiosflags(std::ios::showbase) << std::right << std::setw(3) << "*" << min_matched_id << "|" << "\033[4m" << collection.at(min_matched_id) << "\033[0m" << std::resetiosflags(std::ios::showbase) << std::endl;
        }

        prova::loga::multi_alignment malign(collection, paths, proxy.mask(), min_matched_id);
        prova::loga::multi_alignment::region_map regions = malign.align();
        if(params.fixtures) {
            regions = malign.fixture_word_boundary(regions);
        }
        if(params.clusterwise) {
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

    std::size_t num_clusters = group.labels();
    auto begin = fruits.cbegin();
    auto end = begin;
    std::advance(end, num_clusters);
    std::string largest_cluster_name = *std::max_element(begin, end, [](const std::string& l, const std::string& r){
        return l < r;
    });

    // std::cout << "largest_cluster_name: " << largest_cluster_name << " " << num_clusters << std::endl;

    if(params.consolidated) {
        for(const auto& [id, p]: parsed_log) {
            std::string cluster_name = p.cluster() < std::numeric_limits<std::size_t>::max() ? ((p.cluster() < fruits.size()) ? fruits.at(p.cluster()) : std::format("C{}", p.cluster())) : " ";
            std::cout << std::right << std::setw(4) << id << " -> " << std::left << std::setw(largest_cluster_name.size()) << cluster_name << " | " << std::resetiosflags(std::ios::showbase);
            prova::loga::multi_alignment::print_interval_set(p.intervals(), collection.at(id), std::cout);
            std::cout << std::endl;
        }
    }
}

#endif
