#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <algorithm>
#include <map>
#include <bit>
#include <boost/graph/adjacency_list.hpp>
#include <boost/icl/interval.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/separate_interval_set.hpp>

namespace prova{

namespace algorithms{

class index;

bool operator<(const index& left, const index& right);
bool operator>(const index& left, const index& right);
bool operator==(const index& left, const index& right);
bool operator!=(const index& left, const index& right);
bool operator<=(const index& left, const index& right);
bool operator>=(const index& left, const index& right);
std::ostream& operator<<(std::ostream& stream, const index& idx);
index operator+(const index& idx, std::size_t l);
index operator-(const index& idx, std::size_t l);
index operator+(const index& left, const index& right);
index operator-(const index& left, const index& right);

class index{
    std::vector<std::size_t> _positions;

    friend bool operator<(const index& left, const index& right);
    friend bool operator>(const index& left, const index& right);
    friend bool operator==(const index& left, const index& right);
    friend bool operator!=(const index& left, const index& right);
    friend bool operator<=(const index& left, const index& right);
    friend bool operator>=(const index& left, const index& right);
    friend std::ostream& operator<<(std::ostream& stream, const index& idx);
    friend index operator+(const index& idx, std::size_t l);
    friend index operator+(const index& left, const index& right);
    friend index operator-(const index& idx, std::size_t l);
    friend index operator-(const index& left, const index& right);
public:
    using const_iterator = std::vector<std::size_t>::const_iterator;
    using size_type = std::vector<std::size_t>::size_type;
public:
    inline explicit index(std::size_t dims): _positions(dims, 0) {
        assert(_positions.size() == dims);
        assert(_positions.size() >= 2);
    }
    explicit index(std::vector<std::size_t>&& positions) : _positions(std::move(positions)) {
        assert(_positions.size() >= 2);
    }

    void move_all(std::int64_t delta);
    void move(std::size_t dim, std::int64_t delta);
public:
    index bottom_right() const;
    index top_left() const;
    bool is_top() const;
public:
    inline size_type count() const { return _positions.size(); }
    inline const_iterator begin() const { return _positions.begin(); }
    inline const_iterator end() const { return _positions.end(); }
    inline std::size_t at(std::size_t index) const { return _positions.at(index); }
    inline std::size_t operator[](std::size_t index) const { return at(index); }

    inline std::size_t& at(std::size_t index) { return _positions.at(index); }
    inline std::size_t& operator[](std::size_t index) { return at(index); }

    index distance(const index& other) const;
    std::size_t l1_from_zero() const;
};

class collection;

class face{
    std::size_t _dimensions;
    std::size_t _index;
    const collection& _collection;
public:
    explicit face(const collection& collection, std::size_t index);

    inline bool operator==(const face& other) const {
        return /*_collection == other._collection &&*/ _dimensions == other._dimensions && _index == other._index;
        // collection comparison not necessary as of now because there will be only one collection in the experiments
        // if needed we can use hash based comparison latter
    }

    /**
     * @brief move idx by delta at dimension dim
     * @param idx
     * @param at
     * @param delta
     * @return
     */
    index move(const index& idx, std::int64_t delta) const;

    class slider{
        const face& _face;

        friend class face;

        inline slider(const face& face): _face(face) {}
    public:
        inline index move(const index& idx, std::int64_t delta) const { return _face.move(idx, delta); }
        inline index first() const;
        inline index last() const;

        inline std::size_t length() const { return max() - min(); }
        std::size_t min() const;
        std::size_t max() const;

        inline bool operator==(const slider& other) const {
            return _face == other._face;
        }
    public:
        class iterator{
            slider& _slider;
            std::size_t _at;
        public:
            using value_type = index;

            inline explicit iterator(slider& s): _slider(s), _at(s.length()+1) {}
            inline explicit iterator(slider& s, std::size_t at): _slider(s), _at(at) {}

