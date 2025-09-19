#include "loga/multi_alignment.h"
#include <iostream>
#include <format>

prova::loga::multi_alignment::region_map prova::loga::multi_alignment::align() const {
    interval_map intervals;
    
    for(const auto& [key, path]: _matrix) {
        if(key.first != _base_index) continue;
        std::cout << std::format("({},{})", key.first, key.second) << "| ";
        for(const auto& s: path){
            prova::loga::index start = s.start();
            std::size_t start_pos = start.at(0);
            std::size_t end_pos   = start_pos + s.length();
            interval_type::type interval = interval_type::right_open(start_pos, end_pos);
            interval_val val;
            matched_val matched;
            matched.id = key.second;
            matched.base_pos = start_pos;
            matched.ref_pos = start.at(1);
            val.insert(matched);
            intervals.add(std::make_pair(interval, val)); //
            std::cout << std::format("[{}, {})", start_pos, end_pos) << "-" << s.start();
            // std::cout << s << "~~~";
        }
        std::cout << std::endl;
    }
    
    region_map regions;
    
    for(std::size_t i = 0; i != _collection.count(); ++i) {
        regions.insert(std::make_pair(i, interval_set{}));
    }
    
    const std::string& base_ref = _collection.at(_base_index);
    // auto base_begin = base_ref.begin();
    for(const auto& iv: intervals) {
        if(iv.second.size() == _collection.count()-1) {
            std::size_t len = iv.first.upper() - iv.first.lower();
            // auto start = base_begin+iv.first.lower();
            // auto end   = start + len;
            // std::string_view base_view(start, end);
            // std::cout << iv.first << ": <" << base_view << ">" << std::endl;
            // regions[base_index].add(region_type::right_open(iv.first.lower(), iv.first.lower() +len));
            std::set<zone> zones;
            zones.insert(zone::constant);
            regions[_base_index].add(std::make_pair(region_type::right_open(iv.first.lower(), iv.first.lower() +len), zones));
            for(const matched_val& v: iv.second) {
                // std::cout << "\t" << v.id << "-> " << v.ref_pos << " (" << v.base_pos << ")" << std::endl;
                // const std::string& ref = _collection.at(v.id);
                std::size_t delta = iv.first.lower() - v.base_pos;
                std::size_t ref_start = v.ref_pos+delta;
                std::size_t ref_end   = delta+v.ref_pos+len;
                // std::string_view ref_view(ref.begin()+ref_start, ref.begin()+ref_end);
                // std::cout << "\t<" << ref_view << ">" << std::endl;
                // regions[v.id].add(region_type::closed(ref_start, ref_end));
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
        
        // for(const auto& z: candidate.second) {
        //     zone tag = *z.second.cbegin();
        //     const std::string& ref = _collection.at(candidate.first);
        //     std::cout << z.first << " <" << ref.substr(z.first.lower(), z.first.upper()-z.first.lower()) << "> " << tag << std::endl;
        // }
        
        // std::cout << std::endl;
    }
    
    return regions;
}

prova::loga::multi_alignment::region_map prova::loga::multi_alignment::fixture_word_booundary(const region_map &regions) const{
    // ensures that a matched region is surrounded by non-word characters including end of line
    static std::string alphabets = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_:/.";
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
                std::size_t dist_rear = std::distance(view.rbegin(), rear);
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


