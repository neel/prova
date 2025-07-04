#include "prova/trace_parser.h"
#include <mlpack/methods/dbscan/dbscan.hpp>
#include <mlpack/methods/neighbor_search/neighbor_search.hpp>
#include <spoa/spoa.hpp>
#include <boost/asio.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <nlohmann/json.hpp>
#include <fstream>

// std::ostream& operator<<(std::ostream& stream, const trace_parser::chunk_chain& chain) {
//     stream << "[" << chain.matched() << "] ";
//     for(const auto& chunk : chain) {
//         if(chunk.matched) {
//             stream << chunk.content;
//         } else {
//             stream << "⎨" << chunk.content << "⎬";
//         }
//     }

//     if(chain._multiple) {
//         std::size_t placeholders_count = 0;
//         stream << std::endl << "placeholders: " << std::endl;
//         for(const auto& chunk : chain) {
//             if(!chunk.matched) {
//                 std::cout << std::endl << std::format("placeholder {} {{{} values}} [{} -> {}]", placeholders_count, chunk.possibilities.size(), chunk.limits.first, chunk.limits.second) << std::endl;
//                 for(const std::string& p: chunk.possibilities){
//                     std::cout << p << std::endl;
//                 }

//                 placeholders_count++;
//             }
//         }
//     }
//     return stream;
// }

// std::size_t trace_parser::chunk_chain::matched() const {
//     std::size_t n = 0;
//     for(const auto& chunk : _chain) {
//         if(chunk.matched) {
//             n += chunk.content.size();
//         }
//     }
//     return n;
// }

void trace_parser::parse(const std::filesystem::path &path){
    std::ifstream in(std::filesystem::path(path), std::ios::in);
    if (!in)
        throw std::runtime_error("cannot open " + path.string());

    std::string line;
    while (std::getline(in, line)){
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (!line.empty())
            append(line);
    }
}

void trace_parser::append(const string_type &str){
    _dataset.push_back(string_entry(str));
}

trace_parser &trace_parser::operator<<(const string_type &str) {
    append(str);
    return *this;
}

void trace_parser::compute(std::size_t threads, bool score_only){
    std::size_t const n = _dataset.size();
    _distances.set_size(n, n);
    _distances.zeros();

    const auto& ra_index = _dataset.template get<by_text>();

    unsigned int T = !threads ? std::thread::hardware_concurrency() : threads;

    std::cout << "Clustering " << std::endl;
    std::atomic_uint32_t jobs_completed = 0;
    boost::asio::thread_pool pool(T);
    for (std::size_t i = 0; i < n; ++i) {
        boost::asio::post(pool, [this, i, n, score_only, &jobs_completed]() {
            for (std::size_t j = i+1; j < n; ++j) {
                _distances(i, j) = distance(i, j);
                _distances(j, i) = _distances(i,j);
            }
            jobs_completed++;
            std::cout << std::format("\rDistance Matrix rows {}/{}", jobs_completed.load(), n);
        });
    }
    std::cout << std::endl << std::endl;

    pool.join();
    _computed = true;
}

double trace_parser::distance(std::size_t i, std::size_t j, bool score_only) const{
    if(i == j) {
        return 0.0; // Distance is 0 for identical strings
    } else if(_computed) {
        return _distances(i,j);
    } else {
        double score;
        const auto& ra_index = _dataset.template get<0>();
        chain_type alignment = align(ra_index[i].text, ra_index[j].text, &score);

        if(score_only) {
            return score;
        } else {
            // Count matched characters (similar to difflib.SequenceMatcher)
            std::size_t matched_chars = 0;
            for(const auto& chunk : alignment) {
                if(chunk.first) { // This is a match
                    matched_chars += chunk.second.size();
                }
            }

            // Calculate distance as 1 - similarity
            // Similarity = matched_chars / max_matched_chars
            // Distance = 1 - similarity (so distance is in [0, 1])
            std::size_t max_matched_chars = std::min(ra_index[i].text.size(), ra_index[j].text.size());
            if(max_matched_chars > 0) {
                double similarity = static_cast<double>(matched_chars) / static_cast<double>(max_matched_chars);
                return 1.0 - similarity;
            } else {
                return 0.0; // Both strings are empty, distance is 0
            }
        }
    }
}

