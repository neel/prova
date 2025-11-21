#include <loga/automata.h>

void prova::loga::automata::build() {
    for(std::size_t i = 0; i < _pseqs.size(); ++i) {
        const prova::loga::pattern_sequence& pseq = _pseqs.at(i);
        terminal_pair_type terminals = automata::thompson_graph(_graph, pseq, i);
        _terminals[i] = terminals;
    }
}

std::ostream &prova::loga::automata::graphml(std::ostream &stream){
    auto vertex_label_map = boost::make_function_property_map<vertex_type>(
        [&](const vertex_type& v) -> std::string {
            return std::format("{}", _graph[v]._id);
        }
        );

    auto edge_label_map = boost::make_function_property_map<edge_type>(
        [&](const edge_type& e) -> std::string {
            if(_graph[e]._type == segment_edge::placeholder) return "$";
            if(_graph[e]._type == segment_edge::epsilon)     return "ε";
            return _graph[e]._str;
        }
        );

    auto edge_group_map = boost::make_function_property_map<edge_type>(
        [&](const edge_type& e) -> std::size_t {
            return _graph[e]._id;
        }
        );

    boost::dynamic_properties properties;
    properties.property("label", vertex_label_map);
    properties.property("label", edge_label_map);
    properties.property("type",  edge_group_map);

    boost::write_graphml(stream, _graph, properties, true);

    return stream;
}

std::ostream &prova::loga::automata::graphviz(std::ostream &stream){
    struct color_palette {
        static const std::vector<std::string>& colors() {
            static const std::vector<std::string> COLORS = {
                "red","blue","green","orange","purple","brown","cyan","magenta","gold",
                "darkgreen","darkorange","deepskyblue","limegreen","chocolate","indigo",
                "darkgoldenrod","dodgerblue","coral","orchid","olivedrab","steelblue",
                "rosybrown","slateblue","teal","peru","cadetblue","mediumseagreen",
                "lightsalmon","darkkhaki","mediumorchid","mediumslateblue","firebrick",
                "deeppink","navy","forestgreen","darkturquoise","maroon","darkolivegreen",
                "midnightblue","saddlebrown","darkred","darkmagenta","darkblue"
            };
            return COLORS;
        }
        static std::string for_id(std::size_t id){
            const auto& c = colors();
            return c[id % c.size()];
        }
    };

    struct vertex_writer {
        const thompson_digraph_type* g;
        void operator()(std::ostream& out, const vertex_type& v) const {
            const auto& gv = (*g)[v];
            const std::string color = color_palette::for_id(gv._id);
            std::string label = std::format("{}/{}", gv._id, (gv._str == " " ? std::string("□") : gv._str));
            if(gv._start) {
                label = std::format("^{}", gv._id);
            } else if(gv._finish) {
                label = std::format("{}$", gv._id);
            }
            out << "[label=\""   << label
                << "\", shape=\"box\""          // rectangular node
                << ", color=\""  << color       // border color
                << "\", penwidth=2"             // make border visible
                << ", fontname=\"Helvetica\"]";
        }
    };
    struct edge_writer {
        const thompson_digraph_type* g;
        void operator()(std::ostream& out, const edge_type& e) const {
            const auto& ge = (*g)[e];
            vertex_type u = boost::source(e, *g);
            const auto& up = (*g)[u];

            std::string lab;
            if (ge._type == segment_edge::placeholder){
                if(ge._id == up._id) {
                    lab = "$";
                } else {
                    std::string captured;
                    for(const auto& w: ge._captured) {
                        captured.append(w.view());
                    }
                    lab = captured;
                }
            }
            else if (ge._type == segment_edge::epsilon) lab = "ε";
            else lab = ge._str;

            std::string style;
            switch (ge._type) {
            case segment_edge::placeholder: style = "dashed"; break;
            case segment_edge::epsilon:     style = "dotted"; break;
            default:                        style = "solid";  break;
            }

            out << "[label=\"" << (lab == " " ? std::string("□") : lab)
                << "\", color=\"" << color_palette::for_id(ge._id)
                << "\", style=\"" << style
                << "\", penwidth=2]";
        }
    };
    struct graph_writer {
        void operator()(std::ostream& out) const {
            out << "graph [splines=true, overlap=false];\n"
                << "node  [shape=circle, fontname=\"Helvetica\"];\n"
                << "edge  [fontname=\"Helvetica\"];\n";
        }
    };
    // --------------------------------------------------------------------

    boost::write_graphviz(
        stream,
        _graph,
        vertex_writer{&_graph},
        edge_writer{&_graph},
        graph_writer{}         // *** CHANGED: provide graph-writer here ***
        );
    return stream;
}

