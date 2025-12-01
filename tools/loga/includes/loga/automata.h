#ifndef PROVA_ALIGN_AUTOMATA_H
#define PROVA_ALIGN_AUTOMATA_H

#include <string>
#include <vector>
#include <loga/pattern_sequence.h>
#include <loga/token.h>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/copy.hpp>
#include <boost/property_map/function_property_map.hpp>
#include <boost/graph/graphml.hpp>
#include <boost/graph/graphviz.hpp>
#include <armadillo>

namespace prova::loga{

struct automata{
    struct segment_vertex{
        std::size_t _id;
        std::string _str;
        std::size_t _count;
        bool _start  = false;
        bool _finish = false;
    };

    struct segment_edge{
        enum type{
            constant, placeholder, epsilon
        };
        type _type;
        std::string _str;
        std::size_t _id;
        std::vector<prova::loga::wrapped> _captured;

        inline segment_edge& operator=(const segment_edge& other) {
            _type = other._type;
            _str  = other._str;
            _id   = other._id;
            for(prova::loga::wrapped w: other._captured) {
                _captured.push_back(w);
            }
            return *this;
        }
    };

    using thompson_digraph_type = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, segment_vertex, segment_edge>;
    using vertex_type           = boost::graph_traits<thompson_digraph_type>::vertex_descriptor;
    using edge_type             = boost::graph_traits<thompson_digraph_type>::edge_descriptor;
    using terminal_pair_type    = std::pair<vertex_type, vertex_type>;
    using sequence_container    = std::vector<prova::loga::pattern_sequence>;
    using terminals_map         = std::map<std::size_t, terminal_pair_type>;

    thompson_digraph_type  _graph;
    sequence_container     _pseqs;
    terminals_map          _terminals;

    struct generialization_result{
        using segment_iterator = prova::loga::pattern_sequence::const_iterator;
        using progress_pair    = std::pair<std::size_t, std::size_t>;

        struct edge_intent{
            vertex_type  source;
            vertex_type  target;
            segment_edge edge;
        };

        using edges_container = std::vector<edge_intent>;

        vertex_type      last_v;
        segment_iterator last_it;
        std::size_t      tokens;
        std::size_t      base_progress;
        std::size_t      ref_progress;
        edges_container  etrace;

        generialization_result() = default;
        generialization_result(vertex_type v, segment_iterator it, progress_pair progress); // implies that the last_it segment was not visited because number of tokens visited in last_it is 0
        generialization_result(vertex_type v, segment_iterator it, std::size_t n, progress_pair progress);
        generialization_result(const generialization_result&) = default;
        generialization_result& operator=(const generialization_result&) = default;

        generialization_result& extend(const generialization_result& next);
    };

    template <typename Iterator>
    automata(Iterator begin, Iterator end): _pseqs(begin, end) {
        for(std::size_t i = 0; i < _pseqs.size(); ++i) {
            _terminals.insert(std::make_pair(
                i,
                std::make_pair(
                    boost::graph_traits<thompson_digraph_type>::null_vertex(),
                    boost::graph_traits<thompson_digraph_type>::null_vertex()
                    )
                ));
        }
    }

    void build();