trace_parser::chain_type trace_parser::align(const string_type &lhs, const string_type &rhs, double *alignment_score) const {
    if(lhs.empty() && rhs.empty()) {
        if(alignment_score) *alignment_score = 0.0;
        return chain_type{};
    }
    if(lhs.empty()) {
        if(alignment_score) *alignment_score = -static_cast<int>(rhs.size());
        chain_type chain;
        chain.emplace_back(false, chunk_type{0, rhs.size()});
        return chain;
    }
    if(rhs.empty()) {
        if(alignment_score) *alignment_score = -static_cast<int>(lhs.size());
        chain_type chain;
        chain.emplace_back(false, chunk_type{0, lhs.size()});
        return chain;
    }

    auto alignment_engine = spoa::AlignmentEngine::Create(spoa::AlignmentType::kNW, /*match*/1, /*mismatch*/-1, /*gap*/-1);
    spoa::Graph graph{};

    {
        auto alignment = alignment_engine->Align(lhs, graph);
        graph.AddAlignment(alignment, lhs);
    } {
        auto alignment = alignment_engine->Align(rhs, graph);
        graph.AddAlignment(alignment, rhs);
    }

    auto msa = graph.GenerateMultipleSequenceAlignment();
    const std::string& r0 = msa[0];
    const std::string& r1 = msa[1];
    const std::size_t   L = r0.size();

    auto is_match = [](char a, char b){ return a == b && a != '-'; };

    std::string buffer;
    bool last_match = is_match(r0[0], r1[0]);
    std::size_t last_flip = 0, col = 0;

    int score = 0;

    trace_parser::chain_type out;
    for (std::size_t i = 0; i < L; ++i) {
        char a = r0[i], b = r1[i];

        bool matched = is_match(a, b);

        if (alignment_score)
            score +=  matched ? 1 : (a=='-' || b=='-') ? -1 : -1;

        if (matched != last_match) {
            if(col > last_flip) {
                out.emplace_back(last_match, chunk_type{last_flip, col});
                buffer.clear();
            }
            last_flip = col;
            if(a != '-') buffer.push_back(a);
        } else {
            if(a != '-') buffer.push_back(a);
        }

        last_match = matched;
        if(a != '-') ++col;
    }

    out.emplace_back(last_match, chunk_type{last_flip, col});

    if (alignment_score) *alignment_score = static_cast<double>(score);
    return out;
}


std::size_t trace_parser::cluster(double eps, std::size_t minPts) {
    if (!_computed) throw std::logic_error("compute() first");

    arma::rowvec idx(_distances.n_cols);
    for (arma::uword i = 0; i < idx.n_elem; ++i)
        idx[i] = i;

    arma::mat coords(1, _distances.n_cols);
    for (arma::uword i = 0; i < coords.n_cols; ++i)
        coords(0, i) = static_cast<double>(i);

    mlpack::RangeSearch<Metric, arma::mat, mlpack::BallTree> rangeSearch(coords, true, false, Metric(this));
    mlpack::DBSCAN<mlpack::RangeSearch<Metric, arma::mat, mlpack::BallTree>, mlpack::RandomPointSelection> dbscan(eps, minPts, true, rangeSearch);
    arma::Row<std::size_t> labels;
    dbscan.Cluster(idx, labels);

    std::size_t cluster_count = 0;
    auto& ra = _dataset.get<0>();
    for (std::size_t i = 0; i < ra.size(); ++i) {
        ra.modify(ra.begin() +i, [&](auto& obj) {
            obj.cluster_id = static_cast<int>(labels[i]);
        });
        if (labels[i] + 1 > cluster_count)
            cluster_count = labels[i] + 1;
    }

    // --------------------------------------------------------------------
    // 3. ­Compute metrics -------------------------------------------------
    // --------------------------------------------------------------------
    const std::size_t n = labels.n_elem;
    const std::size_t NOISE = std::numeric_limits<std::size_t>::max();

    // -- helper lambdas ---------------------------------------------------
    auto averageDistance = [&](const std::vector<std::size_t>& ptsA, const std::vector<std::size_t>& ptsB) -> double {
        double sum = 0.0;
        std::size_t cnt = 0;
        for (std::size_t i : ptsA)
            for (std::size_t j : ptsB) {
                if (i == j) continue;
                sum += _distances(i, j);
                ++cnt;
            }
        return cnt ? sum / cnt : 0.0;
    };

    // Build cluster → indices map (ignore noise)
    std::vector<std::vector<std::size_t>> clusters(cluster_count);
    std::size_t noisePts = 0;
    for (std::size_t i = 0; i < n; ++i) {
        if (labels[i] == NOISE) ++noisePts;
        else                     clusters[labels[i]].push_back(i);
    }

    // --- Silhouette ------------------------------------------------------
    double silhouetteSum = 0.0;
    std::size_t silhouetteCnt = 0;

    for (std::size_t c = 0; c < cluster_count; ++c) {
        const auto& ptsC = clusters[c];
        if (ptsC.size() < 2) continue;           // a(i)=0, ignore trivial clusters
        for (std::size_t i : ptsC) {
            // a(i): avg distance to own cluster
            double a = averageDistance({i}, ptsC);

            // b(i): smallest avg distance to another cluster
            double b = std::numeric_limits<double>::infinity();
            for (std::size_t d = 0; d < cluster_count; ++d) {
                if (d == c || clusters[d].empty()) continue;
                b = std::min(b, averageDistance({i}, clusters[d]));
            }

            if (std::isfinite(b) && std::max(a, b) > 0.0) {
                silhouetteSum += (b - a) / std::max(a, b);
                ++silhouetteCnt;
            }
        }
    }
    double silhouette = silhouetteCnt ? silhouetteSum / silhouetteCnt : std::numeric_limits<double>::quiet_NaN();

    std::cout << "silhouette: " << silhouette << std::endl;

    _clustered = true;
    return cluster_count;
}

