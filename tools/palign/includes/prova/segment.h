#ifndef PROVA_ALIGN_SEGMENT_H
#define PROVA_ALIGN_SEGMENT_H

#include <prova/fwd.h>
#include <ostream>
#include <prova/index.h>

namespace prova::align{

std::ostream& operator<<(std::ostream& stream, const segment& s);

class segment{
    const std::string&  _base;
    prova::align::index _start;
    std::size_t         _length;

    friend std::ostream& operator<<(std::ostream& stream, const segment& s);

public:
    explicit segment(const std::string& base, index start, std::size_t length);
    inline segment(const segment& other) = default;
    segment(segment&& other);

    const std::string& base() const;
    const prova::align::index start() const;
    const prova::align::index end() const;
    std::size_t length() const;
    std::string_view view() const;
};

}

#endif // PROVA_ALIGN_SEGMENT_H
