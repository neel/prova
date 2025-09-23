#include <loga/collection.h>
#include <set>
#include <boost/range/combine.hpp>
#include <loga/index.h>
#include <boost/algorithm/string/trim.hpp>

prova::loga::collection::size_type prova::loga::collection::count() const { return _inputs.size(); }

prova::loga::collection::const_iterator prova::loga::collection::begin() const { return _inputs.begin(); }

prova::loga::collection::const_iterator prova::loga::collection::end() const { return _inputs.end(); }

const std::string &prova::loga::collection::at(std::size_t index) const { return _inputs.at(index); }

const std::string &prova::loga::collection::operator[](std::size_t index) const { return at(index); }

void prova::loga::collection::add(const std::string &str){
    _inputs.push_back(str);
}

void prova::loga::collection::parse(std::istream &stream){
    std::string line;
    while (std::getline(stream, line)) {
        boost::algorithm::trim(line);
        if (!line.empty()) {
            add(line);
        }
    }
}

prova::loga::collection& prova::loga::operator<<(collection& col, const std::string& str){
    col.add(str);
    return col;
}

std::size_t prova::loga::collection::unique(const index &idx) const{
    std::set<char> chars;
    for(const auto& zipped: boost::combine(idx, _inputs)){
        std::size_t i = zipped.get<0>();
        const std::string& str = zipped.get<1>();
        chars.insert(str.at(i));
    }
    return chars.size();
}

bool prova::loga::collection::unanimous_concensus(const index &idx) const{
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

