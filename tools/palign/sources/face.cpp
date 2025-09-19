#include <prova/face.h>
#include <prova/collection.h>
#include <prova/index.h>

prova::align::index prova::align::face::move(const index &idx, int64_t delta) const {
    index moved{idx};
    moved.move(_index, delta);
    return moved;
}

prova::align::index prova::align::face::slider::first() const{
    index idx{_face._dimensions};
    for(std::size_t dim = 0; dim != _face._dimensions; ++dim) {
        idx.at(dim) = _face._collection.at(dim).size() -1;
    }
    idx.at(_face._index) = min();
    return idx;
}

prova::align::index prova::align::face::slider::last() const{
    index idx{_face._dimensions};
    for(std::size_t dim = 0; dim != _face._dimensions; ++dim) {
        idx.at(dim) = _face._collection.at(dim).size() -1;
    }
    idx.at(_face._index) = max();
    return idx;
}

bool prova::align::face::slider::iterator::operator==(const iterator &other) const {
    return _slider == other._slider && _at == other._at;
}

bool prova::align::face::slider::iterator::invalid() const { return _at > _slider.length(); }

prova::align::face::slider::iterator prova::align::face::slider::iterator::operator++(int) {
    iterator old = *this;
    operator++();
    return old;
}

prova::align::face::slider::iterator &prova::align::face::slider::iterator::operator--() {
    --_at;
    return *this;
}

prova::align::face::slider::iterator &prova::align::face::slider::iterator::operator++() {
    ++_at;
    return *this;
}

prova::align::face::slider::iterator prova::align::face::slider::iterator::operator--(int) {
    iterator old = *this;
    operator--();
    return old;
}

prova::align::face::slider::iterator::value_type prova::align::face::slider::iterator::operator*() const{
    index idx{_slider._face._dimensions};
    for(std::size_t dim = 0; dim != _slider._face._dimensions; ++dim) {
        idx.at(dim) = _slider._face._collection.at(dim).size() -1;
    }
    idx.at(_slider._face._index) = _at;
    return idx;
}

prova::align::face::face(const collection& coll, std::size_t index): _collection(coll), _dimensions(coll.count()), _index(index) {
    assert(index < _dimensions);
}

bool prova::align::face::operator==(const face &other) const {
    return /*_collection == other._collection &&*/ _dimensions == other._dimensions && _index == other._index;
    // collection comparison not necessary as of now because there will be only one collection in the experiments
    // if needed we can use hash based comparison latter
}

prova::align::face::slider prova::align::face::slide() const {
    return slider{*this};
}

prova::align::index prova::align::face::slider::move(const index &idx, int64_t delta) const { return _face.move(idx, delta); }

std::size_t prova::align::face::slider::length() const { return max() - min(); }

std::size_t prova::align::face::slider::min() const {
    return 0;
}

std::size_t prova::align::face::slider::max() const {
    return _face._collection.at(_face._index).size() -1;
}

bool prova::align::face::slider::operator==(const slider &other) const {
    return _face == other._face;
}
