#include "prova/algorithms.h"
#include <algorithm>
#include <cassert>
#include <boost/range/combine.hpp>
#include <boost/foreach.hpp>
#include <set>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>
#include <format>
#include <queue>
#include <bit>
#include <boost/lexical_cast.hpp>
#include <boost/graph/dag_shortest_paths.hpp>
#include <boost/graph/bellman_ford_shortest_paths.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/graphml.hpp>
#include <boost/property_map/function_property_map.hpp>

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
    assert(other <= *this);

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
            if(carry >= threshold-1) {
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

void prova::algorithms::alignment::bubble_pairwise(const_iterator u, const_iterator v, const index &idx, memo_type& memo, std::size_t threshold, std::size_t carry){
    assert(idx.count() == 2);
    bool color = (u->at(idx.at(0)) == v->at(idx.at(1)));
    if (color) {
        if(!idx.is_top()) {
            bubble_pairwise(u, v, idx.top_left(), memo, threshold, carry + 1);
        } else {
            if(carry >= threshold-1) {
                memo[idx] = carry + 1;
            }
        }
    } else {
        if (carry >= threshold) {
            memo[idx.bottom_right()] = carry;
        }
        if(!idx.is_top()) {
            bubble_pairwise(u, v, idx.top_left(), memo, threshold, 0);
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

prova::algorithms::graph prova::algorithms::alignment::bubble_all(std::size_t threshold) {
    assert(threshold > 0);
    std::size_t N = _collection.count();
    std::vector<std::size_t> L;
    L.reserve(N);
    std::transform(_collection.begin(), _collection.end(), std::back_inserter(L), [](const std::string& str){
        return str.size()-1;
    });

    for(std::size_t j = 0; j < N; ++j) {
        enumerate_mixed_radix(L, j, [threshold, this](std::vector<std::size_t> x){
            // std::cout << "[";
            // std::ranges::copy(x, std::ostream_iterator<std::size_t>(std::cout, ","));
            // std::cout << "]" << std::endl;
            bubble(index{std::move(x)}, threshold, 0);
        });
    }

    segment_collection_type segments;
    for(const auto& [idx, length]: _memo) {
        segments.emplace_back(segment{_collection.at(0), idx, length});
    }

    segment start{_collection.at(0), index{_collection.count()}, 0};

    std::vector<std::size_t> last_indices;
    std::transform(_collection.begin(), _collection.end(), std::back_inserter(last_indices), [](const std::string& str){
        return str.size();
    });

    segment finish{_collection.at(0), index{std::move(last_indices)}, 0};

    return prova::algorithms::graph{std::move(segments), std::move(start), std::move(finish)};
}

void prova::algorithms::alignment::bubble_all_pairwise(prova::algorithms::alignment::matrix_type& mat, std::size_t threshold){
    assert(threshold > 0);
    std::size_t N = 2;

    std::size_t u = 0;
    for(auto base = _collection.begin(); base != _collection.end(); ++base) {

        std::size_t v = 0;
        for(auto ref = _collection.begin(); ref != _collection.end(); ++ref) {
            if(base == ref) {
                ++v;
                continue;
            }

            std::vector<std::size_t> L{base->size()-1, ref->size()-1};
            memo_type memo;

            for(std::size_t j = 0; j < N; ++j) {
                enumerate_mixed_radix(L, j, [threshold, this, base, ref, &memo](std::vector<std::size_t> x){
                    // std::cout << "[";
                    // std::ranges::copy(x, std::ostream_iterator<std::size_t>(std::cout, ","));
                    // std::cout << "]" << std::endl;
                    bubble_pairwise(base, ref, index{std::move(x)}, memo, threshold, 0);
                });
            }

            segment_collection_type segments;
            for(const auto& [idx, length]: memo) {
                segments.emplace_back(segment{*base, idx, length});
            }
            segment start{*base, index{2}, 0};
            segment finish{*base, index{{base->size(), ref->size()}}, 0};

            prova::algorithms::graph graph{std::move(segments), std::move(start), std::move(finish)};
            graph.build();
            prova::algorithms::path path = graph.shortest_path();

            auto key = std::make_pair(u, v);
            // std::cout << "score: " << path.score() << std::endl;
            mat.emplace(key, std::move(path));
            // std::cout << "score: " << mat.at(key).score() << std::endl;
            ++v;
        }
        ++u;
    }
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



prova::algorithms::graph::graph(segment_collection_type&& segments, segment &&start, segment &&finish): _segments(std::move(segments)), _start(std::move(start)), _finish(std::move(finish)){

}

void prova::algorithms::graph::build(){
    // { formulas
    auto weight_fn = [](std::size_t h, std::size_t g){ return static_cast<std::int64_t>(h) - static_cast<std::int64_t>(g); };
    auto dist_from_start = [&weight_fn](const segment& s){
        return weight_fn(s.start().l1_from_zero(), s.length());
    };
    auto dist_to_finish = [this, &weight_fn](const segment& s){
        return weight_fn(_finish.start().distance(s.end()).l1_from_zero(), 1);
    };
    // }

    // { add terminal vertices
    _S = boost::add_vertex(_graph);
    _T = boost::add_vertex(_graph);
    // }

    // { add segment vertices
    std::vector<vertex_type> segment_vertices(_segments.size());
    for(std::size_t i = 0; i != _segments.size(); ++i) {
        segment_vertices[i] = boost::add_vertex(_graph);
        _vertices.emplace(segment_vertices[i], i);
    }
    // }

    // { terminal connections
    for(std::size_t i = 0; i != _segments.size(); ++i) {
        { // S -> p
            auto pair = boost::add_edge(_S, segment_vertices[i], _graph);
            if(pair.second) {
                auto e = pair.first;
                _graph[e].weight = dist_from_start(_segments[i]);
                _graph[e].slide  = 0;

                std::cout << "* " << "S          \t->\t " << _segments[i] << " >> " << _graph[e].weight << std::endl;
            }
        } { // p -> T
            auto pair = boost::add_edge(segment_vertices[i], _T, _graph);
            if(pair.second) {
                auto e = pair.first;
                _graph[e].weight = dist_to_finish(_segments[i]);
                _graph[e].slide  = 0;

                std::cout << "* " << _segments[i] << " \t->\t T" << " >> " << _graph[e].weight << std::endl;
            }
        }
    }
    // }

    // { inter-segment connections
    for(std::size_t i = 0; i != _segments.size(); ++i) {
        const segment& p = _segments[i];
        const index ps = p.start();
        const index pe = p.end();
        for(std::size_t j = 0; j != _segments.size(); ++j) {
            if(i == j) continue;
            const segment& q = _segments[j];
            const index qs = q.start();
            const index qe = q.end();

            // { q starts after p starts
            bool q_starts_after_p_starts = std::ranges::all_of(boost::combine(ps, qs), [](const auto& zipped){
                return zipped.template get<0>() < zipped.template get<1>();
            });
            bool q_starts_after_p_ends = std::ranges::all_of(boost::combine(pe, qs), [](const auto& zipped){
                return zipped.template get<0>() < zipped.template get<1>();
            });
            bool q_ends_after_p_ends = std::ranges::all_of(boost::combine(pe, qe), [](const auto& zipped){
                return zipped.template get<0>() < zipped.template get<1>();
            });
            // }

            if (q_starts_after_p_ends) {                                  // non overlapping
                index hop = qs - pe;
                std::size_t distance = hop.l1_from_zero();
                auto pair = boost::add_edge(segment_vertices[i], segment_vertices[j], _graph);
                if(pair.second) {
                    auto e = pair.first;
                    _graph[e].weight =  weight_fn(distance, q.length());
                    _graph[e].slide  = 0;

                    std::cout << _segments[i] << " \t->\t " << _segments[j] << " >> " << _graph[e].weight << std::endl;
                }
            } else if(q_starts_after_p_starts && q_ends_after_p_ends) {    // at least one dimension overlaps
                // definitely q_starts_after_p_ends is false.
                // therefore q starts before p ends
                // therefore at least one dim overlaps

                std::size_t dim = 0;
                bool overlapped = false;
                for(const auto& zipped: boost::combine(qs, pe)) {
                    if(zipped.get<0>() <= zipped.get<1>()) {
                        break;
                    }
                    dim++;
                }
                assert(dim < qs.count());

                std::size_t gap = pe[dim] - qs[dim];
                std::size_t slide = gap +1;
                index ql = pe;
                ql[dim] = qs[dim] + slide;
                index hop = ql - pe;
                std::size_t distance = hop.l1_from_zero();
                auto pair = boost::add_edge(segment_vertices[i], segment_vertices[j], _graph);
                if(pair.second) {
                    auto e = pair.first;
                    _graph[e].weight =  weight_fn(distance, q.length()-slide);
                    _graph[e].slide  = slide;

                    std::cout << _segments[i] << " \t->\t " << _segments[j] << " >> " << _graph[e].weight << " | " << _graph[e].slide << std::format("({} -> {})", boost::lexical_cast<std::string>(pe), boost::lexical_cast<std::string>(qs)) << std::endl;
                }

            }
        }
    }
    // }
}

std::ostream& prova::algorithms::graph::print(std::ostream& stream){
    auto vertex_label_map = boost::make_function_property_map<vertex_type>(
        [&](const vertex_type& v) -> std::string {
            if(v == _S) return "S";
            if(v == _T) return "T";
            return boost::lexical_cast<std::string>(_segments.at(_vertices.at(v)));
        }
    );

    auto vertex_weight_map = boost::make_function_property_map<edge_type>(
        [&](const edge_type& e) -> double {
            return _graph[e].weight;
        }
    );

    boost::dynamic_properties properties;
    properties.property("label", vertex_label_map);
    properties.property("weight", vertex_weight_map);

    boost::write_graphml(stream, _graph, properties, true);
    return stream;
}

prova::algorithms::path prova::algorithms::graph::shortest_path(){
    std::size_t vertex_count = boost::num_vertices(_graph);

    if(vertex_count == 3){
        prova::algorithms::path path;
        path.add(_segments.at(0));
        return path;
    }

    std::vector<double> distances(boost::num_vertices(_graph), std::numeric_limits<double>::infinity());
    std::vector<vertex_type> predecessors(boost::num_vertices(_graph), _S);
    auto distance_map = boost::make_iterator_property_map(distances.begin(), boost::get(boost::vertex_index, _graph));
    auto predecessor_map = boost::make_iterator_property_map(predecessors.begin(), boost::get(boost::vertex_index, _graph));

    auto weight_map = boost::make_function_property_map<edge_type>(
        [&](const edge_type& e) -> double {
            return _graph[e].weight;
        }
    );

    bool no_negative_cycle = boost::bellman_ford_shortest_paths(
        _graph,
        boost::num_vertices(_graph),
        boost::weight_map(weight_map)
            .distance_map(distance_map)
            .predecessor_map(predecessor_map)
            .root_vertex(_S)
    );

    if (!no_negative_cycle) {
        throw std::runtime_error{"Negative weight cycle detected"};
    } else {
        std::stack<vertex_type> vstack;
        vertex_type v = _T;
        std::int64_t cost = distances[v];
        while(v != _S) {
            if(v != _T)
                vstack.push(v);

            v = predecessors[v];
            cost += distances[v];
        }

        prova::algorithms::path path;
        while(!vstack.empty()) {
            vertex_type v = vstack.top();
            if(v != _S && v != _T) {
                std::size_t i = _vertices[v];
                const segment& s = _segments.at(i);
                path.add(s);
            }
            vstack.pop();
        }
        return path;
    }
}

std::ostream& prova::algorithms::path::print(std::ostream& out){
    std::size_t i = 0;
    for(const auto& s: _segments) {
        if(i != 0) {
            out << " -> ";
        }

        out << s.view();
        ++i;
    }
    out << " | " << score();
    return out;
}

std::size_t prova::algorithms::path::matched() const{
    return std::accumulate(begin(), end(), 0, [](std::size_t last, const auto& s){
        return last + s.length();
    });
}

double prova::algorithms::path::score() const{
    assert(size() > 0);
    const segment& first_segment = *begin();
    const std::string& base = first_segment.base();
    return static_cast<double>(matched()) / static_cast<double>(base.size());
}






