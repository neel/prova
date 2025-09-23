#include <loga/group.h>


prova::loga::group::group(const collection &collection, const labels_type &labels): _collection(collection), _labels(labels) {
    // _labels contains id -> label mapping but we need the opposite
    // build the _mapping multimap using labels
    // this _mapping can be used to iterate over items in the same cluster
    _mapping.clear();
    // _labels is id -> label; we invert to multimap<label, id>
    for (std::size_t id = 0; id < _labels.n_elem; ++id) {
        _mapping.emplace(_labels[id], id);
    }
}

prova::loga::group::label_proxy prova::loga::group::proxy(std::size_t label){
    return prova::loga::group::label_proxy(*this, label);
}


prova::loga::group::label_proxy::label_proxy(const group &g, std::size_t label): _group(g), _label(label) {}

std::size_t prova::loga::group::label_proxy::label() const { return _label; }

prova::loga::group::label_proxy::value prova::loga::group::label_proxy::at(std::size_t i) const{
    // return the i'th value under the key _label in the _mapping multimap
    const auto range = _group._mapping.equal_range(_label);

    // Count how many ids are present for this label to bounds-check 'i'
    std::size_t count = 0;
    for (auto it = range.first; it != range.second; ++it) ++count;
    if (i >= count) {
        throw std::out_of_range("label_proxy::at: index out of range for label");
    }

    // Advance to the i'th element in the range
    auto it = range.first;
    std::advance(it, static_cast<std::ptrdiff_t>(i));
    const std::size_t id = it->second;
    const std::string& str = _group._collection.at(id);

    return value{str, id};
}

std::size_t prova::loga::group::label_proxy::count() const {
    const auto range = _group._mapping.equal_range(_label);
    return std::distance(range.first, range.second);
}

std::size_t prova::loga::group::labels() const {
    std::size_t unique_labels = 0;
    std::size_t last_label = std::numeric_limits<std::size_t>::max();

    for (const auto& kv : _mapping) {
        if (kv.first != last_label) {
            ++unique_labels;
            last_label = kv.first;
        }
    }
    return unique_labels;
}

prova::loga::group::label_proxy::value::value(const std::string &str, std::size_t index): _str(str), _index(index) {}

const std::string &prova::loga::group::label_proxy::value::str() const{
    return _str;
}

std::size_t prova::loga::group::label_proxy::value::id() const{
    return _index;
}
