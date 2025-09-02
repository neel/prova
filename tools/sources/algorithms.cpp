#include "prova/algorithms.h"
#include <algorithm>
#include <cassert>
#include <boost/range/combine.hpp>
#include <boost/foreach.hpp>
#include <set>
#include <numeric>
#include <iostream>

bool prova::algorithms::operator<(const index& left, const index& right){
    assert(left.count() == right.count());
    return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
}

bool prova::algorithms::operator>(const index& left, const index& right){
    assert(left.count() == right.count());
    return right < left;
}

bool prova::algorithms::operator==(const index& left, const index& right){
    assert(left.count() == right.count());

    auto combined = boost::combine(left, right);
    return std::all_of(std::begin(combined), std::end(combined), [](const auto& zipped){
        return zipped.template get<0>() == zipped.template get<1>();
    });
}

bool prova::algorithms::operator<=(const index& left, const index& right){
    return left == right || left < right;
}

bool prova::algorithms::operator>=(const index& left, const index& right){
    return left == right || left > right;
}

bool prova::algorithms::operator!=(const index& left, const index& right){
    return !operator==(left, right);
}

std::ostream& prova::algorithms::operator<<(std::ostream& stream, const prova::algorithms::index& idx){
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

std::ostream& prova::algorithms::operator<<(std::ostream& stream, const prova::algorithms::segment& s){
    stream << s._start << ": " << s._length << " " << s.view();
    return stream;
}

prova::algorithms::index prova::algorithms::operator+(const prova::algorithms::index& idx, std::size_t l){
    prova::algorithms::index moved{idx};
    moved.move_all(l);
    return moved;
}

prova::algorithms::index prova::algorithms::operator+(const prova::algorithms::index& left, const prova::algorithms::index& right){
    auto combined = boost::combine(left, right);
    std::vector<std::size_t> added;
    std::transform(combined.begin(), combined.end(), std::back_inserter(added), [](const auto& zipped){
        return zipped.template get<0>() + zipped.template get<1>();
    });
    return prova::algorithms::index{std::move(added)};
}

prova::algorithms::index prova::algorithms::operator-(const prova::algorithms::index& left, const prova::algorithms::index& right){
    auto combined = boost::combine(left, right);
    std::vector<std::size_t> added;
    std::transform(combined.begin(), combined.end(), std::back_inserter(added), [](const auto& zipped){
        auto res = zipped.template get<0>() - zipped.template get<1>();
        assert(res >= 0);
        return res;
    });
    return prova::algorithms::index{std::move(added)};
}

prova::algorithms::index prova::algorithms::operator-(const prova::algorithms::index& idx, std::size_t l){
    prova::algorithms::index moved{idx};
    moved.move_all(-l);
    return moved;
}

void prova::algorithms::index::move_all(std::int64_t delta){
    std::for_each(_positions.begin(), _positions.end(), [delta](std::size_t& v){
        assert(delta > 0 || v >= -delta);
        v = v+delta;
    });
}

void prova::algorithms::index::move(std::size_t dim, int64_t delta){
    std::size_t& v = _positions.at(dim);
    assert(delta > 0 || v >= -delta);
    v = v+delta;
}

prova::algorithms::index prova::algorithms::index::top_left() const {
    prova::algorithms::index br{*this};
    br.move_all(-1);
    return br;
}

prova::algorithms::index prova::algorithms::index::bottom_right() const{
    prova::algorithms::index br{*this};
    br.move_all(1);
    return br;
}

bool prova::algorithms::index::is_top() const{
    return std::any_of(_positions.begin(), _positions.end(), [](std::size_t v){
        return v == 0;
    });
}

std::size_t prova::algorithms::index::l1_from_zero() const{ return std::accumulate(begin(), end(), 0); }

prova::algorithms::index prova::algorithms::index::distance(const index &other) const{
    assert(other < *this);

    prova::algorithms::index diff{_positions.size()};
    for(std::size_t i = 0; i != diff.count(); ++i){
        diff[i] = _positions[i] - other._positions[i];
    }
    return diff;
}

void prova::algorithms::collection::add(const std::string &str){
    _inputs.push_back(str);
}

std::size_t prova::algorithms::collection::unique(const index &idx) const{
    std::set<char> chars;
    for(const auto& zipped: boost::combine(idx, _inputs)){
        std::size_t i = zipped.get<0>();
        const std::string& str = zipped.get<1>();
        chars.insert(str.at(i));
    }
    return chars.size();
}

bool prova::algorithms::collection::unanimous_concensus(const index &idx) const{
    char first_char = 0;
    bool first_iteration = true;
    for(const auto& zipped: boost::combine(idx, _inputs)){
        std::size_t i = zipped.get<0>();
        const std::string& str = zipped.get<1>();
        char si = str.at(i);
        if(first_iteration) {
            first_char = si;
            first_iteration = false;
        }
        if(si != first_char){
            return false;
        }
    }
    return true;
}

void prova::algorithms::alignment::bubble(const index& idx, std::size_t threshold, std::size_t carry) {
    bool color = _collection.unanimous_concensus(idx);
    if (color) {
        if(!idx.is_top()) {
            bubble(idx.top_left(), threshold, carry + 1);
        } else {
            if(carry > threshold-1) {
                _memo[idx] = carry + 1;
            }
        }
    } else {
        if (carry >= threshold) {
            _memo[idx.bottom_right()] = carry;
        }
        if(!idx.is_top()) {
            bubble(idx.top_left(), threshold, 0);
        }
    }
}

// Iterative: least-significant index 0 rolls fastest.
void enumerate_mixed_radix(const std::vector<std::size_t>& L, std::size_t j, const std::function<void(std::vector<std::size_t>)>& visit) {
    const std::size_t N = L.size();
    std::vector<std::size_t> x(N, 0);
    x[j] = L[j];

    while (true) {
        visit(x);

        std::size_t i = 0;
        while (i < N && x[i] == L[i]) {
            if(i != j) x[i] = 0;
            ++i;
        }
        if (i == N) {
            break; // finished
        }
        if(i != j)
            ++x[i];
    }
}

void prova::algorithms::alignment::bubble_all(std::size_t threshold) {
    assert(threshold > 0);
    std::size_t N = _collection.count();
    std::vector<std::size_t> L;
    L.reserve(N);
    std::transform(_collection.begin(), _collection.end(), std::back_inserter(L), [](const std::string& str){
        return str.size()-1;
    });

    for(std::size_t j = 0; j < N; ++j) {
        enumerate_mixed_radix(L, j, [threshold, this](std::vector<std::size_t> x){
            bubble(index{std::move(x)}, threshold, 0);
        });
    }

    for(const auto& [idx, length]: _memo) {
        _segments.emplace_back(segment{_collection.at(0), idx, length});
    }
}

void prova::algorithms::alignment::build_graph(){
    graph_type G;

    // { formulas
    auto weight_fn = [](std::size_t h, std::size_t g){ return h - g; };
    auto dist_from_start = [&weight_fn](const segment& s){
        return weight_fn(s.start().l1_from_zero(), s.length());
    };
    auto dist_to_finish = [this, &weight_fn](const segment& s){
        return weight_fn(finish_segment().start().distance(s.end()).l1_from_zero(), 1);
    };
    auto dist = [](const segment& x, const segment& y){

    };
    // }

    // { add terminal vertices
    vertex_type S = boost::add_vertex(G);
    vertex_type T = boost::add_vertex(G);
    // }

    // { add segment vertices
    std::vector<vertex_type> segment_vertices(_segments.size());
    for(std::size_t i = 0; i != _segments.size(); ++i) {
        segment_vertices[i] = boost::add_vertex(G);
    }
    // }

    // { terminal connections
    for(std::size_t i = 0; i != _segments.size(); ++i) {
        { // S -> p
            auto pair = boost::add_edge(S, segment_vertices[i], G);
            if(pair.second) {
                auto e = pair.first;
                G[e].weight = dist_from_start(_segments[i]);
                G[e].slide  = 0;
            }
        } { // p -> T
            auto pair = boost::add_edge(segment_vertices[i], T, G);
            if(pair.second) {
                auto e = pair.first;
                G[e].weight = dist_to_finish(_segments[i]);
                G[e].slide  = 0;
            }
        }
    }
    // }

    // { inter-segment connections
    for(std::size_t i = 0; i != _segments.size(); ++i) {
        const segment& p = _segments[i];
        for(std::size_t j = 0; j != _segments.size(); ++j) {
            if(i == j) continue;
            const segment& q = _segments[j];
            if (q.start() > p.end()) {                                  // non overlapping
                index hop = q.start() - p.end();
                std::size_t distance = hop.l1_from_zero();
                auto pair = boost::add_edge(i, j, G);
                if(pair.second) {
                    auto e = pair.first;
                    G[e].weight =  weight_fn(distance, q.length());
                    G[e].slide  = 0;
                }
            } else {     // at least one dimension overlap
                index pe = p.end();
                index qs = q.start();
                std::size_t dim = 0;
                bool aligned = false;
                for(const auto& zipped: boost::combine(pe, qs)) {
                    bool less = zipped.get<0>() < zipped.get<1>();
                    if(less) {
                        aligned = true;
                        break;
                    }
                    dim++;
                }
                if(aligned) {
                    std::size_t gap = pe[dim] - qs[dim];
                    std::size_t slide = gap +1;
                    index ql = qs + slide;
                    index hop = ql - pe;
                    std::size_t distance = hop.l1_from_zero();
                    auto pair = boost::add_edge(segment_vertices[i], segment_vertices[j], G);
                    if(pair.second) {
                        auto e = pair.first;
                        G[e].weight =  weight_fn(distance, q.length()-slide);
                        G[e].slide  = slide;
                    }
                }

            }
        }
    }
    // }
}

prova::algorithms::index prova::algorithms::face::move(const index &idx, int64_t delta) const {
    index moved{idx};
    moved.move(_index, delta);
    return moved;
}

prova::algorithms::index prova::algorithms::face::slider::first() const{
    index idx{_face._dimensions};
    for(std::size_t dim = 0; dim != _face._dimensions; ++dim) {
        idx.at(dim) = _face._collection.at(dim).size() -1;
    }
    idx.at(_face._index) = min();
    return idx;
}

prova::algorithms::index prova::algorithms::face::slider::last() const{
    index idx{_face._dimensions};
    for(std::size_t dim = 0; dim != _face._dimensions; ++dim) {
        idx.at(dim) = _face._collection.at(dim).size() -1;
    }
    idx.at(_face._index) = max();
    return idx;
}

prova::algorithms::face::slider::iterator::value_type prova::algorithms::face::slider::iterator::operator*() const{
    index idx{_slider._face._dimensions};
    for(std::size_t dim = 0; dim != _slider._face._dimensions; ++dim) {
        idx.at(dim) = _slider._face._collection.at(dim).size() -1;
    }
    idx.at(_slider._face._index) = _at;
    return idx;
}

prova::algorithms::face::face(const collection& coll, std::size_t index): _collection(coll), _dimensions(coll.count()), _index(index) {
    assert(index < _dimensions);
}

std::size_t prova::algorithms::face::slider::min() const {
    return 0;
}

std::size_t prova::algorithms::face::slider::max() const {
    return _face._collection.at(_face._index).size() -1;
}