    template <typename ValueT>
    void generialize(arma::Mat<ValueT>& coverage_mat, arma::Mat<ValueT>& capture_mat, bool skip = false) {
        coverage_mat.set_size(_pseqs.size(), _pseqs.size());
        capture_mat.set_size(_pseqs.size(), _pseqs.size());
        coverage_mat.fill(0);
        capture_mat.fill(0);

        for(std::size_t i = 0; i < _pseqs.size(); ++i) {
            const prova::loga::pattern_sequence& pseq_base = _pseqs.at(i);
            auto base_start  = _terminals.at(i).first;
            auto base_finish = _terminals.at(i).second;

            for(std::size_t j = 0; j < _pseqs.size(); ++j) {
                if(i == j) continue;

                const prova::loga::pattern_sequence& pseq_ref = _pseqs.at(j);
                auto ref_start  = _terminals.at(j).first;
                auto ref_finish = _terminals.at(j).second;

                generialization_result result = automata::directional_partial_generialize(_graph, base_start, pseq_ref, j, skip);
                std::size_t coverage = 0, capture = 0;
                for (auto& e : result.etrace) {
                    if (e.edge._type == segment_edge::placeholder) {
                        capture += e.edge._captured.size();
                    }
                }
                // (i, j) -> {coverage, capture}

                capture_mat(i, j)  = capture;

                if(result.last_v == base_finish) {
                    // reached finish point
                    // find the edge connecting to the finish vertex with id j

                    // { count long jumps

                    // }

                    edge_type e;
                    bool edge_found = false;
                    for(auto [ei, ei_end] = boost::in_edges(result.last_v, _graph); ei != ei_end; ++ei) {
                        const segment_edge& ep = _graph[*ei];
                        if(ep._id == j){
                            e = *ei;
                            edge_found = true;
                            break;
                        }
                    }
                    assert(edge_found);
                    auto [ne, neins] = boost::add_edge(boost::source(e, _graph), ref_finish, _graph);
                    assert(neins);
                    _graph[ne]._id   = _graph[e]._id;
                    _graph[ne]._type = _graph[e]._type;
                    _graph[ne]._str  = _graph[e]._str;
                    for(const auto& w: _graph[e]._captured) {
                        _graph[ne]._captured.push_back(w);
                    }
                    // auto ep = std::move(_graph[e]);
                    // boost::remove_edge(e, _graph);
                    // _graph[ne] = std::move(ep);
                }
            }
        }
        for(std::size_t i = 0; i < _pseqs.size(); ++i) {
            vertex_type start  = _terminals.at(i).first;
            vertex_type finish = _terminals.at(i).second;
            vertex_type v = start;

            std::set<std::size_t> aligned_references;
            while(v != finish) {
                std::set<std::size_t> current_references;
                edge_type base_e;
                bool base_found = false;
                for(auto [ei, ei_end] = boost::out_edges(v, _graph); ei != ei_end; ++ei) {
                    const segment_edge& ep = _graph[*ei];
                    if(ep._id != i) {
                        current_references.insert(ep._id);
                    } else {
                        base_e = *ei;
                        base_found = true;
                    }
                }

                assert(base_found);

                if(v == start) {
                    aligned_references = current_references;
                } else {
                    std::erase_if(aligned_references, [&](std::size_t v){
                        return !current_references.contains(v);
                    });
                }

                v = boost::target(base_e, _graph);
            }
            for(std::size_t c: aligned_references) {
                coverage_mat(c, i) = 1;
            }
        }
    }

    std::ostream& graphml(std::ostream& stream);

    std::ostream& graphviz(std::ostream& stream);

    void subgraph(const std::set<std::size_t>& subset, prova::loga::automata::thompson_digraph_type& graph);
    void subgraph(std::size_t base_id, prova::loga::automata::thompson_digraph_type& graph);
    static std::ostream& graphviz(std::ostream& stream, const thompson_digraph_type& graph);


    /**
     * @brief thompson_graph
     * @param pseq
     * @pre pseq contains a sequence of segments each having a zone (either constant or placeholder)
     * @pre two consecutive segments would always have different zones
     * @post generates a linear graph corresponding to the pattern depicted by pseq
     * @return
     */
    static std::pair<vertex_type, vertex_type> thompson_graph(thompson_digraph_type& graph, const prova::loga::pattern_sequence& pseq, std::size_t id);

    static generialization_result directional_partial_generialize(thompson_digraph_type& pattern_graph, vertex_type start, const prova::loga::pattern_sequence& pseq, std::size_t ref_id, bool skip = false);

