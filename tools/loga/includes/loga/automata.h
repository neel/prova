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
        
        generialization_result(vertex_type v, segment_iterator it, progress_pair progress); // implies that the last_it segment was not visited because number of tokens visited in last_it is 0
        generialization_result(vertex_type v, segment_iterator it, std::size_t n, progress_pair progress);
        generialization_result(const generialization_result&) = default;
        generialization_result& operator=(const generialization_result&) = default;
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
    void generialize(arma::Mat<ValueT>& coverage_mat, arma::Mat<ValueT>& capture_mat) {
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
                
                generialization_result result = automata::directional_partial_generialize(_graph, pseq_ref, base_start, j);
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
            vertex_type finish = _terminals.at(i).second;
            std::set<std::size_t> coverage;
            for(auto [ei, ei_end] = boost::in_edges(finish, _graph); ei != ei_end; ++ei) {
                const segment_edge& ep = _graph[*ei];
                if(ep._id != i)
                    coverage.insert(ep._id);
            }
            for(std::size_t c: coverage) {
                coverage_mat(c, i) = 1;
            }
        }
    }
    
    std::ostream& graphml(std::ostream& stream);
    
    std::ostream& graphviz(std::ostream& stream);
    
    
    /**
     * @brief thompson_graph
     * @param pseq
     * @pre pseq contains a sequence of segments each having a zone (either constant or placeholder)
     * @pre two consecutive segments would always have different zones
     * @post generates a linear graph corresponding to the pattern depicted by pseq
     * @return
     */
    static std::pair<vertex_type, vertex_type> thompson_graph(thompson_digraph_type& graph, const prova::loga::pattern_sequence& pseq, std::size_t id);
    
    static generialization_result directional_partial_generialize(thompson_digraph_type& pattern_graph, const prova::loga::pattern_sequence& pseq, vertex_type start, std::size_t ref_id);
    
    template <typename Iterator>
    static generialization_result formalized_directional_partial_generialize(const thompson_digraph_type& pattern_graph, Iterator begin, Iterator end, vertex_type start, std::size_t ref_id) {
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
        std::size_t tindex = 0;
        
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
        
        std::tie(last_e, last_v) = next_base(last_v);
        while(sit != send) {
            const segment_edge& lep = pattern_graph[last_e];
            auto base_type = lep._type;
            auto ref_type = sit->tag();
            if(base_type == segment_edge::constant && ref_type == prova::loga::zone::constant) {
                bool found = match(last_v, sit, tindex);
                if(!found) break;
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
                
                if(ref_type == prova::loga::zone::placeholder){                         // if placeholder then there is no token in *sit
                    // ++sit;                                                              // skip the current segment
                    // tindex = 0;                                                         // move to the first token of the next segment
                    std::tie(sit, tindex) = next_ref(sit, tindex);
                }
                
                do{                                 // irrespective of base_type next vertex has an _str depicting a constant
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
                        break;
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
                        break;
                    }
                }while(!pattern_graph[last_v]._finish);
                
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
    
    /**
     * @brief partial_generialize finds if the pattern_graph generializes pseq partially or not
     * @param pattern_graph
     * @param pseq
     * @pre pseq contains a sequence of segments each having a zone (either constant or placeholder)
     * @pre two consecutive segments would always have different zones
     * @return
     */
    template <typename Iterator>
    static generialization_result directional_partial_generialize(const thompson_digraph_type& pattern_graph, Iterator begin, Iterator end, vertex_type start, std::size_t ref_id) {
        vertex_type last = start;
        std::size_t length = 0;
        auto sit    = begin;
        auto send   = end;
        auto presit = sit; // garunteed to be valid always
        
        std::size_t debug_base_id = pattern_graph[last]._id;
        
        std::size_t base_progress = 0;
        std::size_t ref_progress  = 0;
        
        std::vector<generialization_result::edge_intent> edges;
        
        std::size_t tconsumed = 0;
        std::vector<prova::loga::wrapped> token_buffer;
        for(;;) {
            // advance lookahead
            auto [ei, ei_end] = boost::out_edges(last, pattern_graph);
            if(ei == ei_end) {                                                  // exit: [base exhausted] no edge to follow
                // return last;
                generialization_result res{last, sit, tconsumed, std::make_pair(base_progress, ref_progress)};
                res.etrace = std::move(edges);
                return res;
            }
            edge_type e;
            bool edge_found = false;
            for(auto [ei, ei_end] = boost::out_edges(last, pattern_graph); ei != ei_end; ++ei) {
                const segment_edge& ep = pattern_graph[*ei];
                if(ep._id == pattern_graph[start]._id){
                    e = *ei;
                    edge_found = true;
                    break;
                }
            }
            assert(edge_found);
            base_progress++;
            
            vertex_type v = boost::target(e, pattern_graph);
            const segment_edge&   ep = pattern_graph[e];
            const segment_vertex& vp = pattern_graph[v];
            
            if (vp._finish) {
                if (ep._type == segment_edge::epsilon) {
                    {
                        // auto [ne, nins] = boost::add_edge(last, v, pattern_graph);
                        // assert(nins);
                        // pattern_graph[ne]._type = segment_edge::epsilon;
                        // pattern_graph[ne]._id   = ref_id;
                        generialization_result::edge_intent e;
                        e.source     = last;
                        e.target     = v;
                        e.edge._id   = ref_id;
                        e.edge._type = segment_edge::epsilon;
                        edges.emplace_back(std::move(e));
                    }
                    // return v;
                    generialization_result res{v, sit, tconsumed, std::make_pair(base_progress, ref_progress)};           // exit: [constant ending in base]
                    res.etrace = std::move(edges);
                    return res;
                } else if (ep._type == segment_edge::placeholder) {
                    if(sit != send){
                        // if tconsumed > 0 then
                        // take the tokens that have been captured already
                        // token_buffer is cleared in thend of segment loop
                        std::size_t placeholders_consumed = 0;
                        if(sit->tag() == prova::loga::zone::placeholder) {
                            ++placeholders_consumed;
                        } else {
                            auto rest_current_begin = sit->tokens().begin();
                            auto rest_current_end   = sit->tokens().end();
                            std::advance(rest_current_begin, tconsumed);
                            for(auto tmp_it = rest_current_begin; tmp_it != rest_current_end; ++tmp_it) {
                                token_buffer.push_back(*tmp_it);
                            }
                        }
                        
                        auto tmp_sit = sit;
                        auto tmp_sit_last = tmp_sit;
                        while(++tmp_sit != send) {
                            if(tmp_sit->tag() == prova::loga::zone::placeholder) {
                                ++placeholders_consumed;
                            } else {
                                auto rest_next_begin = tmp_sit->tokens().begin();
                                auto rest_next_end   = tmp_sit->tokens().end();
                                for(auto tmp_it = rest_next_begin; tmp_it != rest_next_end; ++tmp_it) {
                                    token_buffer.push_back(*tmp_it);
                                }
                                tmp_sit_last = tmp_sit;
                            }
                        }
                        
                        if(token_buffer.size() > 0 || placeholders_consumed > 0) {
                            {
                                // auto [ne, nins] = boost::add_edge(last, v, pattern_graph);
                                // assert(nins);
                                // pattern_graph[ne]._type = segment_edge::placeholder;
                                // pattern_graph[ne]._id   = ref_id;
                                // for(auto& token: token_buffer) {
                                //     pattern_graph[ne]._captured.emplace_back(token);
                                // }
                                generialization_result::edge_intent e;
                                e.source     = last;
                                e.target     = v;
                                e.edge._id   = ref_id;
                                e.edge._type = segment_edge::placeholder;
                                for(auto& token: token_buffer) {
                                    e.edge._captured.emplace_back(token);
                                }
                                token_buffer.clear();
                                edges.emplace_back(std::move(e));
                            }
                            // return v;
                            generialization_result res{v, tmp_sit_last, tmp_sit_last->tokens().count(), std::make_pair(base_progress, ref_progress)};     // exit: [placeholder ending in base satisfied by ref]
                            res.etrace = std::move(edges);
                            return res;
                        }
                        
                        // return last;
                        generialization_result res{last, sit, tconsumed, std::make_pair(base_progress, ref_progress)};                                    // exit: [placeholder ending in base but ref semi-exhausted]
                        res.etrace = std::move(edges);
                        return res;
                    } else {
                        // return last;
                        generialization_result res{last, sit, tconsumed, std::make_pair(base_progress, ref_progress)};                                    // exit: [placeholder ending in base but ref exhausted]
                        res.etrace = std::move(edges);
                        return res;
                    }
                } else {
                    assert(ep._type != segment_edge::constant);
                }
            }
            
            bool placeholder_hop = ep._type == segment_edge::placeholder;
            const std::string& lookahead = vp._str;
            
            bool advance_lookahead = false;
            while(sit != send) {    // segment loop
                const prova::loga::pattern_sequence::segment& s = *sit;
                prova::loga::zone zone = s.tag();
                const prova::loga::tokenized& tokens = s.tokens();
                
                // if placeholder_hop then
                //      if zone is constant
                //          a subset of the tokens in the segment will align to the placeholder unless a token matches with the lookahead
                //          loop t into segment.tokens
                //              if t == lookahead
                //                  advance lookahead
                //              else
                //                  since this is a placeholder_hop check the next token because the unmatched token could be eaten by a the placeholder
                //      else
                //          the tokens in the placeholder zone doesn't have a text to match against the lookahead
                //          So, we have to skip all tokens in the current segment
                //          The lookahead is still the same
                //          next segment's zone is expected to be constant (because pseq is [c][p][c][p]...)
                // else
                //      we know exactly what we are expecting (the lookahead)
                //      if the first token does not match with lookahead
                //          thats the end of simulation
                //      else
                //          we need to advance the lookahead vertex as well as the token
                
                auto tbegin = tokens.begin();
                auto tend   = tokens.end();
                auto tit    = tbegin;
                // resume tit iteration
                if(tconsumed < tokens.count()) {
                    std::advance(tit, tconsumed);
                } else {
                    presit = sit;
                    ++sit;
                    ++ref_progress;
                    tconsumed = 0;
                    continue;
                }
                
                if(placeholder_hop) {
                    if(zone == prova::loga::zone::constant) {
                        bool matched = false;
                        for(;tit != tend; ++tit) {
                            bool lokahead_matched = (lookahead == tit->view());
                            if(lokahead_matched) {
                                {
                                    // auto [ne, nins] = boost::add_edge(last, v, pattern_graph);
                                    // assert(nins);
                                    // pattern_graph[ne]._type = ep._type;
                                    // pattern_graph[ne]._id   = ref_id;
                                    // for(auto& token: token_buffer) {
                                    //     pattern_graph[ne]._captured.emplace_back(token);
                                    // }
                                    generialization_result::edge_intent e;
                                    e.source     = last;
                                    e.target     = v;
                                    e.edge._id   = ref_id;
                                    // e.edge._str   = lookahead; // not collected because we favour _captured in this case
                                    e.edge._type = segment_edge::placeholder;
                                    for(auto& token: token_buffer) {
                                        e.edge._captured.emplace_back(token);
                                    }
                                    token_buffer.clear();
                                    edges.emplace_back(std::move(e));
                                }
                                last = v;
                                tconsumed++;
                                matched = true;
                                break;
                            } else {
                                token_buffer.push_back(*tit);
                                tconsumed++;
                                // hope until we reach tend
                            }
                        }
                        // given that the next segment will be placeholder zone
                        // if loop continues till here (without breaking) then this is the furthest we can reach
                        if(!matched){
                            assert(tit == tend);
                            // next segment is placeholder zone, next next is constant zone again
                            // if advancing sit twice failes then last iterator that we could visit is presit
                            presit = sit;
                            if (++sit == send) {
                                // return last;
                                generialization_result res{last, presit, presit->tokens().count(), std::make_pair(base_progress, ref_progress)};          // exit: [base placeholder + ref exhausted while looking for lookahead]
                                res.etrace = std::move(edges);
                                return res;
                            }
                            // next next segment would be constant zone again
                            tconsumed = 0;
                            if (++sit == send) {
                                // return last;
                                generialization_result res{last, presit, presit->tokens().count(), std::make_pair(base_progress, ref_progress)};          // exit: [base placeholder + ref exhausted while looking for lookahead]
                                res.etrace = std::move(edges);
                                return res;
                            }
                            advance_lookahead = false;
                            continue;
                        } else {
                            advance_lookahead = true;
                            // sit is not advanced
                            // we need a new lookahead
                            // tconsumed will be used for [resume tit iteration] block to choose whether to advance segment or not
                            // break the segment loop to first reach [advance lookahead] block due to advance_lookahead being true and then [resume tit iteration]
                            break;
                        }
                    } else {
                        advance_lookahead = false;
                        for(;tit != tend; ++tit) {
                            tconsumed++;
                        }
                        // no need to to sit++ because tconsumed reaches count which will lead to sit++ upon continue
                        continue; // next segment is constant
                    }
                } else {
                    if(zone == prova::loga::zone::constant) {
                        bool matched = false;
                        for(;tit != tend; ++tit) {
                            bool lokahead_matched = (lookahead == tit->view());
                            if(lokahead_matched) {
                                {
                                    // auto [ne, nins] = boost::add_edge(last, v, pattern_graph);
                                    // assert(nins);
                                    // pattern_graph[ne]._type = ep._type;
                                    // pattern_graph[ne]._id   = ref_id;
                                    generialization_result::edge_intent e;
                                    e.source     = last;
                                    e.target     = v;
                                    e.edge._id   = ref_id;
                                    e.edge._type = ep._type;
                                    // e.edge._str   = lookahead; // not collected because it is obvious
                                    edges.emplace_back(std::move(e));
                                }
                                last = v;
                                tconsumed++;
                                matched = true;
                                break;
                            } else {
                                // not a placeholder_hop
                                // lookahead could not be matched
                                break;
                            }
                        }
                        // given that the next segment will be placeholder zone
                        // if loop continues till here (without breaking) then this is the furthest we can reach
                        if(!matched) {
                            // return last;
                            generialization_result res{last, sit, tconsumed, std::make_pair(base_progress, ref_progress)};                         // exit: [base constant + ref unmatched]
                            res.etrace = std::move(edges);
                            return res;
                        } else {
                            advance_lookahead = true;
                            break; // break the segment loop because we have to change the lookahead now
                        }
                    } else {
                        // tit->view() doesn't work here because we are in a placeholder segment
                        // If the placeholder segment of the pseq generializes the constant segment of the linear graph
                        // then that would be detected while going through the next segment
                        if(debug_base_id == 1 && ref_id == 9)
                            std::cout << "2CP" << std::endl;
                        last = v;
                        advance_lookahead = true;
                        for(;tit != tend; ++tit) {
                            tconsumed++;
                        }
                        break; // next segment is constant
                    }
                }
            }
            if(!advance_lookahead){
                // return last;
                generialization_result res{last, presit, presit->tokens().count(), std::make_pair(base_progress, ref_progress)};                            // exit: [base constant + ref unmatched]
                res.etrace = std::move(edges);
                return res;
                // presit must be valid since sit is valid so dereferencing must be okay
            }
            token_buffer.clear(); // clear because we are now changing the lookahead
        }
        
        // return last;
        generialization_result res{last, presit, presit->tokens().count(), std::make_pair(base_progress, ref_progress)};
        res.etrace = std::move(edges);
        return res;
    }
    
    static std::size_t apply_trace(thompson_digraph_type& graph, const generialization_result& res);
    
    void directional_subset_generialize(thompson_digraph_type& pattern_graph, const prova::loga::pattern_sequence& pseq, std::size_t base_id, std::size_t ref_id);
    
    prova::loga::pattern_sequence merge(std::size_t id, const std::set<std::size_t>& members, bool bidirectional = false) const;
};

}

#endif // PROVA_ALIGN_AUTOMATA_H
