#include <loga/graph.h>
#include <loga/path.h>
#include <iostream>
#include <format>
#include <stack>
#include <boost/lexical_cast.hpp>
#include <boost/range/combine.hpp>
#include <boost/property_map/function_property_map.hpp>
#include <boost/property_map/dynamic_property_map.hpp>
#include <boost/graph/graphml.hpp>
#include <boost/graph/dag_shortest_paths.hpp>
#include <boost/graph/bellman_ford_shortest_paths.hpp>
#include <boost/graph/graphviz.hpp>

prova::loga::graph::graph(segment_collection_type&& segments, segment &&start, segment &&finish): _segments(std::move(segments)), _start(std::move(start)), _finish(std::move(finish)){

}

void prova::loga::graph::build(){
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
        const auto& segment = _segments[i];
        const auto& start   = segment.start();
        const auto& end     = segment.end();
        const auto& vertex  = segment_vertices[i];

        bool touches_start_line = std::ranges::any_of(boost::combine(start, _start.start()), [](const auto& zipped){
            return zipped.template get<0>() == zipped.template get<1>();
        });
        bool touches_finish_line = std::ranges::any_of(boost::combine(end, _finish.start()), [](const auto& zipped){
            return zipped.template get<0>() == zipped.template get<1>()-1;
        });

        bool touches_finish_corner = std::ranges::all_of(boost::combine(end, _finish.start()), [](const auto& zipped){
            return zipped.template get<0>() == zipped.template get<1>()-1;
        });
        bool touches_start_corner = std::ranges::all_of(boost::combine(start, _start.start()), [](const auto& zipped){
            return zipped.template get<0>() == zipped.template get<1>();
        });

        if(touches_start_corner || !touches_start_line || (!touches_start_line && !touches_start_corner)){ // S -> p
            auto pair = boost::add_edge(_S, vertex, _graph);
            if(pair.second) {
                auto e = pair.first;
                _graph[e].weight = dist_from_start(segment);
                _graph[e].slide  = 0;

                // std::cout << "* " << "S          \t->\t " << _segments[i] << " >> " << _graph[e].weight << std::endl;
            }
        }

        if(touches_finish_corner || !touches_finish_line || (!touches_finish_line && !touches_finish_corner)){ // p -> T
            auto pair = boost::add_edge(vertex, _T, _graph);
            if(pair.second) {
                auto e = pair.first;
                _graph[e].weight = dist_to_finish(segment);
                _graph[e].slide  = 0;

                // std::cout << "* " << _segments[i] << " \t->\t T" << " >> " << _graph[e].weight << std::endl;
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
                if(std::any_of(hop.begin(), hop.end(), [](const auto& d){ return d <= 1; })) {
                    continue;
                }
                std::size_t distance = hop.l1_from_zero();
                auto pair = boost::add_edge(segment_vertices[i], segment_vertices[j], _graph);
                if(pair.second) {
                    auto e = pair.first;
                    _graph[e].weight =  weight_fn(distance, q.length());
                    _graph[e].slide  = 0;

                    // std::cout << _segments[i] << " \t->\t " << _segments[j] << " >> " << _graph[e].weight << std::endl;
                }
            } else if(q_starts_after_p_starts && q_ends_after_p_ends) {    // at least one dimension overlaps
                // definitely q_starts_after_p_ends is false.
                // therefore q starts before p ends
                // therefore at least one dim overlaps

                // std::size_t dim = 0;
                // bool overlapped = false;
                // for(const auto& zipped: boost::combine(qs, pe)) {
                //     if(zipped.get<0>() == zipped.get<1>()) {
                //         overlapped = true;
                //         break;
                //     }
                //     dim++;
                // }
                // if(overlapped) continue;

                // std::size_t gap = pe[dim] - qs[dim];
                // std::size_t slide = gap +1;
                // index ql = pe;
                // ql[dim] = qs[dim] + slide;
                // index hop = ql - pe;
                // std::size_t distance = hop.l1_from_zero();
                // auto pair = boost::add_edge(segment_vertices[i], segment_vertices[j], _graph);
                // if(pair.second) {
                //     auto e = pair.first;
                //     _graph[e].weight = weight_fn(distance, q.length()-slide);
                //     _graph[e].slide  = slide;

                //     std::cout << _segments[i] << " \t->\t " << _segments[j] << " >> " << _graph[e].weight << " | " << _graph[e].slide << std::format("({} -> {})", boost::lexical_cast<std::string>(pe), boost::lexical_cast<std::string>(qs)) << std::endl;
                // }

            }
        }
    }
    // }
}

std::ostream& prova::loga::graph::print(std::ostream& stream, const prova::loga::collection& collection){
    auto vertex_label_map = boost::make_function_property_map<vertex_type>(
        [&](const vertex_type& v) -> std::string_view {
            if(v == _S) return "S";
            if(v == _T) return "T";
            // return boost::lexical_cast<std::string>(_segments.at(_vertices.at(v)));
            return _segments.at(_vertices.at(v)).view(collection);
        }
    );

    auto vertex_weight_map = boost::make_function_property_map<edge_type>(
        [&](const edge_type& e) -> double {
            double w = _graph[e].weight;
            return (w == 0) ? std::numeric_limits<double>::epsilon() : w;
        }
    );

    boost::dynamic_properties properties;
    properties.property("label", vertex_label_map);
    properties.property("weight", vertex_weight_map);

    boost::write_graphml(stream, _graph, properties, true);
    return stream;
}

const prova::loga::segment &prova::loga::graph::largest_segment() const {
    auto it = std::max_element(_segments.cbegin(), _segments.cend(), [](const segment& l, const segment& r){
        return l.length() < r.length();
    });
    return *it;
}

prova::loga::path prova::loga::graph::shortest_path(){
    std::size_t vertex_count = boost::num_vertices(_graph);

    if(vertex_count == 3){
        prova::loga::path path(_finish.start());
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

        prova::loga::path path(_finish.start());
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
