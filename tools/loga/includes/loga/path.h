#ifndef PROVA_ALIGN_PATH_H
#define PROVA_ALIGN_PATH_H

#include <loga/fwd.h>
#include <vector>
#include <ostream>
#include <cstdint>

namespace prova::loga{

class path{
    using container_type = std::vector<segment>;

    template <class Archive>
    void serialize(Archive & ar, std::uint32_t const version) {
        ar & _segments;
    }

    container_type _segments;
public:
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;
    using value_type = container_type::value_type;
    using size_type = container_type::size_type;
    using reference = container_type::reference;
    using const_reference = container_type::const_reference;

    inline path() = default;
    inline path(const path&) = default;
    path(path&& other);

    void add(const segment &s);
    const_iterator begin() const;
    const_iterator end() const;
    size_type size() const;
    std::ostream& print(std::ostream& out);

    std::size_t matched() const;
    double score() const;
};



}

#endif // PROVA_ALIGN_PATH_H