void trace_parser::save(const std::filesystem::path& dir){
    std::filesystem::create_directories(dir);

    if(_computed) {
        _distances.save((dir / "distances.bin").string(), arma::arma_binary);
    }

    if(_clustered) {
        nlohmann::json index  = nlohmann::json::array();
        const auto& byLabel   = _dataset.get<by_cluster>();

        for (auto beg = byLabel.lower_bound(0); beg != byLabel.end(); ) {
            int cid        = beg->cluster_id;
            auto end       = byLabel.upper_bound(cid);
            std::size_t n  = std::distance(beg, end);

            std::ostringstream fname;
            fname << cid << ".cluster.txt";
            std::ofstream ofs(dir / fname.str());

            for (auto it = beg; it != end; ++it)
                ofs << it->text << '\n';

            index.push_back({
                {"id",    cid},
                {"path",  (dir / fname.str()).string()},
                {"count", n}
            });

            beg = end;
        }

        std::ofstream idx(dir / "index.json");
        idx << index.dump(2);
    }
}

void trace_parser::load(const std::filesystem::path& dir) {
    _distances.load((dir / "distances.bin").string(), arma::arma_binary);

    _computed = true;

    nlohmann::json index;
    {
        std::ifstream js(dir / "index.json");
        if (js){
            js >> index;
        }
    }

    std::size_t clusters = 0;
    for (const auto& entry : index) {
        int  cid     = entry.at("id").get<int>();
        std::filesystem::path relPath = entry.at("path").get<std::filesystem::path>();

        std::ifstream txt(relPath);
        if (!txt)
            throw std::runtime_error("cannot open " + std::filesystem::absolute(relPath).string());

        auto& text_index = _dataset.get<by_text>();
        std::string line;
        while(std::getline(txt, line)) {
            if(line.empty()) continue;

            auto it = text_index.find(line);
            if(it != text_index.end()) {
                // already present → update its cluster_id
                text_index.modify(it, [&](string_entry& e){
                    e.cluster_id = cid;
                });
            } else {
                // not present → insert a new entry
                _dataset.emplace_back(std::move(line), cid);
            }
        }
        ++clusters;
    }

    _clustered = clusters > 0;
}

std::ostream& trace_parser::print(std::ostream &stream) const {
    std::cout << std::format("N: {} C: {}", _dataset.size(), cluster_count()) << std::endl;

    const auto& ra = _dataset.template get<by_cluster>();
    for (const auto& item: ra) {
        stream << item.text  << " → " << item.cluster_id << std::endl;
    }
    stream.flush();
    return stream;
}