std::pair<prova::loga::automata::vertex_type, prova::loga::automata::vertex_type> prova::loga::automata::thompson_graph(thompson_digraph_type &graph, const prova::loga::pattern_sequence &pseq, std::size_t id) {
    vertex_type start = boost::add_vertex(graph);
    vertex_type last = start;
    graph[last]._start = true;
    graph[last]._id = id;
    std::size_t placeholders = 0;
    for(const prova::loga::pattern_sequence::segment& s: pseq) {
        prova::loga::zone zone = s.tag(); // either constant or placeholder
        if(zone == prova::loga::zone::constant) {
            for(const prova::loga::wrapped& t: s.tokens()) {
                vertex_type v = boost::add_vertex(graph);
                graph[v]._str = t.view();
                graph[v]._id = id;

                auto [e, inserted] = boost::add_edge(last, v, graph);
                graph[e]._type = (placeholders > 0) ? segment_edge::placeholder : segment_edge::constant;
                graph[e]._str  = graph[v]._str;
                graph[e]._id   = id;

                last = v;
                placeholders = 0;
            }
        } else {
            // placeholders are not typed such as \d+ or \w+ it is always consume all unless reaches a literal
            // so, no hope between two placeholder tokens is necessary
            // rather merge all the contigous placeholders into one hop described by a single edge btween two literals
            ++placeholders; // zero token placeholder is also valid
            for(const prova::loga::wrapped& t: s.tokens())
                ++placeholders;
        }
    }

    vertex_type finish = boost::add_vertex(graph);
    graph[finish]._finish = true;
    graph[finish]._id = id;
    auto [e, inserted] = boost::add_edge(last, finish, graph);
    graph[e]._type = (placeholders > 0) ? segment_edge::placeholder : segment_edge::epsilon;
    graph[e]._id   = id;
    return {start, finish};
}

prova::loga::automata::generialization_result prova::loga::automata::directional_partial_generialize(thompson_digraph_type &pattern_graph, const prova::loga::pattern_sequence &pseq, vertex_type start, std::size_t ref_id) {
    assert(pseq.size() > 0);

    auto sbegin = pseq.begin();
    auto send   = pseq.end();

    generialization_result result = formalized_directional_partial_generialize(pattern_graph, sbegin, send, start, ref_id);
    apply_trace(pattern_graph, result);
    return result;
}

std::size_t prova::loga::automata::apply_trace(thompson_digraph_type &graph, const generialization_result &res){
    std::size_t count = 0;
    for (const auto& s : res.etrace) {
        auto [ne, ok] = boost::add_edge(s.source, s.target, graph);
        (void)ok;
        assert(ok);

        graph[ne]._type = s.edge._type;
        graph[ne]._str  = s.edge._str;
        graph[ne]._id   = s.edge._id;
        for(auto& token: s.edge._captured) {
            graph[ne]._captured.emplace_back(token);
        }
        ++count;
    }
    return count;
}

