#include <prova/collection.h>
#include <set>
#include <boost/range/combine.hpp>
#include <prova/index.h>

prova::align::collection::size_type prova::align::collection::count() const { return _inputs.size(); }

prova::align::collection::const_iterator prova::align::collection::begin() const { return _inputs.begin(); }

prova::align::collection::const_iterator prova::align::collection::end() const { return _inputs.end(); }

const std::string &prova::align::collection::at(std::size_t index) const { return _inputs.at(index); }

const std::string &prova::align::collection::operator[](std::size_t index) const { return at(index); }

void prova::align::collection::add(const std::string &str){
    _inputs.push_back(str);
}

std::size_t prova::align::collection::unique(const index &idx) const{
    std::set<char> chars;
    for(const auto& zipped: boost::combine(idx, _inputs)){
        std::size_t i = zipped.get<0>();
        const std::string& str = zipped.get<1>();
        chars.insert(str.at(i));
    }
    return chars.size();
}

bool prova::align::collection::unanimous_concensus(const index &idx) const{
    char first_char = 0;
    bool first_iteration = true;
    for(const auto& zipped: boost::combine(idx, _inputs)){
        std::size_t i = zipped.get<0>();
        const std::string& str = zipped.get<1>();
        char si = str.at(i);
        if(first_iteration) {
            first_char = si;
            first_iteration = false;
        }
        if(si != first_char){
            return false;
        }
    }
    return true;
}

