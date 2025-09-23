#include <loga/collection.h>
#include <loga/alignment.h>
#include <loga/distance.h>
#include <loga/cluster.h>
#include <loga/group.h>
#include <loga/multi_alignment.h>
#include <iostream>

#include <boost/icl/interval.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/separate_interval_set.hpp>

int main() {
    prova::loga::collection collection;
    collection.add("Hello W Here I am do you hear me 579");
    collection.add("J Here Jx am do you hear me 18303");
    collection.add("Hola W Here We are do you hear me");

    prova::loga::alignment alignment(collection);
    prova::loga::alignment::matrix_type paths;
    alignment.bubble_all_pairwise(paths, 2);

    prova::loga::multi_alignment malign(collection, paths, 0);
    prova::loga::multi_alignment::region_map regions = malign.align();

    malign.print_regions(regions, std::cout) << std::endl;
    std::cout << "-----" << std::endl;
    regions = malign.fixture_word_booundary(regions);
    malign.print_regions(regions, std::cout) << std::endl;

    return 0;
}

// int main1() {
//     prova::loga::collection collection;
//     // collection.add("Hello W Here I am do you hear me 579");
//     // collection.add("J Here Jx am do you hear me 18303");
//     // collection.add("Hola W Here We are do you hear me");
//     std::ifstream log("Apache.small.log");
//     collection.parse(log);

//     prova::loga::alignment alignment(collection);

//     prova::loga::alignment::matrix_type paths;
//     alignment.bubble_all_pairwise(paths, 2);

//     prova::loga::distance distance(paths, collection.count());
//     distance.compute();
//     prova::loga::distance::distance_matrix_type dmat = distance.matrix();
//     std::cout << "dmat " << dmat.size() << std::endl;

//     prova::loga::cluster cluster(dmat);
//     cluster.compute(0.1, 3);
//     prova::loga::cluster::labels_type labels = cluster.labels();
//     std::cout << "labels " << labels.size() << std::endl;

//     prova::loga::group group(collection, labels);
//     for(std::uint32_t c = 0; c != group.labels(); ++c) {
//         std::cout << "Group: " << c << std::endl;
//         prova::loga::group::label_proxy proxy = group.proxy(c);
//         std::size_t count = proxy.count();
//         for(std::size_t i = 0; i != count; ++i) {
//             auto v = proxy.at(i);
//             std::cout << v.id() << " " << v.str() << std::endl;
//         }
//     //      prova::loga::multi_alignment malign(proxy);
//     //      prova::loga::multi_alignment::region_map regions = malign.align();
//     //      regions = malign.fixture_word_booundary(regions);
//     //      malign.print_regions(regions, std::cout) << std::endl;
//     }


//     // prova::loga::multi_alignment malign(collection, matrix, 0);
//     // prova::loga::multi_alignment::region_map regions = malign.align();

//     // malign.print_regions(regions, std::cout) << std::endl;
//     // std::cout << "-----" << std::endl;
//     // regions = malign.fixture_word_booundary(regions);
//     // malign.print_regions(regions, std::cout) << std::endl;
// }
