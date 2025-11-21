#ifndef PROVA_ALIGN_PATTERN_SEQUENCE_H
#define PROVA_ALIGN_PATTERN_SEQUENCE_H

#include <loga/token.h>
#include <loga/zone.h>
#include <numeric>
#include <vector>
#include <format>

namespace prova::loga{

struct pattern_sequence {
    struct segment {
        prova::loga::tokenized _tokens;
        prova::loga::zone      _tag;
        
        inline explicit segment(const prova::loga::zone& tag): _tag(tag), _tokens(prova::loga::tokenized::nothing()) {}
        inline segment(const prova::loga::zone& tag, const std::string& str): _tag(tag),  _tokens(str) {}
        
        inline segment& operator=(const std::string& str) {
            _tokens = prova::loga::tokenized(str);
            return *this;
        }
        
        inline const prova::loga::zone& tag() const noexcept { return _tag; }
        inline const prova::loga::tokenized& tokens() const noexcept { return _tokens; }
        
        inline std::size_t hash() const { return std::hash<std::string>{}(_tokens.raw()); }
    };
    
    std::vector<segment> _segments;
    
    using const_iterator = std::vector<segment>::const_iterator;
    using iterator       = std::vector<segment>::iterator;
    
    inline void add(segment&& s) { _segments.push_back(std::move(s)); }
    
    inline iterator begin() noexcept { return _segments.begin(); }
    inline iterator end() noexcept { return _segments.end(); }
    
    inline const_iterator begin() const noexcept { return _segments.begin(); }
    inline const_iterator end() const noexcept { return _segments.end(); }
    
    inline const segment& at(std::size_t i) const { return _segments.at(i); }
    
    inline std::size_t nconstants() const noexcept {
        return std::accumulate(_segments.cbegin(), _segments.cend(), 0, [](std::size_t res, const segment& s){
            return s.tag() == prova::loga::zone::constant;
        });
    }
    
    inline std::size_t size() const noexcept { return _segments.size(); }
};

template <typename Stream>
Stream& print_pattern(Stream& stream, const pattern_sequence& pat){
    std::size_t placeholder_count = 0;
    for(const pattern_sequence::segment& segment: pat){
        auto tag = segment.tag();
        if(tag == prova::loga::zone::constant) {
            auto substr = segment.tokens().raw();
            stream << substr;
        } else {
            const auto& color = prova::loga::colors::palette.at(placeholder_count % prova::loga::colors::palette.size());
            stream << color << std::format("${}", placeholder_count) << prova::loga::colors::reset;
            ++placeholder_count;
        }
    }
    return stream;
}

}

#endif // PROVA_ALIGN_PATTERN_SEQUENCE_H
