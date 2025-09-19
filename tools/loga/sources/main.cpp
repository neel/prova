#include "loga/alignment.h"
#include "loga/multi_alignment.h"
#include <iostream>

#include <boost/icl/interval.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/separate_interval_set.hpp>

int main() {
    prova::loga::alignment alignment;
    alignment.add("Hello W Here I am do you hear me 579");
    alignment.add("J Here Jx am do you hear me 18303");
    alignment.add("Hola W Here We are do you hear me");

    prova::loga::alignment::matrix_type matrix;
    alignment.bubble_all_pairwise(matrix, 2);

    prova::loga::multi_alignment malign(alignment.inputs(), matrix, 0);
    prova::loga::multi_alignment::region_map regions = malign.align();

    malign.print_regions(regions, std::cout) << std::endl;
    std::cout << "-----" << std::endl;
    regions = malign.fixture_word_booundary(regions);
    malign.print_regions(regions, std::cout) << std::endl;
}
