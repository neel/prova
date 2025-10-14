#include <loga/token.h>
#include <loga/tokenized_collection.h>
#include <loga/tokenized_alignment.h>
#include <loga/tokenized_distance.h>
#include <loga/cluster.h>
#include <loga/path.h>
#include <loga/tokenized_group.h>
#include <loga/tokenized_multi_alignment.h>
#include <iostream>
#include <filesystem>
#include <cereal/archives/portable_binary.hpp>
#include <igraph/igraph.h>

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

int main() {
    prova::loga::tokenized_collection collection;
    // collection.add("Hello W Here I am do you hear me 579");
    // collection.add("J Here Jx am do you hear me 18303");
    // // collection.add("Hola W Here We are do you hear me");
    // // prova::loga::tokenized_alignment::matrix_type tpaths;
    // // prova::loga::tokenized_distance tdistance(tpaths, collection.count());
    // // tdistance.compute(collection, 1);
    // const auto& l_structure = collection.at(0).structure();
    // const auto& r_structure = collection.at(1).structure();
    // double d = prova::loga::levenshtein_distance(l_structure.cbegin(), l_structure.cend(), r_structure.cbegin(), r_structure.cend());
    // std::cout << "d: " << d << std::endl;
    // return 1;

    // collection
    //     << "- 1131566463 2005.11.09 cn142 Nov 9 12:01:03 cn142/cn142 ntpd[7467]: synchronized to 10.100.20.250, stratum 3"
    //     << "- 1131566467 2005.11.09 cn46 Nov 9 12:01:07 cn46/cn46 ntpd[15291]: synchronized to 10.100.16.250, stratum 3"
    //     << "- 1131566470 2005.11.09 cn661 Nov 9 12:01:10 cn661/cn661 ntpd[18505]: synchronized to 10.100.20.250, stratum 3"
    //     << "- 1131566473 2005.11.09 cn379 Nov 9 12:01:13 cn379/cn379 ntpd[10573]: synchronized to 10.100.18.250, stratum 3"
    //     << "- 1131566473 2005.11.09 cn733 Nov 9 12:01:13 cn733/cn733 ntpd[27716]: synchronized to 10.100.20.250, stratum 3"
    //     << "- 1131566477 2005.11.09 cn543 Nov 9 12:01:17 cn543/cn543 ntpd[13785]: synchronized to 10.100.18.250, stratum 3"
    //     << "- 1131566479 2005.11.09 bn431 Nov 9 12:01:19 bn431/bn431 ntpd[28723]: synchronized to 10.100.20.250, stratum 3"
    //     << "- 1131566484 2005.11.09 cn931 Nov 9 12:01:24 cn931/cn931 ntpd[29054]: synchronized to 10.100.20.250, stratum 3"
    //     ;

    std::ifstream log("Thunderbird_2k.small.log");
    collection.parse(log);

    std::filesystem::path archive_file_path = std::format("Thunderbird_2k.small.log.1g.paths");

    for(const prova::loga::tokenized& word: collection) {
        std::cout << word << std::endl;
    }

    prova::loga::tokenized_alignment alignment(collection);
    prova::loga::tokenized_alignment::matrix_type paths;
    if(std::filesystem::exists(archive_file_path)){
        std::ifstream archive_file(archive_file_path);
        cereal::PortableBinaryInputArchive archive(archive_file);
        archive(paths);
    } else {
        alignment.bubble_all_pairwise(paths, 1);

        std::ofstream archive_file(archive_file_path);
        cereal::PortableBinaryOutputArchive archive(archive_file);
        archive(paths);
    }

    prova::loga::tokenized_distance distance(paths, collection.count());
    distance.compute(collection, 1);
    {
        std::ofstream graphml("distances.graphml");
        distance.print_graphml(graphml);
    }
    prova::loga::tokenized_distance::distance_matrix_type dmat = distance.matrix();
    std::cout << "dmat " << dmat.size() << std::endl;

    auto bgl_graph = distance.knn_graph(1);
    igraph_t igraph;
    igraph_vector_t edges;
    prova::loga::bgl_to_igraph(bgl_graph, &igraph, &edges);
    prova::loga::cluster::labels_type labels(collection.count());
    prova::loga::leiden_membership(&igraph, &edges, labels);



    // prova::loga::cluster cluster(dmat);

    // cluster.compute(0.0001, 3);
    // prova::loga::cluster::labels_type labels = cluster.labels();
    // std::cout << "labels " << labels.size() << std::endl;

    const std::array<std::string, 20> fruits = {
        "Apple", "Banana", "Orange", "Mango", "Grape", "Pineapple", "Strawberry", "Cherry", "Peach", "Pear",
        "Watermelon", "Papaya", "Coconut", "Lemon", "Lime", "Kiwi", "Plum", "Apricot", "Pomegranate", "Guava"
    };

    std::map<std::size_t, parsed> parsed_log;
    prova::loga::tokenized_group group(collection, labels);
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

        if(true /*params.clusterwise*/) {
            std::string cluster_name = (c < std::numeric_limits<std::size_t>::max()) ? ((c < fruits.size()) ? fruits.at(c) : std::format("C{}", c)) : " Failed ";
            std::cout << "Label: " << cluster_name << std::format(" ({})", count) << std::endl;
            std::cout << std::resetiosflags(std::ios::showbase) << std::right << std::setw(3) << "*" << min_matched_id << "|" << "\033[4m" << collection.at(min_matched_id) << "\033[0m" << std::resetiosflags(std::ios::showbase) << std::endl;
        }

        prova::loga::tokenized_multi_alignment malign(collection, paths, proxy.mask(), min_matched_id);
        prova::loga::tokenized_multi_alignment::region_map regions = malign.align();
        if(false /*params.fixtures*/) {
            // regions = malign.fixture_word_boundary(regions);
        }
        if(true /*params.clusterwise*/) {
            malign.print_regions_string(regions, std::cout) << std::endl;
        }

        // for(auto& [id, zones]: regions) {
        //     parsed p(id, c, std::move(zones));
        //     parsed_log.emplace(std::make_pair(id, p));
        // }
    }
    return 0;
}
