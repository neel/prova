#ifndef PROVA_ALIGN_SEGMENT_H
#define PROVA_ALIGN_SEGMENT_H

#include <loga/fwd.h>
#include <ostream>
#include <loga/index.h>

namespace prova::loga{

std::ostream& operator<<(std::ostream& stream, const segment& s);

class segment{
    const std::string&  _base;
    prova::loga::index _start;
    std::size_t         _length;

    friend std::ostream& operator<<(std::ostream& stream, const segment& s);

    template <class Archive>
    void serialize(Archive & ar, std::uint32_t const version) {
        ar & _base & _start & _length;
    }

public:
    explicit segment(const std::string& base, index start, std::size_t length);
    inline segment(const segment& other) = default;
    segment(segment&& other);

    const std::string& base() const;
    const prova::loga::index start() const;
    const prova::loga::index end() const;
    std::size_t length() const;
    std::string_view view() const;
};

}

#endif // PROVA_ALIGN_SEGMENT_H