    /**
     * @brief How far can this base pattern generalize that reference sequence under the placeholder rules
     * Keeps aligning the reference sequence with the base sequence's linear graph based on matching constant - constant tokens until either of the following happens
     *  - encountered constant - constant mismatch between the lase linear graph and reference sequence
     *  - lookahead of a base placeholder couldn't be aligned with a constant token of reference sequence
     *  - lookahead of a reference placeholder couldn't be aligned with a constant token of base linear graph
     *
     * @param pattern_graph
     * @param begin start position of the reference sequence
     * @param end   end position of the reference sequence
     * @param start start vertex of the base linear graph
     * @param ref_id id of the reference
     * @return
     */
    template <typename Iterator>
    static generialization_result formalized_directional_partial_generialize(const thompson_digraph_type& pattern_graph, vertex_type start, Iterator begin, Iterator end, std::size_t tindex, std::size_t ref_id) {
        std::size_t base_id = pattern_graph[start]._id;

        auto match = [&pattern_graph](vertex_type lv, Iterator segit, std::size_t tidx) -> bool {
            if(segit->tag() == prova::loga::zone::placeholder) return false;
            if(tidx >= segit->tokens().count())                return false;

            assert(segit->tag() == prova::loga::zone::constant);
            assert(tidx < segit->tokens().count());

            const auto& token = segit->tokens().at(tidx);
            const segment_vertex& vp = pattern_graph[lv];

            return token.view() == vp._str;
        };

        auto find_match = [&pattern_graph, &match](vertex_type lv, Iterator segit, std::size_t tidx) -> std::pair<bool, std::size_t> {
            bool m = match(lv, segit, tidx++);
            while(!m) {
                if(tidx < segit->tokens().count())
                    m = match(lv, segit, tidx++);
                else break;
            }
            return std::make_pair(m, tidx -1);                                            // under all circumstances tidx was incremented
        };

        /**
         * Doesn't updated shared variables only returns
         */
        auto next_base = [&pattern_graph, base_id](vertex_type lv) -> std::pair<edge_type, vertex_type> {
            bool edge_found = false;
            edge_type e;
            for(auto [ei, ei_end] = boost::out_edges(lv, pattern_graph); ei != ei_end; ++ei) {
                const segment_edge& ep = pattern_graph[*ei];
                if(ep._id == base_id){
                    e = *ei;
                    edge_found = true;
                    break;
                }
            }
            assert(edge_found);
            return {e, boost::target(e, pattern_graph)};
        };

        /**
         * Doesn't updated shared variables only returns
         */
        auto next_ref = [](Iterator segit, std::size_t tidx) -> std::pair<Iterator, std::size_t> {
            if(tidx +1 >= segit->tokens().count()) {
                return {++segit, 0};
            } else {
                return {segit, ++tidx};
            }
        };

        auto run_ref = [&find_match](vertex_type x_last_v, Iterator x_sit, std::size_t x_tindex, Iterator send, bool& found_ref, std::size_t& count_ref) -> std::pair<Iterator, std::size_t>{
            assert(!found_ref);
            assert(count_ref == 0);
            for(;x_sit != send;) {
                if(x_sit->tag() == prova::loga::zone::constant) {
                    bool matched;
                    std::tie(matched, x_tindex) = find_match(x_last_v, x_sit, x_tindex);
                    if(matched) {
                        found_ref = true;
                        break;
                    }
                }
                ++x_sit;                                                          // either no token found in sit segment or encountered a placeholder
                x_tindex = 0;                                                     // segment which cannot be matched due to lack of tokens in it
                ++count_ref;
            }
            return {x_sit, x_tindex};
        };

        auto run_base = [&pattern_graph, &match, &next_base](edge_type x_last_e, vertex_type x_last_v, Iterator x_sit, std::size_t x_tindex, bool& found_base, std::size_t& count_base) -> std::pair<edge_type, vertex_type>{
            if(x_sit->tag() == prova::loga::zone::constant) {
                while(!pattern_graph[x_last_v]._finish) {
                    bool matched = match(x_last_v, x_sit, x_tindex);
                    if(matched) {
                        found_base = true;
                        break;
                    }
                    std::tie(x_last_e, x_last_v) = next_base(x_last_v);
                }
                ++count_base;
            }
            return {x_last_e, x_last_v};
        };

        edge_type   last_e;
        vertex_type last_v = start;
        auto sit    = begin;
        auto send   = end;

        auto l_sit  = sit;                                                              // last matched reference segment
        auto l_v    = start;
        auto l_tidx = tindex;                                                           // last matched token index in the ;_sit reference segment

        std::size_t base_progress = 0;
        std::size_t ref_progress  = 0;
        std::vector<generialization_result::edge_intent> edges;

        auto advance_state = [&]() -> bool {
        // l_v to last_v read in base
        // l_sit:l_tidx to sit:tindex tokens read in reference
#if 0
            std::vector<std::string> debug_base_str;
#endif
            segment_edge::type base_hop = segment_edge::epsilon;
            std::size_t base_count = 0;
            {
                auto x_v = l_v;
                auto x_e = last_e;
                base_hop = pattern_graph[x_e]._type;
                while(x_v != last_v){
                    ++base_count;
                    const segment_edge& x_ep = pattern_graph[x_e];
                    if(x_ep._type == segment_edge::placeholder) { // placeholder has high precedence over other types
                        if(base_hop != segment_edge::placeholder)
                            base_hop = segment_edge::placeholder;
                    }
#if 0
                    debug_base_str.push_back(pattern_graph[x_v]._str);
#endif
                    std::tie(x_e, x_v) = next_base(x_v);
                }
#if 0
                debug_base_str.push_back(pattern_graph[x_v]._str);
#endif
            }
            base_progress += base_count;

            prova::loga::zone ref_zone = prova::loga::zone::constant;
            generialization_result::edge_intent e;
            e.source     = l_v;
            e.target     = last_v;
            e.edge._id   = ref_id;
            auto x_sit   = l_sit;
            auto x_tidx  = l_tidx;

            if(sit > l_sit || tindex > l_tidx) {
                std::tie(x_sit, x_tidx) = next_ref(x_sit, x_tidx);
            }

            while(x_sit != send && (x_sit < sit || (x_sit == sit && x_tidx <= tindex))) {
                ++ref_progress;
                auto zone = x_sit->tag();
                if(zone == prova::loga::zone::constant) {
                    auto w = x_sit->tokens().at(x_tidx);
                    e.edge._captured.emplace_back(w);
                } else {
                    e.edge._str = "$";
                }

                if(zone == prova::loga::zone::placeholder) { // placeholder has high precedence over other types
                    if(ref_zone != prova::loga::zone::placeholder)
                        ref_zone = prova::loga::zone::placeholder;
                }
                std::tie(x_sit, x_tidx) = next_ref(x_sit, x_tidx);
            }

            e.edge._type = (ref_zone == prova::loga::zone::placeholder)
                               ? segment_edge::placeholder
                               : (base_hop == segment_edge::epsilon)
                                     ? segment_edge::epsilon
                                     : segment_edge::constant;
#if 0
            {
                // for debug purpose only
                std::cout << (base_hop == segment_edge::placeholder ? "p" : "c");
                std::cout << "/" << boost::join(debug_base_str, "/") << "/";
                std::cout << " <> ";
                std::cout << e.edge._str;
                std::vector<std::string> buffer;
                for(const auto& w: e.edge._captured) {
                    buffer.emplace_back(w.view());
                }
                std::cout << "|" << boost::join(buffer, "|") << "|";
                std::cout << std::endl;
            }
#endif
            edges.emplace_back(std::move(e));

            l_v    = last_v;
            l_sit  = sit;
            l_tidx = tindex;

            bool res = false;
            if(!pattern_graph[last_v]._finish){
                std::tie(last_e, last_v) = next_base(last_v);
                res = true;
            }
            if(sit != send) {
                std::tie(sit, tindex)    = next_ref(sit, tindex);
                res = res & true;
            }
            return res;
        };

        if(pattern_graph[last_v]._finish){
            generialization_result res{l_v, l_sit, l_tidx, std::make_pair(base_progress, ref_progress)};
            res.etrace = std::move(edges);
            return res;
        }

        std::tie(last_e, last_v) = next_base(last_v);
        while(sit != send) {
            const segment_edge& lep = pattern_graph[last_e];
            auto base_type = lep._type;
            auto ref_type = sit->tag();
            if(base_type == segment_edge::constant && ref_type == prova::loga::zone::constant) {
                bool found = match(last_v, sit, tindex);
                if(!found) {
                    break;
                }
                else advance_state();
            } else {
                bool aligned = false;
                if(base_type == segment_edge::epsilon){
                    if(pattern_graph[last_v]._finish){
                        if(sit == send) {
                            aligned = true;
                        }
                        break;
                    }
                    else break; // unexpected
                }

                assert(base_type == segment_edge::placeholder || ref_type == prova::loga::zone::placeholder);

                if(ref_type == prova::loga::zone::placeholder){                        // if placeholder then there is no token in *sit
                    std::tie(sit, tindex) = next_ref(sit, tindex);                     // skip the current segment
                }                                                                      // move to the first token of the next segment

                bool found_ref          = false;
                bool found_base         = false;
                std::size_t count_ref   = 0;
                std::size_t count_base  = 0;

                auto x_tindex = tindex;
                auto x_sit    = sit;
                auto x_last_e = last_e;
                auto x_last_v = last_v;

                if(sit != send) {
                    std::tie(x_sit, x_tindex)    = run_ref(last_v, sit, tindex, send, found_ref, count_ref);
                    std::tie(x_last_e, x_last_v) = run_base(last_e, last_v, sit, tindex, found_base, count_base);
                }

                if(found_ref || found_base) {
                    aligned = true;
                    if(found_ref) {
                        sit    = x_sit;
                        tindex = x_tindex;
                    } else {
                        last_e = x_last_e;
                        last_v = x_last_v;
                    }
                } else {
                    if(pattern_graph[last_v]._finish) {
                        // reached finish vertex of the base graph
                        // base_type == segment_edge::placeholder || ref_type == prova::loga::zone::placeholder -> at least one is a placeholder
                        // if sit == send
                        //     send serves as the representative of finish vertex
                        //     both ends at the same time therefore aligned
                        // else
                        //     not enough evidence to call them aligned
                        if(sit == send) {
                            aligned = true;
                        }
                    }
                }
                if(!aligned) break;
                else {
                    if(!advance_state()){
                        break;
                    }
                }
            }
        }
        if(sit == send) {
            if(pattern_graph[last_v]._finish){
                advance_state();
            }
        }

        generialization_result res{l_v, l_sit, l_tidx, std::make_pair(base_progress, ref_progress)};
        res.etrace = std::move(edges);
        return res;
    }

