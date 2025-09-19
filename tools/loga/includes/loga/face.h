#ifndef PROVA_ALIGN_FACE_H
#define PROVA_ALIGN_FACE_H

#include <loga/fwd.h>
#include <cstddef>
#include <cstdint>
#include <cassert>

namespace prova::loga{

class face{
    std::size_t _dimensions;
    std::size_t _index;
    const collection& _collection;
public:
    explicit face(const collection& collection, std::size_t index);

    bool operator==(const face& other) const;

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
        index move(const index& idx, std::int64_t delta) const;
        inline index first() const;
        inline index last() const;

        std::size_t length() const;
        std::size_t min() const;
        std::size_t max() const;

        bool operator==(const slider& other) const;
    public:
        class iterator{
            slider& _slider;
            std::size_t _at;
        public:
            using value_type = index;

            inline explicit iterator(slider& s): _slider(s), _at(s.length()+1) {}
            inline explicit iterator(slider& s, std::size_t at): _slider(s), _at(at) {}

            bool operator==(const iterator& other) const;

            bool invalid() const;

            iterator& operator++();
            iterator operator++(int);
            iterator& operator--();
            iterator operator--(int);
            value_type operator*() const;

        };

        iterator begin() { return iterator{*this, 0}; }
        iterator end() { return iterator{*this}; }
    };

    friend class iterator;

    slider slide() const;
};

}

#endif // PROVA_ALIGN_FACE_H
