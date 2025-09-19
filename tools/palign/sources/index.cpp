#include <prova/index.h>
#include <algorithm>
#include <numeric>
#include <boost/range/combine.hpp>
#include <prova/segment.h>

bool prova::align::operator<(const prova::align::index& left, const prova::align::index& right){
    assert(left.count() == right.count());
    return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
}

bool prova::align::operator>(const prova::align::index& left, const prova::align::index& right){
    assert(left.count() == right.count());
    return right < left;
}

bool prova::align::operator==(const prova::align::index& left, const prova::align::index& right){
    assert(left.count() == right.count());

    auto combined = boost::combine(left, right);
    return std::all_of(std::begin(combined), std::end(combined), [](const auto& zipped){
        return zipped.template get<0>() == zipped.template get<1>();
    });
}

bool prova::align::operator<=(const prova::align::index& left, const prova::align::index& right){
    return left == right || left < right;
}

bool prova::align::operator>=(const prova::align::index& left, const prova::align::index& right){
    return left == right || left > right;
}

bool prova::align::operator!=(const prova::align::index& left, const prova::align::index& right){
    return !operator==(left, right);
}

std::ostream& prova::align::operator<<(std::ostream& stream, const prova::align::index& idx){
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

prova::align::index prova::align::operator+(const prova::align::index& idx, std::size_t l){
    prova::align::index moved{idx};
    moved.move_all(l);
    return moved;
}

prova::align::index prova::align::operator+(const prova::align::index& left, const prova::align::index& right){
    auto combined = boost::combine(left, right);
    std::vector<std::size_t> added;
    std::transform(combined.begin(), combined.end(), std::back_inserter(added), [](const auto& zipped){
        return zipped.template get<0>() + zipped.template get<1>();
    });
    return prova::align::index{std::move(added)};
}

prova::align::index prova::align::operator-(const prova::align::index& left, const prova::align::index& right){
    auto combined = boost::combine(left, right);
    std::vector<std::size_t> added;
    std::transform(combined.begin(), combined.end(), std::back_inserter(added), [](const auto& zipped){
        auto res = zipped.template get<0>() - zipped.template get<1>();
        assert(res >= 0);
        return res;
    });
    return prova::align::index{std::move(added)};
}

prova::align::index prova::align::operator-(const prova::align::index& idx, std::size_t l){
    prova::align::index moved{idx};
    moved.move_all(-l);
    return moved;
}

void prova::align::index::move_all(std::int64_t delta){
    std::for_each(_positions.begin(), _positions.end(), [delta](std::size_t& v){
        assert(delta > 0 || v >= -delta);
        v = v+delta;
    });
}

void prova::align::index::move(std::size_t dim, int64_t delta){
    std::size_t& v = _positions.at(dim);
    assert(delta > 0 || v >= -delta);
    v = v+delta;
}

prova::align::index prova::align::index::top_left() const {
    prova::align::index br{*this};
    br.move_all(-1);
    return br;
}

prova::align::index prova::align::index::bottom_right() const{
    prova::align::index br{*this};
    br.move_all(1);
    return br;
}

bool prova::align::index::is_top() const{
    return std::any_of(_positions.begin(), _positions.end(), [](std::size_t v){
        return v == 0;
    });
}

std::size_t prova::align::index::l1_from_zero() const{ return std::accumulate(begin(), end(), 0); }

prova::align::index prova::align::index::distance(const index &other) const{
    assert(other <= *this);

    prova::align::index diff{_positions.size()};
    for(std::size_t i = 0; i != diff.count(); ++i){
        diff[i] = _positions[i] - other._positions[i];
    }
    return diff;
}
