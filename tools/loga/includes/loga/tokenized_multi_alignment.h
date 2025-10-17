#ifndef PROVA_ALIGN_TOKENIZED_MULTI_ALIGNMENT_H
#define PROVA_ALIGN_TOKENIZED_MULTI_ALIGNMENT_H

#include <loga/fwd.h>
#include <cstddef>
#include <loga/tokenized_alignment.h>
#include <boost/icl/interval.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/separate_interval_set.hpp>
#include <loga/zone.h>
#include <loga/path.h>

namespace prova {
namespace loga {

class tokenized_multi_alignment{
public:
    struct matched_val{
        std::size_t id;
        std::size_t ref_pos;
        std::size_t base_pos;
        
        bool operator<(const matched_val& other) const;
        bool operator==(const matched_val& other) const;
    };
    
private:
    const tokenized_collection& _collection;
    const prova::loga::tokenized_alignment::matrix_type& _matrix;
    std::size_t _base_index;
    std::set<std::size_t> _filter;
    
public:
    using interval_val  = std::set<matched_val>;
    using interval_map  = boost::icl::split_interval_map<std::size_t, interval_val>;
    using interval_set  = boost::icl::split_interval_map<std::size_t, std::set<zone>>;
    using region_map    = std::map<std::size_t, interval_set>;
    using region_type   = interval_set::interval_type;
    using interval_type = interval_map::interval_type;
    using filter_type   = std::set<std::size_t>;
    
public:
    tokenized_multi_alignment(const tokenized_collection& collection, const tokenized_alignment::matrix_type& matrix, const filter_type& filter, std::size_t base_index);
    tokenized_multi_alignment(const tokenized_collection& collection, const tokenized_alignment::matrix_type& matrix, std::size_t base_index);
    region_map align() const;
    region_map fixture_word_boundary(const region_map &regions) const;
    
public:
    std::ostream& print_regions(const region_map& regions, std::ostream& stream);
    std::ostream& print_regions_string(const region_map& regions, std::ostream& stream);
    static std::ostream& print_interval_set(const interval_set& interval, const std::string& str, std::ostream& stream);
    static std::ostream& print_interval_set(const interval_set& interval, const prova::loga::tokenized& ref, std::ostream& stream);
};

std::ostream& operator<<(std::ostream& stream, const prova::loga::tokenized_multi_alignment::matched_val& mval);
std::ostream& operator<<(std::ostream& stream, const std::set<prova::loga::tokenized_multi_alignment::matched_val>& set);

} // namespace algorithms
} // namespace prova

#endif // PROVA_ALIGN_TOKENIZED_MULTI_ALIGNMENT_H
