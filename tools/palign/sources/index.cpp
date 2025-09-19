#include <loga/index.h>
#include <algorithm>
#include <numeric>
#include <boost/range/combine.hpp>
#include <loga/segment.h>

bool prova::loga::operator<(const prova::loga::index& left, const prova::loga::index& right){
    assert(left.count() == right.count());
    return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
}

bool prova::loga::operator>(const prova::loga::index& left, const prova::loga::index& right){
    assert(left.count() == right.count());
    return right < left;
}

bool prova::loga::operator==(const prova::loga::index& left, const prova::loga::index& right){
    assert(left.count() == right.count());

    auto combined = boost::combine(left, right);
    return std::all_of(std::begin(combined), std::end(combined), [](const auto& zipped){
        return zipped.template get<0>() == zipped.template get<1>();
    });
}

bool prova::loga::operator<=(const prova::loga::index& left, const prova::loga::index& right){
    return left == right || left < right;
}

bool prova::loga::operator>=(const prova::loga::index& left, const prova::loga::index& right){
    return left == right || left > right;
}

bool prova::loga::operator!=(const prova::loga::index& left, const prova::loga::index& right){
    return !operator==(left, right);
}

std::ostream& prova::loga::operator<<(std::ostream& stream, const prova::loga::index& idx){
    stream << "{";
    bool first = true;
    for(const auto& pos: idx._positions) {
        if (!first) stream << ", ";
        first = false;

        stream << pos;
    }
    stream << "}";
    return stream;
}

prova::loga::index prova::loga::operator+(const prova::loga::index& idx, std::size_t l){
    prova::loga::index moved{idx};
    moved.move_all(l);
    return moved;
}

prova::loga::index prova::loga::operator+(const prova::loga::index& left, const prova::loga::index& right){
    auto combined = boost::combine(left, right);
    std::vector<std::size_t> added;
    std::transform(combined.begin(), combined.end(), std::back_inserter(added), [](const auto& zipped){
        return zipped.template get<0>() + zipped.template get<1>();
    });
    return prova::loga::index{std::move(added)};
}

prova::loga::index prova::loga::operator-(const prova::loga::index& left, const prova::loga::index& right){
    auto combined = boost::combine(left, right);
    std::vector<std::size_t> added;
    std::transform(combined.begin(), combined.end(), std::back_inserter(added), [](const auto& zipped){
        auto res = zipped.template get<0>() - zipped.template get<1>();
        assert(res >= 0);
        return res;
    });
    return prova::loga::index{std::move(added)};
}

prova::loga::index prova::loga::operator-(const prova::loga::index& idx, std::size_t l){
    prova::loga::index moved{idx};
    moved.move_all(-l);
    return moved;
}

void prova::loga::index::move_all(std::int64_t delta){
    std::for_each(_positions.begin(), _positions.end(), [delta](std::size_t& v){
        assert(delta > 0 || v >= -delta);
        v = v+delta;
    });
}

void prova::loga::index::move(std::size_t dim, int64_t delta){
    std::size_t& v = _positions.at(dim);
    assert(delta > 0 || v >= -delta);
    v = v+delta;
}

prova::loga::index prova::loga::index::top_left() const {
    prova::loga::index br{*this};
    br.move_all(-1);
    return br;
}

prova::loga::index prova::loga::index::bottom_right() const{
    prova::loga::index br{*this};
    br.move_all(1);
    return br;
}

bool prova::loga::index::is_top() const{
    return std::any_of(_positions.begin(), _positions.end(), [](std::size_t v){
        return v == 0;
    });
}

std::size_t prova::loga::index::l1_from_zero() const{ return std::accumulate(begin(), end(), 0); }

prova::loga::index prova::loga::index::distance(const index &other) const{
    assert(other <= *this);

    prova::loga::index diff{_positions.size()};
    for(std::size_t i = 0; i != diff.count(); ++i){
        diff[i] = _positions[i] - other._positions[i];
    }
    return diff;
}
