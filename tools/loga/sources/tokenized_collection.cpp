#include <loga/tokenized_collection.h>
#include <boost/range/combine.hpp>
#include <loga/index.h>
#include <boost/algorithm/string/trim.hpp>

prova::loga::tokenized_collection::size_type prova::loga::tokenized_collection::count() const { return _tokenized_inputs.size(); }

prova::loga::tokenized_collection::const_iterator prova::loga::tokenized_collection::begin() const { return _tokenized_inputs.begin(); }

prova::loga::tokenized_collection::const_iterator prova::loga::tokenized_collection::end() const { return _tokenized_inputs.end(); }

const prova::loga::tokenized_collection::value_type& prova::loga::tokenized_collection::at(std::size_t index) const { return _tokenized_inputs.at(index); }

const prova::loga::tokenized_collection::value_type& prova::loga::tokenized_collection::operator [](std::size_t index) const { return at(index); }


void prova::loga::tokenized_collection::add(const std::string& str) {
    _raw_inputs.push_back(str);
    _tokenized_inputs.emplace_back(prova::loga::tokenized(str));
}

void prova::loga::tokenized_collection::parse(std::istream &stream){
    std::string line;
    while (std::getline(stream, line)) {
        boost::algorithm::trim(line);
        if (!line.empty()) {
            add(line);
        }
    }
}

prova::loga::tokenized_collection& prova::loga::operator<<(prova::loga::tokenized_collection& col, const std::string& str){
    col.add(str);
    return col;
}

std::size_t prova::loga::tokenized_collection::unique(const index &idx) const{
    std::set<prova::loga::wrapped> tokens;
    for(const auto& zipped: boost::combine(idx, _tokenized_inputs)){
        std::size_t i = zipped.get<0>();
        const prova::loga::tokenized& tokenized = zipped.get<1>();
        tokens.insert(tokenized.at(i));
    }
    return tokens.size();
}

bool prova::loga::tokenized_collection::unanimous_concensus(const index &idx) const{
    std::set<prova::loga::wrapped> tokens;
    for(const auto& zipped: boost::combine(idx, _tokenized_inputs)){
        std::size_t i = zipped.get<0>();
        const prova::loga::tokenized& tokenized = zipped.get<1>();
        const prova::loga::wrapped& si = tokenized.at(i);
        if(tokens.size() == 0) {
            tokens.insert(si);
        }
        if(si != *tokens.cbegin()){
            return false;
        }
    }
    return true;
}
