#include <loga/path.h>
#include <cassert>
#include <numeric>
#include <loga/segment.h>
#include <algorithm>

prova::loga::path::path(const index &finish): _finish(finish) {}

prova::loga::path::path(path &&other): _segments(other._segments), _finish(std::move(other._finish)) {}

void prova::loga::path::add(const segment &s) { _segments.push_back(s); }

prova::loga::path::size_type prova::loga::path::size() const { return _segments.size(); }

prova::loga::path::const_iterator prova::loga::path::end() const { return _segments.end(); }

prova::loga::path::const_iterator prova::loga::path::begin() const { return _segments.begin(); }

std::ostream& prova::loga::path::print(std::ostream& out, const prova::loga::collection& collection) const  {
    std::size_t i = 0;
    for(const prova::loga::segment& s: _segments) {
        if(i != 0) {
            out << " -> ";
        }

        out << "{{{" << s.start() << "-" << s.end() << "|" << s.view(collection) << "}}}" << "\n";
        ++i;
    }
    out << " | " << score(collection);
    return out;
}

std::ostream &prova::loga::path::print(std::ostream &out, const tokenized_collection &collection) const {
    std::size_t i = 0;
    for(const prova::loga::segment& s: _segments) {
        if(i != 0) {
            out << " -> ";
        }

        out << "{{{" << s.start() << "-" << s.end() << "|" << s.view(collection) << "}}}" << "\n";
        ++i;
    }
    out << " | " << score(collection);
    return out;
}

const prova::loga::index &prova::loga::path::finish() const { return _finish; }

std::size_t prova::loga::path::matched() const{
    return std::accumulate(begin(), end(), 0, [](std::size_t last, const prova::loga::segment& s){
        return last + s.length();
    });
}

double prova::loga::path::score(const collection &collection) const{
    assert(size() > 0);
    return static_cast<double>(matched()) / static_cast<double>(std::ranges::max(_finish));
}

double prova::loga::path::score(const tokenized_collection &collection) const{
    assert(size() > 0);
    std::size_t matched = std::accumulate(begin(), end(), 0, [&collection](std::size_t last, const prova::loga::segment& s){
        return last + s.chars(collection);
    });
    return static_cast<double>(matched);
}


