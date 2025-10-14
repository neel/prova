#ifndef PROVA_ALIGN_PATH_H
#define PROVA_ALIGN_PATH_H

#include <loga/fwd.h>
#include <loga/index.h>
#include <vector>
#include <ostream>
#include <cstdint>
#include <cereal/types/vector.hpp>

namespace prova::loga{

class path{
    using container_type = std::vector<segment>;

    friend class cereal::access;

    template <class Archive>
    void serialize(Archive & ar, std::uint32_t const version) {
        ar(_segments, _finish);
    }

    container_type _segments;
    prova::loga::index _finish;

public:
    using iterator          = container_type::iterator;
    using const_iterator    = container_type::const_iterator;
    using value_type        = container_type::value_type;
    using size_type         = container_type::size_type;
    using reference         = container_type::reference;
    using const_reference   = container_type::const_reference;

    path() = default;
    path(const prova::loga::index& finish);
    inline path(const path&) = default;
    path(path&& other);

    void add(const segment &s);
    const_iterator begin() const;
    const_iterator end() const;
    size_type size() const;
    std::ostream& print(std::ostream& out, const collection &collection) const;
    std::ostream& print(std::ostream& out, const tokenized_collection &collection) const;

    const prova::loga::index& finish() const;

    std::size_t matched() const;
    double score(const prova::loga::collection& collection) const;
    double score(const prova::loga::tokenized_collection& collection) const;
};



}

#endif // PROVA_ALIGN_PATH_H
