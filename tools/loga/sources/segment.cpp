#include <loga/segment.h>
#include <loga/collection.h>
#include <loga/tokenized_collection.h>

std::ostream& prova::loga::operator<<(std::ostream& stream, const prova::loga::segment& s){
    stream << s._start << ": " << s._length /*<< " " << s.view()*/;
    return stream;
}

prova::loga::segment::segment(): _base(0), _length(0) {

}

prova::loga::segment::segment(const std::size_t base, index start, std::size_t length): _base(base), _start(start), _length(length) {}

prova::loga::segment::segment(segment &&other): _base(other._base), _start(std::move(other._start)), _length(std::move(other._length)) {
    for(std::size_t i = 0; i != other._start.count(); ++i) {
        other._start.at(i) = 0;
    }
    other._length = 0;
}

const std::string &prova::loga::segment::base(const prova::loga::collection &collection) const {
    return collection.at(_base);
}

const std::string &prova::loga::segment::base(const prova::loga::tokenized_collection &collection) const {
    return collection.at(_base).raw();
}

const prova::loga::index prova::loga::segment::start() const { return _start; }

const prova::loga::index prova::loga::segment::end() const { return _start + _length-1; }

std::size_t prova::loga::segment::length() const { return _length; }

std::string_view prova::loga::segment::view(const collection &collection) const {
    const std::string& base = collection.at(_base);
    auto start = base.begin();
    std::advance(start, _start.at(0));
    auto finish = start;
    std::advance(finish, _length);
    return {start, finish};
}

std::string prova::loga::segment::view(const tokenized_collection &collection) const {
    const prova::loga::tokenized& base = collection.at(_base);
    auto start = base.begin();
    std::advance(start, _start.at(0));
    auto finish = start;
    std::advance(finish, _length);

    std::string res;
    for(auto it = start; it != finish; ++it) {
        std::string_view view = it->view();
        res += view;
    }

    return res;
}

std::size_t prova::loga::segment::chars(const tokenized_collection &collection) const {
    const prova::loga::tokenized& base = collection.at(_base);
    auto start = base.begin();
    std::advance(start, _start.at(0));
    auto finish = start;
    std::advance(finish, _length);

    std::size_t res;
    for(auto it = start; it != finish; ++it) {
        res += it->view().size();
    }

    return res;
}