void prova::loga::automata::directional_subset_generialize(thompson_digraph_type &pattern_graph, const prova::loga::pattern_sequence &pseq, std::size_t base_id, std::size_t ref_id) {
    assert(pseq.size() > 0);

    // [[incomplete]]

    const vertex_type base_start  = _terminals.at(base_id).first;
    const vertex_type base_finish = _terminals.at(base_id).second;

    std::map<vertex_type, std::size_t> map_v_id;
    std::map<std::size_t, vertex_type> map_id_v;
    std::map<generialization_result::segment_iterator, std::size_t> map_ref_id;
    std::map<std::size_t, generialization_result::segment_iterator> map_id_ref;

    // TODO populate four maps;
    {
        vertex_type v = base_start;
        std::size_t idx = 0;
        for (;;) {
            map_v_id.emplace(v, idx);
            map_id_v.emplace(idx, v);

            if (v == base_finish) break;

            // Find the unique next edge along this base chain (_id == base_id)
            edge_type next_e{};
            bool      found = false;
            for (auto [ei, ei_end] = boost::out_edges(v, pattern_graph); ei != ei_end; ++ei) {
                const segment_edge& ep = pattern_graph[*ei];
                if (ep._id == base_id) {
                    next_e = *ei;
                    found  = true;
                    break;
                }
            }
            assert(found && "Base chain is expected to be linear for this id");

            v = boost::target(next_e, pattern_graph);
            ++idx;
        }
    }

    // Reference sequence: assign a dense index to each segment iterator.
    {
        std::size_t idx = 0;
        for (auto it = pseq.begin(); it != pseq.end(); ++it, ++idx) {
            map_ref_id.emplace(it, idx);
            map_id_ref.emplace(idx, it);
        }
    }

    arma::imat memo;
    memo.set_size(map_v_id.size(), map_ref_id.size());

    vertex_type start = base_start;
    auto sbegin = pseq.begin();
    auto send   = pseq.end();

    vertex_type finish  = _terminals.at(base_id).second;

    std::size_t score = 0;
    bool base_finished = false;
    while(sbegin < send) {
        generialization_result result = directional_partial_generialize(pattern_graph, sbegin, send, start, ref_id);
        if(result.last_it == send){
            if(result.last_v == finish) {
                base_finished = true;
            }
            ++score;
            break;
        }
        memo(map_v_id.at(start), map_ref_id.at(sbegin)) = score;
        sbegin = result.last_it;
        ++sbegin;
    }
}

prova::loga::pattern_sequence prova::loga::automata::merge(std::size_t id, bool bidirectional) const {
    vertex_type start  = _terminals.at(id).first;
    vertex_type finish = _terminals.at(id).second;

    prova::loga::pattern_sequence res;

    vertex_type last = start;
    do {
        edge_type base_e;
        std::vector<edge_type> ref_edges;

        std::size_t base_edge_found = 0;
        for(auto [ei, ei_end] = boost::out_edges(last, _graph); ei != ei_end; ++ei) {
            const segment_edge& ep = _graph[*ei];
            if(ep._id == id){
                base_e = *ei;
                base_edge_found++;
            } else {
                ref_edges.push_back(*ei);
            }
        }
        assert(base_edge_found == 1);
        last = boost::target(base_e, _graph);

        if(ref_edges.size() == 0){
            break;
        }

        bool is_ref_placeholder = false;
        const segment_edge& base_edge_props = _graph[base_e];
        if(base_edge_props._type == segment_edge::placeholder) {
            // only if bidirectional is true
            // if base edge is a placeholder and every other reference edge is constant
            //   and the hash of these constants are same then
            //   the base is actually not a placeholder
            prova::loga::pattern_sequence::segment seg(prova::loga::zone::placeholder);
            res.add(std::move(seg));
        } else { // base edge is constant
            // if base edge is a constant and at least one reference edge is placeholder
            //    then the base is actually a placeholder
            std::set<segment_edge::type>     ref_edge_types;
            // std::map<std::size_t, edge_type> transition_tokens;
            for(edge_type re: ref_edges) {
                const segment_edge& re_props = _graph[re];
                ref_edge_types.insert(re_props._type);
                // if(re_props._type == segment_edge::placeholder) continue;
                // if(re_props._type == segment_edge::epsilon)     continue;
                // std::size_t captured_tokens_hash = 0;
                // for(const prova::loga::wrapped& w: re_props._captured){
                //     boost::hash_combine(captured_tokens_hash, w.hash());
                // }
                // transition_tokens.insert(std::make_pair(captured_tokens_hash, re));
            }

            if(ref_edge_types.contains(segment_edge::placeholder)) {
                is_ref_placeholder = true;
                prova::loga::pattern_sequence::segment seg(prova::loga::zone::placeholder);
                res.add(std::move(seg));
            }
        }

        // last has already been updated
        if(!is_ref_placeholder) {
            const auto& last_props = _graph[last];
            prova::loga::pattern_sequence::segment seg(prova::loga::zone::constant, last_props._str);
            res.add(std::move(seg));
        }
    } while(last != finish);

    return res;
}

prova::loga::automata::generialization_result::generialization_result(vertex_type v, segment_iterator it, progress_pair progress): last_v(v), last_it(it), tokens(0), base_progress(progress.first), ref_progress(progress.second) {}

prova::loga::automata::generialization_result::generialization_result(vertex_type v, segment_iterator it, std::size_t n, progress_pair progress): last_v(v), last_it(it), tokens(n), base_progress(progress.first), ref_progress(progress.second) {}

