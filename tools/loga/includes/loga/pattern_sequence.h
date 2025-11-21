#ifndef PROVA_ALIGN_PATTERN_SEQUENCE_H
#define PROVA_ALIGN_PATTERN_SEQUENCE_H

#include <loga/token.h>
#include <loga/zone.h>
#include <numeric>
#include <vector>

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

#endif // PROVA_ALIGN_PATTERN_SEQUENCE_H
