#ifndef PROVA_ALIGN_TOKENIZED_GROUP_H
#define PROVA_ALIGN_TOKENIZED_GROUP_H

#include <armadillo>
#include <loga/tokenized_collection.h>
#include <loga/tokenized_alignment.h>
#include <loga/cluster.h>
#include <map>

namespace prova::loga {

class tokenized_group{
    using distance_matrix_type = arma::mat;
    using labels_type          = arma::Row<std::size_t>;
    using mapping_type         = std::multimap<std::size_t, std::size_t>;

    const tokenized_collection&       _collection;
    const labels_type&      _labels;
    mapping_type            _mapping;

public:
    class label_proxy{
    public:
        using mask_type = std::set<std::size_t>;
    private:
        const tokenized_group& _group;
        std::size_t  _label;
        mask_type    _mask;

        friend class tokenized_group;

        explicit label_proxy(const tokenized_group& g, std::size_t label);
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
    tokenized_group(const tokenized_collection& collection, const labels_type& labels);
    tokenized_group::label_proxy proxy(std::size_t label);
    std::size_t labels() const;
    std::size_t unclustered() const;
};

}


#endif // PROVA_ALIGN_TOKENIZED_GROUP_H