trace_parser::graph_type trace_parser::align(int i, std::vector<std::vector<zone>>& all_zones) const {
    std::size_t N = cluster_count(i);
    std::cout << std::format("Cluster {} size {}", i, N) << std::endl;

    auto alignment_engine = spoa::AlignmentEngine::Create(spoa::AlignmentType::kNW, /*match*/1, /*mismatch*/-1, /*gap*/-1);
    spoa::Graph graph{};

    auto range = cluster_range(i);
    for (auto it = range.first; it != range.second; ++it) {
        auto alignment = alignment_engine->Align(it->text, graph);
        graph.AddAlignment(alignment, it->text);
    }
    auto msa = graph.GenerateMultipleSequenceAlignment();

    const std::size_t rows = N;
    const std::size_t cols = msa.front().size();

    auto group = [&msa, rows] (std::size_t col_begin, std::size_t col_end) -> std::vector<std::string> {
        std::vector<std::string> possibilities;
        possibilities.resize(rows);

        for (std::size_t row = 0; row < rows; ++row) {
            for (std::size_t col = col_begin; col < col_end; ++col) {
                char c = msa[row][col];
                if(c != '-') {
                    possibilities[row].push_back(c);
                }
            }
        }
        return possibilities;
    };

    auto unique = [&msa, rows] (std::size_t col) -> std::size_t {
        std::set<char> possibilities;
        for (std::size_t row = 0; row < rows; ++row) {
            char c = msa[row][col];
            possibilities.insert(c);
        }
        return possibilities.size();
    };

    auto uniques = [&group, cols] (std::set<std::string>& unique_possibilities, std::size_t begin, std::size_t col) -> std::pair<std::size_t, std::size_t> {
        std::vector<std::string> possibilities = group(begin, col);
        std::copy(possibilities.begin(), possibilities.end(), std::inserter(unique_possibilities, unique_possibilities.end()));

        std::size_t min_len = cols, max_len = 0;
        for(const auto& p: unique_possibilities) {
            std::size_t length = p.size();
            if(length < min_len) {
                min_len = length;
            }

            if(length > max_len) {
                max_len = length;
            }
        }
        return std::make_pair(min_len, max_len);
    };

    auto populate = [&group, cols] (placeholder& p, std::size_t begin, std::size_t col) {
        std::vector<std::string> observed_values = group(begin, col);
        p._values = observed_values;
        std::copy(observed_values.begin(), observed_values.end(), std::inserter(p._unique_values, p._unique_values.end()));

        std::size_t min_len = cols, max_len = 0;
        for(const auto& p: p._unique_values) {
            std::size_t length = p.size();
            if(length < min_len) {
                min_len = length;
            }

            if(length > max_len) {
                max_len = length;
            }
        }
        p._range = std::make_pair(min_len, max_len);
    };

    // for(const auto& seq: msa){
    //     std::cout << seq << std::endl;
    // }

    all_zones.resize(rows);
    trace_parser::graph_type out;
    {
        std::string buffer;
        using chunk_type = trace_parser::graph_type::chunk_type;

        std::size_t nunique = unique(0);

        std::stack<std::size_t> placeholders;
        std::size_t placeholders_count = 0;

        bool last_match = (nunique == 1);
        if(!last_match) {
            placeholders.push(0);
            ++placeholders_count;
        }

        for (std::size_t col = 0; col < cols; ++col) {
            nunique = unique(col);
            bool matched = (nunique == 1);

            if(last_match == matched) {
                if(nunique == 1) {
                    buffer.push_back(msa[0][col]);
                }
            } else {
                if(last_match && !matched) {
                    if(!buffer.empty()) {
                        out.emplace_back(chunk_type{last_match, subsequence{buffer}});
                        buffer.clear();
                    }
                    if(nunique == 1) {
                        buffer.push_back(msa[0][col]);
                    }
                    placeholders.push(col);
                } else if(!last_match && matched) {
                    std::size_t begin = placeholders.top();
                    placeholders.pop();

                    placeholder p{placeholders_count, 0, 0};
                    populate(p, begin, col);

                    out.emplace_back(last_match, p);
                    buffer.clear();

                    if(nunique == 1) {
                        buffer.push_back(msa[0][col]);
                    }

                    placeholders_count++;
                }
            }

            last_match = matched;
        }

        if(!last_match) {
            std::size_t begin = placeholders.top();
            placeholders.pop();

            placeholder p{placeholders_count, 0, 0};
            populate(p, begin, cols-1);

            out.emplace_back(last_match, p);
            buffer.clear();

            if(nunique == 1) {
                buffer.push_back(msa[0][cols-1]);
            }

            placeholders_count++;
        } else {
            out.emplace_back(last_match, subsequence{buffer});
        }

        for (std::size_t row = 0; row < rows; ++row) {
            std::size_t nunique = unique(0);

            bool last_match = (nunique == 1);
            std::size_t last_change = 0;

            std::size_t gaps = 0;
            for (std::size_t col = 0; col < cols; ++col) {
                nunique = unique(col);
                bool matched = (nunique == 1);
                if(msa[row][col] == '-') {
                    ++gaps;
                    continue;
                }

                if(last_match != matched) {
                    all_zones[row].emplace_back(zone{last_match, (col-gaps)-last_change});
                    last_change = (col-gaps);
                }
                last_match = matched;
            }
            all_zones[row].emplace_back(zone{last_match, (cols-1-gaps)-last_change});

            for(std::size_t i = all_zones[row].size(); i < out.size(); ++i) {
                all_zones[row].emplace_back(zone{(out.begin()+i)->first, 0});
            }
        }
    }
    return out;
}