            inline bool operator==(const iterator& other) const {
                return _slider == other._slider && _at == other._at;
            }

            bool invalid() const { return _at > _slider.length(); }

            inline iterator& operator++() {
                ++_at;
                return *this;
            }

            inline iterator operator++(int) {
                iterator old = *this;
                operator++();
                return old;
            }

            inline iterator& operator--() {
                --_at;
                return *this;
            }

            inline iterator operator--(int) {
                iterator old = *this;
                operator--();
                return old;
            }

            inline value_type operator*() const;

        };

        iterator begin() { return iterator{*this, 0}; }
        iterator end() { return iterator{*this}; }
    };

    friend class iterator;

    slider slide() const {
        return slider{*this};
    }
};

class collection{
    std::vector<std::string> _inputs;

    friend class face::slider::iterator;
public:
    using const_iterator = std::vector<std::string>::const_iterator;
    using size_type = std::vector<std::string>::size_type;
public:
    inline size_type count() const { return _inputs.size(); }
    inline const_iterator begin() const { return _inputs.begin(); }
    inline const_iterator end() const { return _inputs.end(); }
    inline const std::string& at(std::size_t index) const { return _inputs.at(index); }
    inline const std::string& operator[](std::size_t index) const { return at(index); }
public:
    void add(const std::string& str);
    std::size_t unique(const index& idx) const;
    bool unanimous_concensus(const index& idx) const;
};

class segment;

std::ostream& operator<<(std::ostream& stream, const segment& s);

class segment{
    const std::string& _base;
    index              _start;
    std::size_t        _length;

    friend std::ostream& operator<<(std::ostream& stream, const segment& s);

public:
    inline explicit segment(const std::string& base, index start, std::size_t length): _base(base), _start(start), _length(length) {}
    inline segment(const segment& other) = default;
    inline segment(segment&& other): _base(other._base), _start(std::move(other._start)), _length(std::move(other._length)) {
        for(std::size_t i = 0; i != other._start.count(); ++i) {
            other._start.at(i) = 0;
        }
        other._length = 0;
    }

    inline const std::string& base() const { return _base; }
    inline const prova::algorithms::index start() const { return _start; }
    inline const prova::algorithms::index end() const { return _start + _length-1; }
    inline std::size_t length() const { return _length; }
    inline std::string_view view() const {
        auto start = _base.begin();
        std::advance(start, _start.at(0));
        auto finish = start;
        std::advance(finish, _length);
        return {start, finish};
    }
};

class path{
    using container_type = std::vector<segment>;

    container_type _segments;
public:
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;
    using value_type = container_type::value_type;
    using size_type = container_type::size_type;
    using reference = container_type::reference;
    using const_reference = container_type::const_reference;

    inline path() = default;
    inline path(const path&) = default;
    inline path(path&& other): _segments(other._segments) {}

    inline void add(const segment& s){
        _segments.push_back(s);
    }
    inline const_iterator begin() const { return _segments.begin(); }
    inline const_iterator end() const { return _segments.end(); }
    inline size_type size() const { return _segments.size(); }
    std::ostream& print(std::ostream& out);

    std::size_t matched() const;
    double score() const;
};

class graph{
    struct edge_props {
        double weight;
        int    slide;
    };

    using segment_collection_type   = std::vector<segment>;
    using graph_type                = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, boost::no_property, edge_props>;
    using vertex_type               = boost::graph_traits<graph_type>::vertex_descriptor;
    using edge_type                 = boost::graph_traits<graph_type>::edge_descriptor;

    segment_collection_type _segments;
    graph_type  _graph;
    vertex_type _S, _T;
    std::map<vertex_type, std::size_t> _vertices;
    segment _start;
    segment _finish;
public:
    inline explicit graph(segment_collection_type&& segments, segment&& start, segment&& finish);
    void build();
    std::ostream& print(std::ostream& stream);
    path shortest_path();
public:
    using iterator = segment_collection_type::iterator;
    using const_iterator = segment_collection_type::const_iterator;
    using value_type = segment;
    using size_type  = segment_collection_type::size_type;
    using reference_type = std::add_lvalue_reference_t<segment>;
    using const_reference_type = std::add_const_t<reference_type>;

