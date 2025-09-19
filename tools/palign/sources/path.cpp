#include <loga/path.h>
#include <cassert>
#include <numeric>
#include <loga/segment.h>

prova::loga::path::path(path &&other): _segments(other._segments) {}

void prova::loga::path::add(const segment &s) { _segments.push_back(s); }

prova::loga::path::size_type prova::loga::path::size() const { return _segments.size(); }

prova::loga::path::const_iterator prova::loga::path::end() const { return _segments.end(); }

prova::loga::path::const_iterator prova::loga::path::begin() const { return _segments.begin(); }

std::ostream& prova::loga::path::print(std::ostream& out){
    std::size_t i = 0;
    for(const auto& s: _segments) {
        if(i != 0) {
            out << " -> ";
        }

        out << s.view();
        ++i;
    }
    out << " | " << score();
    return out;
}

std::size_t prova::loga::path::matched() const{
    return std::accumulate(begin(), end(), 0, [](std::size_t last, const auto& s){
        return last + s.length();
    });
}

double prova::loga::path::score() const{
    assert(size() > 0);
    const segment& first_segment = *begin();
    const std::string& base = first_segment.base();
    return static_cast<double>(matched()) / static_cast<double>(base.size());
}