    template <typename Iterator>
    static generialization_result formalized_directional_partial_piecewise_generialize(std::map<std::size_t, generialization_result>& memo, const thompson_digraph_type& pattern_graph, vertex_type base_v, Iterator begin, Iterator sit, Iterator end, std::size_t tindex, std::size_t ref_id) {
        const auto& base_vp = pattern_graph[base_v];

        std::size_t base_id = base_vp._id;

        std::size_t base_vertex_id = base_vp._count; // vertex number in the linear graph starting from 0 to N for finish
        std::size_t ref_token_id   = 0;
        for(auto t_sit = begin; t_sit != sit; ++t_sit){
            ref_token_id += t_sit->tokens().count();
        }
        ref_token_id += tindex;

        static thread_local int depth = 0;
        ++depth;
        // std::cerr << "ENTER depth=" << depth
        //           << " base_vid=" << base_vertex_id
        //           << " ref_id="   << ref_token_id << "\n";

        std::size_t key = 0;
        boost::hash_combine(key, base_vertex_id);
        boost::hash_combine(key, ref_token_id);

        auto send   = end;

        auto eoa = [&](vertex_type v, Iterator seg_it) -> bool{
            return pattern_graph[v]._finish || seg_it == send;
        };

        /**
         * Doesn't updated shared variables only returns
         */
        auto next_base = [&pattern_graph, base_id](vertex_type lv) -> std::pair<edge_type, vertex_type> {
            bool edge_found = false;
            edge_type e;
            for(auto [ei, ei_end] = boost::out_edges(lv, pattern_graph); ei != ei_end; ++ei) {
                const segment_edge& ep = pattern_graph[*ei];
                if(ep._id == base_id){
                    e = *ei;
                    edge_found = true;
                    break;
                }
            }
            assert(edge_found);
            return {e, boost::target(e, pattern_graph)};
        };

        /**
         * Doesn't updated shared variables only returns
         */
        auto next_ref = [](Iterator segit, std::size_t tidx) -> std::pair<Iterator, std::size_t> {
            if(tidx +1 >= segit->tokens().count()) {
                return {++segit, 0};
            } else {
                return {segit, ++tidx};
            }
        };

        generialization_result result; // default constructor added
        if(memo.contains(key)) {
            result = memo.at(key);
            --depth;
            // std::cerr << "EXIT  depth=" << depth
            //           << " base_vid=" << base_vertex_id
            //           << " ref_id="   << ref_token_id << "\n";
            return result;
        } else {
            result = formalized_directional_partial_generialize(pattern_graph, base_v, sit, send, tindex, ref_id);
        }

        if(eoa(result.last_v, result.last_it)) {
            memo.emplace(key, result);
            --depth;
            // std::cerr << "EXIT  depth=" << depth
            //           << " base_vid=" << base_vertex_id
            //           << " ref_id="   << ref_token_id << "\n";
            return result;
        }

        auto r_sit    = result.last_it;
        auto r_tindex = result.tokens;
        auto r_last_v = result.last_v;

        {
            std::size_t result_ref_token_id   = 0;
            for(auto t_sit = begin; t_sit != r_sit; ++t_sit){
                result_ref_token_id += t_sit->tokens().count();
            }
            result_ref_token_id += tindex;
            assert(result_ref_token_id >= ref_token_id);
            assert(pattern_graph[r_last_v]._count >= base_vertex_id);
        }

        auto x_sit    = r_sit;
        auto x_tindex = r_tindex;
        auto x_last_v = r_last_v;
        edge_type dummy_e;
        std::tie(x_sit,  x_tindex)  = next_ref(x_sit, x_tindex);
        std::tie(dummy_e, x_last_v) = next_base(x_last_v);

        std::size_t ref_shift_gain  = 0;
        std::size_t base_shift_gain = 0;
        std::size_t both_shift_gain = 0;

        generialization_result result_ref;
        generialization_result result_base;
        generialization_result result_both;

        if(x_sit != send) {
            result_ref      = formalized_directional_partial_piecewise_generialize(memo, pattern_graph, r_last_v, begin, x_sit, send, x_tindex, ref_id);
            ref_shift_gain  = std::min(result_ref.base_progress, result_ref.ref_progress);
        }

        if(!pattern_graph[x_last_v]._finish) {
            result_base     = formalized_directional_partial_piecewise_generialize(memo, pattern_graph, x_last_v, begin, r_sit, send, r_tindex, ref_id);
            base_shift_gain = std::min(result_base.base_progress, result_base.ref_progress);
        }

        if(x_sit != send && !pattern_graph[x_last_v]._finish) {
            result_both     = formalized_directional_partial_piecewise_generialize(memo, pattern_graph, x_last_v, begin, x_sit, send, x_tindex, ref_id);
            both_shift_gain = std::min(result_both.base_progress, result_both.ref_progress);
        }

        std::vector<std::size_t> gains{ref_shift_gain, base_shift_gain, both_shift_gain};

        auto max_gain_it = std::ranges::max_element(gains, std::greater<std::size_t>{});
        if(*max_gain_it == 0) {
            memo.emplace(key, result);
            --depth;
            // std::cerr << "EXIT  depth=" << depth
            //           << " base_vid=" << base_vertex_id
            //           << " ref_id="   << ref_token_id << "\n";
            return result;
        } else {
            std::size_t max_gain_dist = std::distance(gains.begin(), max_gain_it);

            switch (max_gain_dist) {
                case 0: result.extend(result_ref);  break;
                case 1: result.extend(result_base); break;
                case 2: result.extend(result_both); break;
            }
        }

        memo.emplace(key, result);
        --depth;
        // std::cerr << "EXIT  depth=" << depth
        //           << " base_vid=" << base_vertex_id
        //           << " ref_id="   << ref_token_id << "\n";
        return result;
    }

    static std::size_t apply_trace(thompson_digraph_type& graph, const generialization_result& res);

    void directional_subset_generialize(thompson_digraph_type& pattern_graph, const prova::loga::pattern_sequence& pseq, std::size_t base_id, std::size_t ref_id);

    // prova::loga::pattern_sequence merge(std::size_t id) const;
    prova::loga::pattern_sequence merge(std::size_t base_id, const std::set<std::size_t>& references) const;
    void clean(std::size_t base_id);
};

}

#endif // PROVA_ALIGN_AUTOMATA_H
