#ifndef PROVA_ALIGN_PACKAGE_H
#define PROVA_ALIGN_PACKAGE_H

#include <loga/collection.h>
#include <loga/alignment.h>
#include <loga/distance.h>
#include <loga/multi_alignment.h>

namespace prova::loga {

class package{
    prova::loga::collection                     _collection;
    prova::loga::alignment::matrix_type         _pmat;
    prova::loga::distance::distance_matrix_type _dmat;
    std::map<std::size_t, int>                  _labels;
    std::map<std::size_t, prova::loga::multi_alignment::region_map> _regions;
};

}

#endif // PROVA_ALIGN_PACKAGE_H
