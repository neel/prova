#include "loga/multi_alignment.h"
#include <iostream>
#include <format>
#include <array>
#include <iomanip>



prova::loga::multi_alignment::region_map prova::loga::multi_alignment::align() const {
    interval_map intervals;

    assert(_filter.contains(_base_index));
    
    // { construct intervals mapping [s, e) -> {t, s, r}+
    //      by considering all paths starting from the base item to all reference items allowed by the filter
    //      where (s, e) are the offsets of the start and end position of the base string corresponding to a matched string segment
    //      t is the index of the reference string and r is the start position in the reference string that align with the matched segment
    for(const auto& [key, path]: _matrix) {
        if(key.first != _base_index) continue;
        if(!_filter.contains(key.second)) continue;
        // std::cout << std::format("({},{})", key.first, key.second) << "| ";
        for(const auto& s: path){
            prova::loga::index start = s.start();
            std::size_t start_pos = start.at(0);
            std::size_t end_pos   = start_pos + s.length();
            interval_type::type interval = interval_type::right_open(start_pos, end_pos);
            interval_val val;    // Set<{t, s, r}>()

            matched_val matched; // {t, s, r}
            matched.id = key.second;
            matched.base_pos = start_pos;
            matched.ref_pos = start.at(1);

            val.insert(matched);
            intervals.add(std::make_pair(interval, val)); //
            // std::cout << std::format("[{}, {})", start_pos, end_pos) << "-" << s.start();
            // std::cout << s << "~~~";
        }
        // std::cout << std::endl;
    }
    // }

    // postcondition: intervals only contains references allowed by the filter
    region_map regions;
    
    // { construct regions and empty regions map mapping t -> interval_set
    //      where interval_set is an association of [s, e) -> {constant, placeholder}
    //      and s, e are offsets in the reference string
    for(std::size_t i = 0; i != _collection.count(); ++i) {
        if(!_filter.contains(i)) continue;
        regions.insert(std::make_pair(i, interval_set{}));
    }
    // }
    // postcondition: all keys in the regions refers to an index pointing to an item allowed by the filter
    // postcondition: all values in the regions are empty interval_set

    const std::string& base_ref = _collection.at(_base_index);
    for(const auto& iv: intervals) {
        std::size_t num_references = iv.second.size();
        if(num_references >= (_filter.size()-1)/2) {
            std::size_t len = iv.first.upper() - iv.first.lower();
            std::set<zone> zones;
            zones.insert(zone::constant);
            regions[_base_index].add(std::make_pair(region_type::right_open(iv.first.lower(), iv.first.lower() +len), zones));
            for(const matched_val& v: iv.second) {
                std::size_t delta       = iv.first.lower() - v.base_pos;
                std::size_t ref_start   = v.ref_pos+delta;
                std::size_t ref_end     = delta+v.ref_pos+len;
                std::set<zone> zones;
                zones.insert(zone::constant);
                regions[v.id].add(std::make_pair(region_type::right_open(ref_start, ref_end), zones));
            }
        }
    }
    
    for(auto& candidate: regions) {
        // std::cout << candidate.first << " {" << candidate.second.size() << "}" << std::endl;
        std::size_t last = 0;
        std::vector<region_type> placeholders;
        for(const auto& z: candidate.second) {
            if(z.first.lower() > last) {
                placeholders.push_back(region_type::right_open(last, z.first.upper()));
            }
            last = z.first.upper();
        }
        std::size_t end = _collection.at(candidate.first).size();
        if(end > last) {
            placeholders.push_back(region_type::right_open(last, end));
        }
        
        std::set<zone> zones;
        zones.insert(zone::placeholder);
        for(const auto& p: placeholders) {
            candidate.second.add(std::make_pair(p, zones));
        }
    }
    
    return regions;
}

