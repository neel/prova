#ifndef PROVA_ALIGN_TOKENIZED_COLLECTION_H
#define PROVA_ALIGN_TOKENIZED_COLLECTION_H

#include <vector>
#include <loga/token.h>
#include <loga/face.h>

namespace prova::loga{

class tokenized_collection{
    using tokenized_inputs = std::vector<prova::loga::tokenized>;
    using value_type       = prova::loga::tokenized;

    std::vector<std::string> _raw_inputs;
    tokenized_inputs  _tokenized_inputs;

    friend class face::slider::iterator;
public:
    using const_iterator = tokenized_inputs::const_iterator;
    using size_type      = tokenized_inputs::size_type;
public:
    size_type count() const;
    const_iterator begin() const;
    const_iterator end() const;
    const value_type& at(std::size_t index) const;
    const value_type& operator[](std::size_t index) const;
public:
    void add(const std::string& str);
    void parse(std::istream& stream);
    std::size_t unique(const index& idx) const;
    bool unanimous_concensus(const index& idx) const;
public:
    friend tokenized_collection& operator<<(tokenized_collection& col, const std::string& str);
};

tokenized_collection& operator<<(tokenized_collection& col, const std::string& str);

}

#endif // PROVA_ALIGN_TOKENIZED_COLLECTION_H
