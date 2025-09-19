#ifndef PROVA_ALIGN_COLLECTION_H
#define PROVA_ALIGN_COLLECTION_H

#include <loga/face.h>
#include <vector>
#include <string>
#include <cstddef>

namespace prova::loga{

class collection{
    std::vector<std::string> _inputs;

    friend class face::slider::iterator;
public:
    using const_iterator = std::vector<std::string>::const_iterator;
    using size_type = std::vector<std::string>::size_type;
public:
    size_type count() const;
    const_iterator begin() const;
    const_iterator end() const;
    const std::string& at(std::size_t index) const;
    const std::string& operator[](std::size_t index) const;
public:
    void add(const std::string& str);
    std::size_t unique(const index& idx) const;
    bool unanimous_concensus(const index& idx) const;
};

}

#endif // PROVA_ALIGN_COLLECTION_H
