#ifndef TRACE_PARSER_H
#define TRACE_PARSER_H

#include <boost/algorithm/string/join.hpp>
#include <list>
#include <string>
#include <iostream>
#include <filesystem>
#include <iostream>
#include <armadillo>
#include <variant>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/member.hpp>
#include <mlpack/core/tree/hrectbound.hpp>

struct sequence_partition{
    inline explicit sequence_partition(std::size_t begin = 0, std::size_t end = 0): _span(std::make_pair(begin, end)) {}
    sequence_partition(const sequence_partition&) = default;

    inline std::string_view apply(const std::string& input) const { return std::string_view(input).substr(_span.first, _span.second - _span.first); }
    inline std::size_t size() const { return _span.second - _span.first; }

    private:
        std::pair<std::size_t, std::size_t> _span;
};

struct subsequence{
    inline explicit subsequence(const std::string& str): _str(str) {}
    subsequence(const subsequence&) = default;

    const std::string& str() const { return _str; }
    inline std::size_t size() const { return _str.size(); }


    std::string _str;
};

struct zone{
    inline explicit zone(bool constant, std::size_t length): _constant(constant), _length(length) {}
    zone(const zone&) = default;


    bool is_constant() const { return _constant; }
    std::size_t length() const { return _length; }

    bool _constant;
    std::size_t _length;
};

struct placeholder{
    using collection_type = std::vector<std::string>;
    using unique_collection_type = std::set<std::string>;
    using const_iterator = typename unique_collection_type::const_iterator;
    using iterator = const_iterator;
    using size_type = typename collection_type::size_type;

    inline explicit placeholder(std::size_t id, std::size_t min, std::size_t max): _id(id), _range(std::make_pair(min, max)) {}
    placeholder(const placeholder&) = default;

    std::size_t id() const { return _id; }

    inline std::size_t size() const { return _range.second - _range.first; }

    inline const_iterator begin() const { return _unique_values.begin(); }
    inline const_iterator end() const { return _unique_values.end(); }
    inline size_type count() const { return _unique_values.size(); }

    inline void add(const std::string& value) {
        _values.push_back(value);
        _unique_values.insert(value);
    }

    inline placeholder& operator=(const collection_type& collection) {
        _values = collection;
        return *this;
    }

    std::string str() const {
        if(_range.first != _range.second)
            return std::format("{}/[{}:{}]", _id, _range.first, _range.second);
        else
            return std::format("{}/[{}]", _id, _range.first);
    }

    void merge(const placeholder& other) {
        assert(_values.size() == other._values.size());
        _unique_values.clear();
        std::size_t min = _range.second, max = _range.second;
        for(auto i = 0; i < _values.size(); ++i) {
            _values[i] += other._values[i];
            if(_values[i].size() > max) {
                max = _values[i].size();
            }
            if(_values[i].size() < min) {
                min = _values[i].size();
            }
            _unique_values.insert(_values[i]);
        }
        _range.first  = min;
        _range.second = max;
    }

    void glue_left(const std::string& str){
        _range.first += str.size();
        _range.second += str.size();

        _unique_values.clear();
        for(std::string& v: _values) {
            v = str+v;
            _unique_values.insert(v);
        }
    }

    void glue_right(const std::string& str){
        _range.first += str.size();
        _range.second += str.size();

        _unique_values.clear();
        for(std::string& v: _values) {
            v = v+str;
            _unique_values.insert(v);
        }
    }

    std::size_t _id;
    std::pair<std::size_t, std::size_t> _range;
    collection_type _values;
    unique_collection_type _unique_values;
};

using sequence_component = std::variant<subsequence, placeholder>;

template <typename ChunkT>
struct chunk_chain{
    using chunk_type = std::pair<bool, ChunkT>;
    using chain_type = std::list<chunk_type>;
    using const_iterator = typename chain_type::const_iterator;
    using size_type = typename chain_type::size_type;

    std::ostream& apply(std::ostream& stream, const std::string& input) const {
        stream << "[" << matched() << "] ";
        for(const auto& chunk : _chain) {
            if(chunk.first) {
                stream << chunk.second.apply(input);
            } else {
                stream << "⎨" << chunk.second.apply(input) << "⎬";
            }
        }

        return stream;
    }

    inline chunk_chain() {}

    inline const_iterator begin() const { return _chain.begin(); }
    inline const_iterator end() const { return _chain.end(); }
    inline size_type size() const { return _chain.size(); }

    inline void emplace_back(chunk_type&& chunk) {
        _chain.emplace_back(std::forward<chunk_type>(chunk));
    }

