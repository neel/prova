#ifndef PROVA_ALIGN_MULTI_ALIGNMENT_H
#define PROVA_ALIGN_MULTI_ALIGNMENT_H

#include <loga/fwd.h>
#include <cstddef>
#include <loga/alignment.h>
#include <boost/icl/interval.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/separate_interval_set.hpp>
#include <loga/zone.h>

namespace prova {
namespace loga {

class multi_alignment{
    struct matched_val{
        std::size_t id;
        std::size_t ref_pos;
        std::size_t base_pos;
        
        bool operator<(const matched_val& other) const {
            return id < other.id;
        }
        
        bool operator==(const matched_val& other) const {
            return id == other.id && ref_pos == other.ref_pos && base_pos == other.base_pos;
        }
    };
    
    const collection& _collection;
    const prova::loga::alignment::matrix_type& _matrix;
    std::size_t _base_index;
    
public:
    using interval_val  = std::set<matched_val>;
    using interval_map  = boost::icl::split_interval_map<std::size_t, interval_val>;
    using interval_set  = boost::icl::split_interval_map<std::size_t, std::set<zone>>;
    using region_map    = std::map<std::size_t, interval_set>;
    using region_type   = interval_set::interval_type;
    using interval_type = interval_map::interval_type;
    
public:
    inline multi_alignment(const collection& collection, const alignment::matrix_type& matrix, std::size_t base_index): _collection(collection), _matrix(matrix), _base_index(base_index) {}
    region_map align() const;
    region_map fixture_word_booundary(const region_map &regions) const;
    
public:
    std::ostream& print_regions(const region_map& regions, std::ostream& stream);
};

} // namespace algorithms
} // namespace prova

#endif // PROVA_ALIGN_MULTI_ALIGNMENT_H
