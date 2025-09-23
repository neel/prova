#ifndef LOGA_ALIGN_GROUP_H
#define LOGA_ALIGN_GROUP_H

#include <armadillo>
#include <loga/collection.h>
#include <loga/alignment.h>
#include <loga/cluster.h>
#include <map>

namespace prova::loga {

class group{
    using distance_matrix_type = prova::loga::cluster::distance_matrix_type;
    using labels_type          = prova::loga::cluster::labels_type;
    using mapping_type         = std::multimap<std::size_t, std::size_t>;

    const collection&       _collection;
    const labels_type&      _labels;
    mapping_type            _mapping;

public:
    class label_proxy{
    public:
        using mask_type = std::set<std::size_t>;
    private:
        const group& _group;
        std::size_t  _label;
        mask_type    _mask;

        friend class group;

        explicit label_proxy(const group& g, std::size_t label);
    public:
        class value{
            const std::string& _str;
            std::size_t _index;

            friend class label_proxy;
        private:
            value(const std::string& str, std::size_t index);
        public:
            const std::string& str() const;
            std::size_t id() const;
        };
    public:
        std::size_t label() const;
        value at(std::size_t i) const;
        std::size_t count() const;
        const mask_type& mask() const;
    };

public:
    group(const collection& collection, const labels_type& labels);
    group::label_proxy proxy(std::size_t label);
    std::size_t labels() const;
};

}

#endif // LOGA_ALIGN_GROUP_H