prova::loga::multi_alignment::region_map prova::loga::multi_alignment::fixture_word_boundary(const region_map &regions) const{
    // ensures that a matched region is surrounded by non-word characters including end of line
    static std::string alphabets = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_:/.@";
    region_map result;
    for(auto& candidate: regions) {
        std::size_t candidate_id = candidate.first;
        const interval_set& intervals = candidate.second;
        std::vector<std::pair<region_type, zone>> updated_regions;
        std::size_t squeeze_left_next = 0;
        for(auto& z: intervals) {
            auto& region = z.first;
            auto updated_region = region;
            // { check if there is any distance carried forward by the previous region
            if(squeeze_left_next > 0) {
                updated_region = region_type::right_open(updated_region.lower()-squeeze_left_next, updated_region.upper());
                squeeze_left_next = 0;
            }
            // }
            prova::loga::zone tag = *z.second.cbegin();
            if(tag == prova::loga::zone::constant) {
                const std::string& ref = _collection.at(candidate_id);
                // std::cout << z.first << " <" << ref.substr(region.lower(), region.upper()-region.lower()) << "> " << tag << std::endl;
                std::size_t len = region.upper()-region.lower();
                auto begin = ref.begin()+region.lower();
                std::string_view view{begin, begin+len};

                // { check right to find
                bool eol = (region.upper() == ref.size());
                auto rear = std::ranges::find_if_not(view.rbegin(), view.rend(), [&](auto const& x) {
                    return std::ranges::find(alphabets, x) != alphabets.end();
                });
                std::size_t dist_rear = eol ? 0 : std::distance(view.rbegin(), rear);
                if(dist_rear > 0) { // squeeze region from right
                    updated_region = region_type::right_open(updated_region.lower(), updated_region.upper()-dist_rear);
                    squeeze_left_next = dist_rear;
                }
                // }

                // { check left to find
                auto front = std::ranges::find_if_not(view.begin(), view.end(), [&](auto const& x) {
                    return std::ranges::find(alphabets, x) != alphabets.end();
                });
                std::size_t dist_front = std::distance(view.begin(), front);

                if(dist_front > 0 && dist_rear > 0 && dist_front == dist_rear) {
                    // PPPCCCPPP
                    // where C is not a word break
                    // therefore merge CCC with left and right Ps
                    // PPPPPPPPP
                    if(!updated_regions.empty()) {
                        // remove the previous region
                        auto& last = updated_regions.back().first;
                        squeeze_left_next += (last.upper() - last.lower());
                        updated_regions.pop_back();
                    }
                    continue;
                }

                if(dist_front > 0) { // squeeze region from right
                    updated_region = region_type::right_open(updated_region.lower()+dist_front, updated_region.upper());
                    if(!updated_regions.empty()) {
                        // expand the upper bound of the last region
                        auto& last = updated_regions.back().first;
                        last = region_type::right_open(last.lower(), last.upper()+dist_front);
                    } else {
                        // create a new placeholder
                        auto new_region = region_type::right_open(0, dist_front);
                        updated_regions.push_back(std::make_pair(new_region, zone::placeholder));
                    }
                }
                // }
                updated_regions.push_back(std::make_pair(updated_region, zone::constant));
            } else {
                updated_regions.push_back(std::make_pair(updated_region, zone::placeholder));
            }
        }

        interval_set updated_intervals;
        for(const auto& pair: updated_regions) {
            std::set<zone> zones;
            zones.insert(pair.second);
            updated_intervals.add(std::make_pair(pair.first, zones));
        }

        result.insert(std::make_pair(candidate_id, updated_intervals));
    }

    return result;

}

// prova::loga::multi_alignment::region_map prova::loga::multi_alignment::fixture_word_boundary(const region_map &regions) const{
//     // ensures that a matched region is surrounded by non-word characters including end of line
//     static std::string alphabets = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_:/.@";
//     region_map result;
//     for(auto& candidate: regions) {
//         std::size_t candidate_id = candidate.first;
//         const interval_set& intervals = candidate.second;
//         std::vector<std::pair<region_type, zone>> updated_regions;
//         std::size_t squeeze_left_next = 0;
//         for(auto& z: intervals) {
//             auto& region = z.first;
//             const std::string& ref = _collection.at(candidate_id);
//             // std::cout << z.first << " <" << ref.substr(region.lower(), region.upper()-region.lower()) << "> " << tag << std::endl;
//             std::size_t len = region.upper()-region.lower();
//             auto begin = ref.begin()+region.lower();
//             std::string_view view{begin, begin+len};
//             prova::loga::zone tag = *z.second.cbegin();
//             if(tag == prova::loga::zone::constant) {
//                 // { check right to find
//                 bool eol = (region.upper() == ref.size());
//                 auto rear = std::ranges::find_if_not(view.rbegin(), view.rend(), [&](auto const& x) {
//                     return std::ranges::find(alphabets, x) != alphabets.end();
//                 });
//                 std::size_t dist_rear = eol ? 0 : std::distance(view.rbegin(), rear);
//                 // }

//                 // { check left to find
//                 auto front = std::ranges::find_if_not(view.begin(), view.end(), [&](auto const& x) {
//                     return std::ranges::find(alphabets, x) != alphabets.end();
//                 });
//                 std::size_t dist_front = std::distance(view.begin(), front);
//                 // }

//                 // check lookback if previous was constant too
//                 if(updated_regions.size() > 0) {
//                     auto& last = updated_regions.back().first;
//                     auto last_zone = updated_regions.back().second;
//                     if(last_zone == prova::loga::zone::constant) {
//                         // this constant will be converted to placeholder and glued to the next placeholder
//                         squeeze_left_next += view.size();
//                         continue;
//                     }
//                 }

