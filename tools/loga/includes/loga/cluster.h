#ifndef PROVA_ALIGN_CLUSTER_H
#define PROVA_ALIGN_CLUSTER_H

#include <armadillo>

namespace prova::loga {

class cluster{
public:
    using distance_matrix_type = arma::mat;
    using labels_type          = arma::Row<std::size_t>;
private:
    const distance_matrix_type& _distances;
    labels_type _labels;
public:
    explicit cluster(const distance_matrix_type& distances);
    void compute(double eps = 0.35, std::size_t minPts = 3, std::size_t threads = 0);
    const labels_type& labels() const;
    bool computed() const;
public:
    const distance_matrix_type& matrix() const;
};


}

#endif // PROVA_ALIGN_CLUSTER_H
