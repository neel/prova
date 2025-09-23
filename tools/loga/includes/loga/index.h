#ifndef PROVA_ALIGN_INDEX_H
#define PROVA_ALIGN_INDEX_H

#include <loga/fwd.h>
#include <ostream>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <cereal/archives/binary.hpp>

namespace prova::loga{

bool operator<(const index& left, const index& right);
bool operator>(const index& left, const index& right);
bool operator==(const index& left, const index& right);
bool operator!=(const index& left, const index& right);
bool operator<=(const index& left, const index& right);
bool operator>=(const index& left, const index& right);
std::ostream& operator<<(std::ostream& stream, const index& idx);
index operator+(const index& idx, std::size_t l);
index operator-(const index& idx, std::size_t l);
index operator+(const index& left, const index& right);
index operator-(const index& left, const index& right);

class index{
    std::vector<std::size_t> _positions;

    template <class Archive>
    void serialize(Archive & ar, std::uint32_t const version) {
        ar & _positions;
    }

    friend bool operator<(const index& left, const index& right);
    friend bool operator>(const index& left, const index& right);
    friend bool operator==(const index& left, const index& right);
    friend bool operator!=(const index& left, const index& right);
    friend bool operator<=(const index& left, const index& right);
    friend bool operator>=(const index& left, const index& right);
    friend std::ostream& operator<<(std::ostream& stream, const index& idx);
    friend index operator+(const index& idx, std::size_t l);
    friend index operator+(const index& left, const index& right);
    friend index operator-(const index& idx, std::size_t l);
    friend index operator-(const index& left, const index& right);
public:
    using const_iterator = std::vector<std::size_t>::const_iterator;
    using size_type = std::vector<std::size_t>::size_type;
public:
    inline explicit index(std::size_t dims): _positions(dims, 0) {
        assert(_positions.size() == dims);
        assert(_positions.size() >= 2);
    }
    explicit index(std::vector<std::size_t>&& positions) : _positions(std::move(positions)) {
        assert(_positions.size() >= 2);
    }

    void move_all(std::int64_t delta);
    void move(std::size_t dim, std::int64_t delta);
public:
    index bottom_right() const;
    index top_left() const;
    bool is_top() const;
public:
    inline size_type count() const { return _positions.size(); }
    inline const_iterator begin() const { return _positions.begin(); }
    inline const_iterator end() const { return _positions.end(); }
    inline std::size_t at(std::size_t index) const { return _positions.at(index); }
    inline std::size_t operator[](std::size_t index) const { return at(index); }

    inline std::size_t& at(std::size_t index) { return _positions.at(index); }
    inline std::size_t& operator[](std::size_t index) { return at(index); }

    index distance(const index& other) const;
    std::size_t l1_from_zero() const;
};

}

#endif // PROVA_ALIGN_INDEX_H
