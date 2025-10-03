#ifndef PROVA_ALIGN_DISTANCE_H
#define PROVA_ALIGN_DISTANCE_H

#include <loga/alignment.h>
#include <armadillo>

namespace prova::loga{

class distance{
public:
    using key_type             = prova::loga::alignment::key_type;
    using path_matrix_type     = prova::loga::alignment::matrix_type;
    using distance_matrix_type = arma::mat;
private:
    const path_matrix_type& _paths;
    std::size_t             _count;
    distance_matrix_type    _distances;
public:
    explicit distance(const path_matrix_type& paths, std::size_t count);
    double dist(const prova::loga::collection& collection, std::size_t i, std::size_t j) const;
    // double dist_neighbourhood(std::size_t i, std::size_t j) const;
    // double dist_sequential(std::size_t i, std::size_t j) const;
    void compute(const prova::loga::collection& collection, std::size_t threads = 0);
    std::ostream& print_graphml(std::ostream& stream) const;
    bool computed() const;
public:
    const distance_matrix_type& matrix() const;
};

}

#endif // PROVA_ALIGN_DISTANCE_H