    inline void emplace_back(bool matched, ChunkT&& chunk) {
        emplace_back(chunk_type{matched, std::forward<ChunkT>(chunk)});
    }

    std::size_t matched() const {
        std::size_t n = 0;
        for(const auto& chunk : _chain) {
            if(chunk.first) {
                n += chunk.second.size();
            }
        }
        return n;
    }

    inline void clear() {
        _chain.clear();
    }

    chain_type _chain;
};

template <>
struct chunk_chain<sequence_component>{
    using chunk_type = std::pair<bool, sequence_component>;
    using chain_type = std::vector<chunk_type>;
    using const_iterator = typename chain_type::const_iterator;
    using iterator = typename chain_type::iterator;
    using size_type = typename chain_type::size_type;

    std::ostream& apply(std::ostream& stream) const {
        stream << "[" << matched() << "] ";
        for(const auto& chunk : _chain) {
            if(chunk.first) {
                assert(std::holds_alternative<subsequence>(chunk.second));
                stream << std::get<subsequence>(chunk.second).str();
            } else {
                assert(std::holds_alternative<placeholder>(chunk.second));
                stream << "⎨" << std::get<placeholder>(chunk.second).str() << "⎬";
            }
        }

        stream << std::endl;
        for(const auto& chunk : _chain) {
            if(!chunk.first) {
                assert(std::holds_alternative<placeholder>(chunk.second));
                const placeholder& p = std::get<placeholder>(chunk.second);
                stream << "⎨" << p.str() << "⎬" << ": " << (p.count() < 50 ? boost::algorithm::join(p, ", ") : std::format("{} {}+", *(p.begin()), p.count()-1 ));
                stream << std::endl;
            }
        }

        return stream;
    }

    inline chunk_chain() {}

    inline const_iterator begin() const { return _chain.begin(); }
    inline const_iterator end() const { return _chain.end(); }
    inline iterator begin() { return _chain.begin(); }
    inline iterator end() { return _chain.end(); }
    inline size_type size() const { return _chain.size(); }

    inline void emplace_back(chunk_type&& chunk) {
        _chain.emplace_back(std::forward<chunk_type>(chunk));
    }

    inline void emplace_back(bool matched, sequence_component&& chunk) {
        emplace_back(chunk_type{matched, std::forward<sequence_component>(chunk)});
    }

    inline std::size_t matched() const {
        std::size_t n = 0;
        for(const auto& chunk : _chain) {
            if(chunk.first) {
                if(std::holds_alternative<subsequence>(chunk.second)) {
                    n += std::get<subsequence>(chunk.second).size();
                } else if(std::holds_alternative<placeholder>(chunk.second)) {
                    n += std::get<placeholder>(chunk.second).size();
                }
            }
        }
        return n;
    }

    inline void clear() {
        _chain.clear();
    }

    chain_type _chain;
};

/**
 * @brief The trace_parser class
 */
struct trace_parser{
    using string_type           = std::string;
    using char_type             = typename string_type::value_type;
    using matrix_type           = arma::mat;
    using chunk_type            = sequence_partition;
    using chain_type            = chunk_chain<chunk_type>;
    using graph_type            = chunk_chain<sequence_component>;
    using alignment_type        = std::vector<std::vector<std::pair<std::size_t, int>>>; // {size, placeholder_id} placeholder_id = -1 implies fixed string

    struct string_entry {
        string_type text;
        int cluster_id;

        inline explicit string_entry(const string_type& t, int id = -1) : text(t), cluster_id(id) {}
    };

    struct by_text {};
    struct by_cluster {};

    struct text_length_cmp {
        bool operator()(const std::string& a, const std::string& b) const noexcept {
            return a > b;
        }
    };

    using dataset_type = boost::multi_index_container<
        string_entry,
        boost::multi_index::indexed_by<
            // 0) preserve insertion/random‐access order if you need it:
            boost::multi_index::random_access<>,
            // 1) UNIQUE index on text:
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<by_text>,
                boost::multi_index::member<string_entry, std::string, &string_entry::text>,
                text_length_cmp
            >,
            // 2) NON-UNIQUE index on cluster_id:
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<by_cluster>,
                boost::multi_index::member<string_entry, int, &string_entry::cluster_id>
            >
        >
    >;

    using random_access_index = typename dataset_type::template nth_index<0>::type;
    using cluster_index = typename dataset_type::template nth_index<1>::type;