//                 // Observations:
//                 //      {PPPP}{CCC CCC}{QQQQ}    -> {PPPPCCC}{ }{CCCQQQQ}       : dist_rear >  0 && dist_front >  0
//                 //                                                                                                      => {p.l, p.u+dist_front}, {c.l+dist_front, c.u-dist_rear}, {n.l - dist_rear, n.u}
//                 //      {PPPP}{CCC CC CCC}{QQQQ} -> {PPPPCCC}{ CC }{CCCQQQQ}    : dist_rear >  0 && dist_front >  0
//                 //                                                                                                      => {p.l, p.u+dist_front}, {c.l+dist_front, c.u-dist_rear}, {n.l - dist_rear, n.u}
//                 //      {PPPP}{ CCCCCC}{QQQQ}    -> {PPPP}{ }{CCCCCCQQQQ}       : dist_rear >  0 && dist_front == 0
//                 //                                                                                                      => {p.l, p.u+dist_front}, {c.l+dist_front, c.u-dist_rear}, {n.l - dist_rear, n.u}
//                 //      {PPPP}{CCCCCC }{QQQQ}    -> {PPPPCCCCCC}{ }{QQQQ}       : dist_rear == 0 && dist_front >  0
//                 //                                                                                                      => {p.l, p.u+dist_front}, {c.l+dist_front, c.u-dist_rear}, {n.l - dist_rear, n.u}
//                 //      {PPP}{ CCC }{QQQ}        -> {PPP}{ CCC }{QQQ}           : dist_rear == 0 && dist_front == 0
//                 //                                                                                                      => pcn
//                 //      {PPP}{CCC}{QQQ}          -> {PPPCCCQQQ}                 : dist_rear == dist_front == view.size()
//                 //                                                                                                      => remove p; skip c; {n.l-view.size()-p.size(), n.u}


//                 if(dist_rear == view.size() && dist_front == view.size()){
//                     prova::loga::zone last_zone = prova::loga::zone::constant;
//                     std::size_t last_len = 0;
//                     if(updated_regions.size() > 0) {
//                         last_zone = updated_regions.back().second;
//                         last_len   = updated_regions.back().first.upper() - updated_regions.back().first.lower();
//                     }

//                     if(last_zone == prova::loga::zone::placeholder){
//                         updated_regions.pop_back();
//                         squeeze_left_next = view.size()+last_len;
//                     } else {
//                         updated_regions.push_back(std::make_pair(region, zone::constant));
//                     }
//                     continue;
//                 }

//                 if(dist_rear > 0 || dist_front > 0) {
//                     if(dist_front > 0 && !updated_regions.empty()) {
//                         auto& last = updated_regions.back().first;
//                         last = region_type::right_open(last.lower(), last.upper()+dist_front);
//                     }

//                     if(dist_rear > 0) {
//                         squeeze_left_next = dist_rear;
//                     }
//                     auto updated_region = region;
//                     updated_region = region_type::right_open(updated_region.lower()+dist_front, updated_region.upper()-dist_rear);
//                     updated_regions.push_back(std::make_pair(updated_region, zone::constant));
//                 } else {
//                     updated_regions.push_back(std::make_pair(region, zone::constant));
//                 }
//             } else {
//                 prova::loga::zone last_zone = prova::loga::zone::constant;
//                 std::size_t last_lb = 0;
//                 if(updated_regions.size() > 0) {
//                     last_zone = updated_regions.back().second;
//                     last_lb   = updated_regions.back().first.lower();
//                 }

//                 std::size_t lb = (last_zone == prova::loga::zone::constant) ? region.lower()-squeeze_left_next : last_lb;
//                 // { check if there is any distance carried forward by the previous region
//                 auto updated_region = region_type::right_open(lb, region.upper());
//                 squeeze_left_next = 0;
//                 // }
//                 updated_regions.push_back(std::make_pair(updated_region, zone::placeholder));
//             }
//         }

//         interval_set updated_intervals;
//         for(const auto& pair: updated_regions) {
//             std::set<zone> zones;
//             zones.insert(pair.second);
//             updated_intervals.add(std::make_pair(pair.first, zones));
//         }

//         result.insert(std::make_pair(candidate_id, updated_intervals));
//     }

//     return result;
// }


prova::loga::multi_alignment::multi_alignment(const collection &collection, const alignment::matrix_type &matrix, const filter_type &filter, std::size_t base_index): _collection(collection), _matrix(matrix), _filter(filter), _base_index(base_index) {}

prova::loga::multi_alignment::multi_alignment(const collection &collection, const alignment::matrix_type &matrix, std::size_t base_index): _collection(collection), _matrix(matrix), _base_index(base_index)  {
    for(std::size_t i = 0; i != _collection.count(); ++i) {
        _filter.insert(i);
    }
}

