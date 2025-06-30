#ifndef TRACE_PARSER_H
#define TRACE_PARSER_H

#include <list>
#include <string>
#include <iostream>
#include <filesystem>
#include <iostream>
#include <armadillo>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/member.hpp>
#include <mlpack/core/tree/hrectbound.hpp>

/**
 * @brief The trace_parser class
 */
struct trace_parser{
    using string_type           = std::string;
    using char_type             = typename string_type::value_type;
    using matrix_type           = arma::mat;

    struct continous_block{
        bool        matched;
        std::string content;
        std::set<std::string> possibilities;
        std::pair<std::size_t, std::size_t> limits;
    };

    struct chunk_chain{
        using chunk_type = continous_block;
        using chain_type = std::list<chunk_type>;
        using const_iterator = typename chain_type::const_iterator;
        using size_type = typename chain_type::size_type;

        friend std::ostream& operator<<(std::ostream& stream, const chunk_chain& chain);

        inline chunk_chain(): _multiple(false) {}
        chunk_chain(const chunk_chain&) = default;
        explicit chunk_chain(chunk_type&& chunk) {
            emplace_back(std::move(chunk));
        }
        inline const_iterator begin() const { return _chain.begin(); }
        inline const_iterator end() const { return _chain.end(); }
        inline size_type size() const { return _chain.size(); }

        template <typename... T>
        void emplace_back(T&&... args) {
            _chain.emplace_back(std::forward<T>(args)...);
        }

        std::size_t matched() const;

        chain_type _chain;
        bool       _multiple;
    };

    using chunk_type            = continous_block;
    using chain_type            = chunk_chain;

    struct string_entry {
        string_type text;
        int cluster_id;

        inline explicit string_entry(const string_type& t, int id = -1) : text(t), cluster_id(id) {}
    };

    using dataset_type = boost::multi_index::multi_index_container<
        string_entry,
        boost::multi_index::indexed_by<
            boost::multi_index::random_access<>,
            boost::multi_index::ordered_non_unique<
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
    void compute(bool score_only = false);

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
        return _dataset.template get<1>().equal_range(cluster_id);
    }

    /**
     * @brief Count strings with a specific cluster label
     */
    inline std::size_t cluster_count(int cluster_id) const {
        return _dataset.template get<1>().count(cluster_id);
    }

    inline std::size_t cluster_count() const {
        std::unordered_set<int> ids;
        for (const auto& rec : _dataset)
            if (rec.cluster_id >= 0)
                ids.insert(rec.cluster_id);

        return ids.size();
    }

    std::ostream& print(std::ostream& stream) const;

    trace_parser::chain_type align(int cluster_id) const;
    void align_all() const;

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
