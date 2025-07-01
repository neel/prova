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

void trace_parser::compute(bool score_only){
    std::size_t const n = _dataset.size();
    _distances.set_size(n, n);
    _distances.zeros();

    const auto& ra_index = _dataset.template get<0>();

    unsigned int T = std::thread::hardware_concurrency();

    std::atomic_uint32_t jobs_completed = 0;
    boost::asio::thread_pool pool(T);
    for (std::size_t i = 0; i < n; ++i) {
        boost::asio::post(pool, [this, i, n, score_only, &jobs_completed]() {
            for (std::size_t j = 0; j < n; ++j) {
                _distances(i,j) = distance(i, j);
            }
            jobs_completed++;
            std::cout << std::format("Distance Matrix rows {}/{}", jobs_completed.load(), n) << std::endl;
        });
    }

    pool.join();
    _computed = true;
}

double trace_parser::distance(std::size_t i, std::size_t j, bool score_only) const{
    if(i == j) {
        return 0.0; // Distance is 0 for identical strings
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
        ra.modify(ra.begin() + i, [&](auto& obj) {
            obj.cluster_id = static_cast<int>(labels[i]);
        });
        if (labels[i] + 1 > cluster_count)
            cluster_count = labels[i] + 1;
    }

    _clustered = true;
    return cluster_count;
}

void trace_parser::save(const std::filesystem::path& dir){
    if (!_clustered)
        throw std::logic_error("call cluster() first");

    std::filesystem::create_directories(dir);

    _distances.save((dir / "distances.bin").string(), arma::arma_binary);

    nlohmann::json index  = nlohmann::json::array();
    const auto& byLabel   = _dataset.get<1>();

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

void trace_parser::load(const std::filesystem::path& dir) {
    if (!_dataset.empty())
        throw std::logic_error("load(): _dataset must be empty");

    _distances.load((dir / "distances.bin").string(), arma::arma_binary);

    _computed = true;

    nlohmann::json index;
    {
        std::ifstream js(dir / "index.json");
        if (!js) throw std::runtime_error("missing index.json");
        js >> index;
    }

    for (const auto& entry : index) {
        int  cid     = entry.at("id").get<int>();
        std::filesystem::path relPath = entry.at("path").get<std::filesystem::path>();

        std::ifstream txt(relPath);
        if (!txt)
            throw std::runtime_error("cannot open " + std::filesystem::absolute(relPath).string());

        std::string line;
        while (std::getline(txt, line)) {
            if (line.empty()) continue;
            _dataset.emplace_back(string_entry(std::move(line), cid));
        }
    }
    _clustered = true;
}

std::ostream& trace_parser::print(std::ostream &stream) const {
    std::cout << std::format("N: {} C: {}", _dataset.size(), cluster_count()) << std::endl;

    const auto& ra = _dataset.template get<1>();
    for (const auto& item: ra) {
        stream << item.text  << " → " << item.cluster_id << std::endl;
    }
    stream.flush();
    return stream;
}

trace_parser::graph_type trace_parser::align(int i) const {
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

    std::string buffer;
    trace_parser::graph_type out;
    using chunk_type = trace_parser::graph_type::chunk_type;

    std::size_t nunique = unique(0);

    bool last_match = (nunique == 1);

    std::stack<std::size_t> placeholders;
    std::size_t placeholders_count = 0;

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

                std::set<std::string> unique_possibilities;
                auto [min_len, max_len] = uniques(unique_possibilities, begin, col);

                placeholder p{placeholders_count, min_len, max_len};
                p = unique_possibilities;

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

        std::set<std::string> unique_possibilities;
        auto [min_len, max_len] = uniques(unique_possibilities, begin, cols-1);

        placeholder p{placeholders_count, min_len, max_len};
        p = unique_possibilities;

        out.emplace_back(last_match, p);
        buffer.clear();

        if(nunique == 1) {
            buffer.push_back(msa[0][cols-1]);
        }

        placeholders_count++;
    } else {
        out.emplace_back(last_match, subsequence{buffer});
    }

    out._multiple = true;
    return out;
}

void trace_parser::align_all() const {
    std::size_t C = cluster_count();
    for(int i=0; i < C; ++i) {
        align(i);
    }
}
