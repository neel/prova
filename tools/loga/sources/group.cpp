#include <loga/group.h>


prova::loga::group::group(const collection &collection, const labels_type &labels): _collection(collection), _labels(labels) {
    _mapping.clear();
    for (std::size_t id = 0; id < _labels.n_elem; ++id) {
        _mapping.emplace(_labels[id], id);
    }
}

prova::loga::group::label_proxy prova::loga::group::proxy(std::size_t label){
    return prova::loga::group::label_proxy(*this, label);
}


prova::loga::group::label_proxy::label_proxy(const group &g, std::size_t label): _group(g), _label(label) {
    _mask.clear();
    const auto range = _group._mapping.equal_range(_label);
    for(const auto& [_, id]: std::ranges::subrange(range.first, range.second)){
        _mask.insert(id);
    }
}

std::size_t prova::loga::group::label_proxy::label() const { return _label; }

prova::loga::group::label_proxy::value prova::loga::group::label_proxy::at(std::size_t i) const{
    const auto range = _group._mapping.equal_range(_label);

    if (i >= std::distance(range.first, range.second)) {
        throw std::out_of_range("label_proxy::at: index out of range for label");
    }

    auto it = range.first;
    std::advance(it, i);
    const std::size_t id = it->second;
    const std::string& str = _group._collection.at(id);

    return value{str, id};
}

std::size_t prova::loga::group::label_proxy::count() const {
    const auto range = _group._mapping.equal_range(_label);
    return std::distance(range.first, range.second);
}

const prova::loga::group::label_proxy::mask_type &prova::loga::group::label_proxy::mask() const {
    return _mask;
}

std::size_t prova::loga::group::labels() const {
    std::size_t unique_labels = 0;
    std::size_t last_label = std::numeric_limits<std::size_t>::max();

    for (const auto& kv : _mapping) {
        if (kv.first != last_label) {
            if(kv.first == std::numeric_limits<std::size_t>::max()){
                continue;
            }
            ++unique_labels;
            last_label = kv.first;
        }
    }
    return unique_labels;
}

std::size_t prova::loga::group::unclustered() const {
    const auto range = _mapping.equal_range(std::numeric_limits<std::size_t>::max());
    return std::distance(range.first, range.second);
}

prova::loga::group::label_proxy::value::value(const std::string &str, std::size_t index): _str(str), _index(index) {}

const std::string &prova::loga::group::label_proxy::value::str() const{
    return _str;
}

std::size_t prova::loga::group::label_proxy::value::id() const{
    return _index;
}