std::ostream &trace_parser::print_aligned(int cluster_id, std::ostream& stream, const std::vector<std::vector<zone>>& all_zones) const {
    auto range = cluster_range(cluster_id);

    std::size_t i = 0;
    for(auto it = range.first; it != range.second; ++it) {
        const std::vector<zone>& zones = all_zones.at(i);

        std::size_t pos = 0;
        for(const zone& z: zones) {
            const auto& txt = *it;
            if(!z.is_constant()) stream << "⎨";
            stream << txt.text.substr(pos, z.length());
            if(!z.is_constant()) stream << "⎬";
            pos += z.length();
        }
        stream << std::endl;

        ++i;
    }

    return stream;
}

void trace_parser::adjust(graph_type& malignment, std::vector<std::vector<zone>>& zones) {
    std::string alphabets = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_:/.";
    {
        // spreading placeholders
        std::size_t component_index = 0;
        for(auto it = malignment.begin(); it != malignment.end(); ++it) {
            auto& [matched, component] = *it;
            if(!matched) {
                assert(std::holds_alternative<placeholder>(component));
                placeholder& p = std::get<placeholder>(component);

                bool starts_with_alphabets = true, ends_with_alphabets = true;
                for(auto& v: p._values) {
                    if(starts_with_alphabets && !v.empty() && alphabets.find(v.front()) == std::string::npos){
                        starts_with_alphabets = false;
                    }
                    if(ends_with_alphabets && !v.empty() && alphabets.find(v.back()) == std::string::npos){
                        ends_with_alphabets = false;
                    }
                }

                if(it != malignment.begin() && starts_with_alphabets) {
                    auto& [_, previous_component] = *(it-1);
                    assert(std::holds_alternative<subsequence>(previous_component));
                    subsequence& pre_sub = std::get<subsequence>(previous_component);

                    auto pos = pre_sub._str.find_last_not_of(alphabets);
                    std::string::size_type start = (pos != std::string::npos) ? pos+1 : 0;

                    std::string left_over  = pre_sub._str.substr(0, start);
                    std::string carry_over = pre_sub._str.substr(start, pre_sub._str.size() - start);
                    pre_sub._str = left_over;

                    if(carry_over.size() > 0) {
                        p.glue_left(carry_over);
                    }

                    for(auto i = 0; i < zones.size(); ++i) {
                        assert(!zones[i][component_index].is_constant());
                        assert(zones[i][component_index-1].is_constant());

                        zones[i][component_index]._length   += carry_over.size();
                        zones[i][component_index-1]._length  = left_over.size();
                    }
                }

                if(it != malignment.end() -1 && ends_with_alphabets) {
                    auto& [_, next_component] = *(it+1);
                    assert(std::holds_alternative<subsequence>(next_component));
                    subsequence& next_sub = std::get<subsequence>(next_component);

                    auto pos = next_sub._str.find_first_not_of(alphabets);
                    std::string::size_type end = (pos != std::string::npos && pos > 0) ? pos : 0;

                    std::string left_over  = next_sub._str.substr(end, next_sub._str.size() - end);
                    std::string carry_over = next_sub._str.substr(0, end);
                    next_sub._str = left_over;

                    if(carry_over.size() > 0) {
                        p.glue_right(carry_over);
                    }

                    for(auto i = 0; i < zones.size(); ++i) {
                        assert(!zones[i][component_index].is_constant());
                        assert(zones[i][component_index+1].is_constant());

                        zones[i][component_index]._length   += carry_over.size();
                        zones[i][component_index+1]._length = left_over.size();
                    }
                }
            }
            ++component_index;
        }
    }

    // malignment.apply(std::cout);

    {
        // remove empty constant block
        trace_parser::graph_type modified_malignment;
        {
            std::stack<std::size_t> zones_to_be_deleted;
            {
                std::size_t component_index = 0;
                for(auto it = malignment.begin(); it != malignment.end(); ++it) {
                    auto& [matched, component] = *it;
                    if(matched) {
                        assert(std::holds_alternative<subsequence>(component));
                        subsequence& sub = std::get<subsequence>(component);
                        if(sub.size() > 0) {
                            modified_malignment.emplace_back(matched, std::move(component));
                        } else {
                            zones_to_be_deleted.push(component_index);
                        }
                    } else {
                        modified_malignment.emplace_back(matched, std::move(component));
                    }

                    ++component_index;
                }
            }
            while(!zones_to_be_deleted.empty()){
                for(auto i = 0; i < zones.size(); ++i) {
                    std::size_t zone_id = zones_to_be_deleted.top();
                    assert(zone_id < zones[i].size());
                    assert(zones[i][zone_id].is_constant());
                    zones[i].erase(zones[i].begin()+zone_id);
                }
                zones_to_be_deleted.pop();
            }
            assert(zones_to_be_deleted.empty());
        }
        malignment = modified_malignment;
        modified_malignment.clear();

        // merge consecutive placeholders
        std::vector<std::vector<zone>> modified_zones;
        modified_zones.resize(zones.size());
        for(auto it = malignment.begin(); it != malignment.end(); ++it) {
            auto& [matched, component] = *it;
            if(!matched) {
                assert(std::holds_alternative<placeholder>(component));
                placeholder& p = std::get<placeholder>(component);

                std::size_t growth = 0;
                auto jt = it+1;
                for(; jt != malignment.end(); ++jt) {
                    auto& [_, next_component] = *(jt);
                    if(std::holds_alternative<placeholder>(next_component)) {
                        placeholder& next_placeholder = std::get<placeholder>(next_component);
                        // merge with p
                        growth++;
                        p.merge(next_placeholder);
                    } else {
                        break;
                    }
                }
                modified_malignment.emplace_back(matched, placeholder(p));

                std::size_t c_index = std::distance(malignment.begin(), it);
                for(auto i = 0; i < modified_zones.size(); ++i) {
                    zone z = zones[i][c_index];
                    std::size_t expanded = 0;
                    for(auto j = c_index+1; j <= c_index+growth; ++j){
                        expanded += zones[i][j].length();
                    }
                    z._length += expanded;
                    modified_zones[i].push_back(z);
                }
                it = jt-1;
            } else {
                assert(std::holds_alternative<subsequence>(component));
                subsequence& s = std::get<subsequence>(component);
                modified_malignment.emplace_back(matched, std::move(component));

                std::size_t c_index = std::distance(malignment.begin(), it);
                for(auto i = 0; i < modified_zones.size(); ++i) {
                    zone z = zones[i][c_index];
                    modified_zones[i].push_back(z);
                }
            }

        }
        malignment = modified_malignment;
        zones = modified_zones;
    }
}

