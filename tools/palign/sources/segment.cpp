#include <prova/segment.h>

std::ostream& prova::align::operator<<(std::ostream& stream, const prova::align::segment& s){
    stream << s._start << ": " << s._length << " " << s.view();
    return stream;
}

prova::align::segment::segment(const std::string &base, index start, std::size_t length): _base(base), _start(start), _length(length) {}

prova::align::segment::segment(segment &&other): _base(other._base), _start(std::move(other._start)), _length(std::move(other._length)) {
    for(std::size_t i = 0; i != other._start.count(); ++i) {
        other._start.at(i) = 0;
    }
    other._length = 0;
}

const std::string &prova::align::segment::base() const { return _base; }

const prova::align::index prova::align::segment::start() const { return _start; }

const prova::align::index prova::align::segment::end() const { return _start + _length-1; }

std::size_t prova::align::segment::length() const { return _length; }

std::string_view prova::align::segment::view() const {
    auto start = _base.begin();
    std::advance(start, _start.at(0));
    auto finish = start;
    std::advance(finish, _length);
    return {start, finish};
}