std::ostream &prova::loga::multi_alignment::print_regions(const region_map &regions, std::ostream &stream){
    for(auto& candidate: regions) {
        for(const auto& z: candidate.second) {
            prova::loga::zone tag = *z.second.cbegin(); // set has only one item
            const std::string& ref = _collection.at(candidate.first);
            std::size_t offset = z.first.lower();
            std::size_t len = z.first.upper()-z.first.lower();
            stream << z.first << " <" << ref.substr(offset, len) << "> " << tag << std::endl;
        }
        
        stream << std::endl;
    }
    return stream;
}

std::ostream &prova::loga::multi_alignment::print_regions_string(const region_map &regions, std::ostream &stream){
    constexpr std::string_view red     = "\033[0;31m";
    constexpr std::string_view green   = "\033[0;32m";
    constexpr std::string_view yellow  = "\033[0;33m";
    constexpr std::string_view blue    = "\033[0;34m";
    constexpr std::string_view magenta = "\033[0;35m";
    constexpr std::string_view cyan    = "\033[0;36m";
    constexpr std::string_view bright_black   = "\033[1;30m";
    constexpr std::string_view bright_red     = "\033[1;31m";
    constexpr std::string_view bright_green   = "\033[1;32m";
    constexpr std::string_view bright_yellow  = "\033[1;33m";
    constexpr std::string_view bright_blue    = "\033[1;34m";
    constexpr std::string_view bright_magenta = "\033[1;35m";
    constexpr std::string_view bright_cyan    = "\033[1;36m";

    constexpr std::array<std::string_view, 13> palette = {
        red, green, yellow, blue, magenta,
        cyan, bright_red, bright_green, bright_yellow,
        bright_blue, bright_magenta, bright_cyan, bright_black
    };

    constexpr std::string_view reset = "\033[0m";

    for(const auto& [id, zones]: regions) {
        stream << std::right << std::setw(5) << id << "|" << std::resetiosflags(std::ios::showbase);
        print_interval_set(zones, _collection.at(id), stream);
        stream << std::endl;
    }
    return stream;
}

std::ostream &prova::loga::multi_alignment::print_interval_set(const interval_set &interval, const std::string &ref, std::ostream &stream){
    constexpr std::string_view red     = "\033[0;31m";
    constexpr std::string_view green   = "\033[0;32m";
    constexpr std::string_view yellow  = "\033[0;33m";
    constexpr std::string_view blue    = "\033[0;34m";
    constexpr std::string_view magenta = "\033[0;35m";
    constexpr std::string_view cyan    = "\033[0;36m";
    constexpr std::string_view bright_black   = "\033[1;30m";
    constexpr std::string_view bright_red     = "\033[1;31m";
    constexpr std::string_view bright_green   = "\033[1;32m";
    constexpr std::string_view bright_yellow  = "\033[1;33m";
    constexpr std::string_view bright_blue    = "\033[1;34m";
    constexpr std::string_view bright_magenta = "\033[1;35m";
    constexpr std::string_view bright_cyan    = "\033[1;36m";

    constexpr std::array<std::string_view, 13> palette = {
        red, green, yellow, blue, magenta,
        cyan, bright_red, bright_green, bright_yellow,
        bright_blue, bright_magenta, bright_cyan, bright_black
    };

    constexpr std::string_view reset = "\033[0m";

    std::size_t placeholder_count = 0;
    for(const auto& z: interval) {
        prova::loga::zone tag = *z.second.cbegin(); // set has only one item
        std::size_t offset = z.first.lower();
        std::size_t len = z.first.upper()-z.first.lower();
        std::string substr = ref.substr(offset, len);
        // std::transform(substr.cbegin(), substr.cend(), substr.begin(), [](const char& c){
        //     return c == ' ' ? '~' : c;
        // });
        if(tag == zone::constant)
            stream << substr;
        else {
            const auto& color = palette.at(placeholder_count++ % palette.size());
            stream << color << substr << reset;
        }
    }
    return stream;
}



bool prova::loga::multi_alignment::matched_val::operator<(const matched_val &other) const {
    return id < other.id;
}

bool prova::loga::multi_alignment::matched_val::operator==(const matched_val &other) const {
    return id == other.id && ref_pos == other.ref_pos && base_pos == other.base_pos;
}

std::ostream &prova::loga::operator<<(std::ostream &stream, const multi_alignment::matched_val &mval) {
    stream << std::format("{}:{}>{}", mval.id, mval.base_pos, mval.ref_pos);
    return stream;
}

std::ostream &prova::loga::operator<<(std::ostream &stream, const std::set<multi_alignment::matched_val> &set) {
    std::copy(set.cbegin(), set.cend(), std::ostream_iterator<multi_alignment::matched_val>(stream, ", "));
    stream << std::endl;
    return stream;
}