    const_iterator begin() const { return _segments.cbegin(); }
    const_iterator end() const { return _segments.cend(); }
    size_type size() const { return _segments.size(); }
};

class alignment{
    struct edge_props {
        double weight;
        int    slide;
    };

    using memo_type                 = std::map<index, std::size_t>;
    using segment_collection_type   = std::vector<segment>;
    using const_iterator            = collection::const_iterator;
    using graph_type                = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, boost::no_property, edge_props>;
    using vertex_type               = boost::graph_traits<graph_type>::vertex_descriptor;

    collection _collection;
    memo_type  _memo;
public:
    using key_type = std::pair<std::uint32_t, std::uint32_t>;
    struct pair_hash{
        std::uint64_t operator()(const key_type& key) const noexcept {
            std::uint64_t res = key.first;
            res = std::rotl(res, 32) + key.second;
            return res;
        }
    };
    using matrix_type = std::unordered_map<key_type, prova::algorithms::path, pair_hash>;
public:
    inline void add(const std::string& str) { _collection.add(str); }
    const collection& inputs() const { return _collection; }

    /**
     * @brief bubble accross the kD tensor starting from the given index through the diagonal (all 1) line
     * @param idx
     * @param threshold
     * @param carry
     * @pre expects idx.size() == inputs.size()
     */
    void bubble(const index& idx, std::size_t threshold, std::size_t carry);

    /**
     * @brief bubble from all floor points in the kD tensor
     * @param threshold
     * @return
     */
    graph bubble_all(std::size_t threshold = 1);

    /**
     * @brief bubble accross a matrix starting from the given index through the diagonal line
     * @param u
     * @param v
     * @param idx
     * @param threshold
     * @param carry
     * @pre expects the idx.size() == 2
     */
    void bubble_pairwise(const_iterator u, const_iterator v, const index& idx, memo_type& memo, std::size_t threshold, std::size_t carry);

    /**
     * @brief Takes the first one as the base and others as the reference
     * @param u
     * @param v
     * @param threshold
     */
    void bubble_all_pairwise(prova::algorithms::alignment::matrix_type& mat, std::size_t threshold = 1);

    const memo_type& memo() const { return _memo; }
};

enum class zone{ constant, placeholder };
inline std::ostream& operator<<(std::ostream& stream, const zone& z) {
    if(z == zone::constant) {
        stream << "C";
    } else {
        stream << "P";
    }
    return stream;
}

class multi_alignment{
    struct matched_val{
        std::size_t id;
        std::size_t ref_pos;
        std::size_t base_pos;

        bool operator<(const matched_val& other) const {
            return id < other.id;
        }

        bool operator==(const matched_val& other) const {
            return id == other.id && ref_pos == other.ref_pos && base_pos == other.base_pos;
        }
    };

    const collection& _collection;
    const alignment::matrix_type& _matrix;
    std::size_t _base_index;

public:
    using interval_val  = std::set<matched_val>;
    using interval_map  = boost::icl::split_interval_map<std::size_t, interval_val>;
    using interval_set  = boost::icl::split_interval_map<std::size_t, std::set<zone>>;
    using region_map    = std::map<std::size_t, interval_set>;
    using region_type   = interval_set::interval_type;
    using interval_type = interval_map::interval_type;

public:
    inline multi_alignment(const collection& collection, const alignment::matrix_type& matrix, std::size_t base_index): _collection(collection), _matrix(matrix), _base_index(base_index) {}
    region_map align() const;
    region_map fixture_word_booundary(const region_map &regions) const;

public:
    std::ostream& print_regions(const region_map& regions, std::ostream& stream);
};

}

}

#endif // ALGORITHMS_H
