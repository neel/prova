#include <loga/token.h>

inline prova::loga::wrapped::wrapped(const std::string &str, const token &token): _str(str), _token(token) {
    std::string_view v = view();
    _hash = std::hash<std::string_view>{}(v);
}

prova::loga::wrapped::wrapped(const wrapped &other): _str(other._str), _token(other._token), _hash(other._hash) {}

std::string_view prova::loga::wrapped::view() const {
    std::string::const_iterator begin = _str.cbegin();
    std::string::const_iterator end   = begin;

    std::advance(begin, _token.lower());
    std::advance(end,   _token.upper());

    // C++20 supports
    return std::string_view(begin, end);
}

// prova::loga::wrapped& prova::loga::wrapped::operator=(const prova::loga::wrapped& other){
//     _str   = other._str;
//     _token = other._token;
//     _hash  = other._hash;
//     return *this;
// }

std::size_t prova::loga::wrapped::length() const { return _token.length(); }

prova::loga::token::category prova::loga::wrapped::category() const {return _token.cat(); }

prova::loga::token::boundary_type prova::loga::wrapped::upper() const { return _token.upper(); }

prova::loga::token::boundary_type prova::loga::wrapped::lower() const { return _token.lower(); }

bool prova::loga::operator==(const token &l, const token &r) {
    return l._interval == r._interval && l._category == r._category;
}

bool prova::loga::operator==(const wrapped &lw, const wrapped &rw) {
    if(lw.category() != rw.category())
        return false;
    return lw.hash() == rw.hash();
    // return lw.view() == rw.view();
}

prova::loga::tokenized::tokenized(const tokenized& other): _str(other._str), _characteristics(other._characteristics){
    std::transform(other._tokens.cbegin(), other._tokens.cend(), std::back_inserter(_tokens), [this](const wrapped& w){
        return wrapped{_str, w._token};
    });
}

prova::loga::tokenized::tokenized(tokenized &&other): _str(std::move(other._str)), _characteristics(std::move(other._characteristics)) {
    std::transform(other._tokens.cbegin(), other._tokens.cend(), std::back_inserter(_tokens), [this](const wrapped& w){
        return wrapped{_str, w._token};
    });
    other._tokens.clear();
}

prova::loga::tokenized::tokenized(const std::string &str): _str(str) {
    std::vector<token> raw_tokens;
    tokenizer::parse(_str.cbegin(), _str.cend(), std::back_inserter(raw_tokens));
    std::transform(raw_tokens.cbegin(), raw_tokens.cend(), std::back_inserter(_tokens), [this](const token& t){
        return wrapped{_str, t};
    });

    std::transform(raw_tokens.cbegin(), raw_tokens.cend(), std::back_inserter(_characteristics), [this](const token& t){
        return t.vector();
    });
}

prova::loga::tokenized& prova::loga::tokenized::operator=(prova::loga::tokenized&& other) {
    _str = std::move(other._str);
    _characteristics = std::move(other._characteristics);
    _tokens.clear();
    std::transform(other._tokens.cbegin(), other._tokens.cend(), std::back_inserter(_tokens), [this](const wrapped& w){
        return wrapped{_str, w._token};
    });
    other._tokens.clear();

    return *this;
}

std::size_t prova::loga::tokenized::count() const { return _tokens.size(); }

const prova::loga::tokenized::structure_type &prova::loga::tokenized::structure() const{
    return _characteristics;
}

const std::string &prova::loga::tokenized::raw() const { return _str; }

const prova::loga::wrapped &prova::loga::tokenized::at(std::size_t i) const { return _tokens.at(i); }

prova::loga::tokenized::const_iterator prova::loga::tokenized::end() const { return _tokens.cend(); }

prova::loga::tokenized::const_iterator prova::loga::tokenized::begin() const { return _tokens.cbegin(); }

prova::loga::token::token(boundary_type begin, boundary_type end, category cat): _interval(interval_type::right_open(begin, end)), _category(cat) {}

std::size_t prova::loga::token::length() const { return upper() - lower(); }

prova::loga::token::coordinate prova::loga::token::vector() const {
    coordinate v = vector(_category);
    v.scale(length());
    return v;
}

prova::loga::token::coordinate prova::loga::token::vector(token::category cat) {
    if (cat == category::alpha)  return coordinate(0);
    if (cat == category::digits) return coordinate(1);
    if (cat == category::symbol) return coordinate(2);
    if (cat == category::space)  return coordinate(3);
    if (cat == category::place)  return coordinate(4);
    assert(cat != category::none);
    return coordinate(4);
}

prova::loga::token::category prova::loga::token::classify(uint8_t ch) {
    if (ch == '$') return category::place;
    if (std::isalpha(ch)) return category::alpha;
    if (std::isdigit(ch)) return category::digits;
    if (std::ispunct(ch)) return category::symbol;
    if (std::isspace(ch)) return category::space;
    return category::none;
}

prova::loga::token::category prova::loga::token::cat() const {return _category; }

prova::loga::token::boundary_type prova::loga::token::upper() const { return _interval.upper(); }

prova::loga::token::boundary_type prova::loga::token::lower() const { return _interval.lower(); }