void trace_parser::save_alignments(const std::filesystem::path& dir, int cluster_id, const graph_type &malignment, const std::vector<std::vector<zone> > &all_zones) {
    nlohmann::json alignment_info = nlohmann::json::object();

    alignment_info["cluster"] = cluster_id;
    alignment_info["length"]  = malignment.matched();
    alignment_info["chunks"]  = nlohmann::json::array();

    for(const auto& chunk : malignment) {
        if(chunk.first) {
            assert(std::holds_alternative<subsequence>(chunk.second));
            const subsequence& sub = std::get<subsequence>(chunk.second);
            alignment_info["chunks"].emplace_back(nlohmann::json::object({
                {"matched", true},
                {"content", sub.str()},
                {"size", sub.size()}
            }));
        } else {
            assert(std::holds_alternative<placeholder>(chunk.second));
            const placeholder& p = std::get<placeholder>(chunk.second);
            alignment_info["chunks"].emplace_back(nlohmann::json::object({
                {"matched", false},
                {"id", p.id()},
                {"range", {p._range.first, p._range.second}},
                {"uniques", p._unique_values},
                {"size", p.size()}
            }));
        }
    }
    std::ofstream alignment_fs(dir / std::format("{}.alignment.json", cluster_id));
    alignment_fs << alignment_info.dump(2);

    std::ofstream stream(dir / std::format("{}.aligned.log", cluster_id));
    auto range = cluster_range(cluster_id);
    std::size_t i = 0;
    for(auto it = range.first; it != range.second; ++it) {
        const std::vector<zone>& zones = all_zones.at(i);
        std::size_t pos = 0;
        for(const zone& z: zones) {
            const auto& txt = *it;
            if(!z.is_constant()) stream << "⎨";
            stream << txt.text.substr(pos, z.length());
            if(!z.is_constant()) stream << "⎬";
            pos += z.length();
        }
        stream << std::endl;

        ++i;
    }
}