    struct Metric{
        inline explicit Metric(const trace_parser* p = 0x0) : _parser(p) {}
        inline explicit Metric(const trace_parser::Metric& other): _parser(other._parser) {}
        inline double Evaluate(std::size_t a, std::size_t b) const { return _parser->distance(a, b); }
        template<typename VecA, typename VecB>
        double Evaluate(const VecA& a, const VecB& b) const{ return Evaluate(static_cast<std::size_t>(a[0]), static_cast<std::size_t>(b[0])); }

        private:
            const trace_parser* _parser;
    };

    trace_parser(): _computed(false), _clustered(false) {}

    void parse(const std::filesystem::path& path);

    template <typename IteratorT>
    void append(IteratorT begin, IteratorT end) {
        for (IteratorT it = begin; it != end; ++it) {
            if (!(*it).empty())
                append(*it);
        }
    }

    void append(const string_type& str);

    trace_parser& operator<<(const string_type& str);

    /**
     * @brief compute pairwise distance and store it inside distance matrix. afterwards set _computed to true.
     */
    void compute(std::size_t threads = 0, bool score_only = false);

    double distance(std::size_t i, std::size_t j, bool score_only = false) const;

    const matrix_type& distances() const { return _distances; }
    bool computed() const { return _computed; }

    /**
     * @brief align two strings using sequence alignment algorithm
     * @param lhs
     * @param rhs
     * @param alignment_score pointer to store the alignment score (optional)
     * @return a chain of chunks where each chunk is (is_match, substring)
     */
    chain_type align(const string_type& lhs, const string_type& rhs, double* alignment_score = nullptr) const;

    /**
     * @brief cluster the dataset using the distance matrix and assign cluster number to each sample.
     * @return number of clusters found;
     */
    std::size_t cluster(double eps = 0.20, std::size_t minPts = 2);

    /**
     * @brief split the dataset into multiple files inside the directory mentioned in path.
     *        the files will be named as cluster_id.cluster.txt it will be plain text line
     *        by line copy from the _datasets
     *        serializes the distance matrix as distances.matrix file inside path directory
     *        using a binary archive through boost serialization library.
     *        additionally creates a json document named index.json inside the path directory
     *        containing a list of json object each containing id -> cluster id, path -> path
     *        to which lines associated with that cluster id is stored, count -> number of
     *        lines of log messages stored in that file.
     * @pre dataset is already clustered
     * @param path path to a directory (create if it does not exist)
     * @param path
     */
    void save(const std::filesystem::path& path);

    /**
     * @brief load the distance matrix from the path into _distances matrix.
     *        open the index.json file from the path and read it.
     *        as _dataset is expected to be empty before calling load,
     *        so read each file mentioned in the index.json and add the string with the cluster
     *        id to the _dataset
     * @param path
     */
    void load(const std::filesystem::path& path);

    inline auto begin() const { return _dataset.template get<0>().begin(); }
    inline auto end() const { return _dataset.template get<0>().end(); }
    inline std::size_t count() const { return _dataset.size(); }

    /**
     * @brief Get string by index
     */
    inline const string_type& operator[](std::size_t index) const {
        return _dataset.template get<0>()[index].text;
    }

    /**
     * @brief Get all strings with a specific cluster label
     */
    inline auto cluster_range(int cluster_id) const {
        return _dataset.template get<by_cluster>().equal_range(cluster_id);
    }

    /**
     * @brief Count strings with a specific cluster label
     */
    inline std::size_t cluster_count(int cluster_id) const {
        return _dataset.template get<by_cluster>().count(cluster_id);
    }

    inline std::size_t cluster_count() const {
        std::unordered_set<int> ids;
        for (const auto& rec : _dataset)
            if (rec.cluster_id >= 0)
                ids.insert(rec.cluster_id);

        return ids.size();
    }

    std::ostream& print(std::ostream& stream) const;

    trace_parser::graph_type align(int cluster_id, std::vector<std::vector<zone> >& all_zones) const;

    std::ostream& print_aligned(int cluster_id, std::ostream &stream, const std::vector<std::vector<zone> >& all_zones) const;

    static void adjust(trace_parser::graph_type& malignment, std::vector<std::vector<zone>>& zones);

    void save_alignments(const std::filesystem::path &dir, int cluster_id, const graph_type &malignment, const std::vector<std::vector<zone> > &zones);


private:
    dataset_type _dataset;
    matrix_type  _distances;
    bool         _computed;
    bool         _clustered;
};

namespace mlpack     {
    template<> struct IsLMetric<trace_parser::Metric> { enum { Value = true }; };
}

#endif // TRACE_PARSER_H
