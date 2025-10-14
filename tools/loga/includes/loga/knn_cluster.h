#ifndef PROVA_ALIGN_KNN_CLUSTER_H
#define PROVA_ALIGN_KNN_CLUSTER_H

#include <armadillo>

namespace prova::loga {

class knn_cluster{
public:
    using distance_matrix_type = arma::mat;
    using labels_type          = arma::Row<std::size_t>;
private:
    const distance_matrix_type& _distances;
    labels_type _labels;
public:
    explicit knn_cluster(const distance_matrix_type& distances);
    void compute(std::size_t threads = 0);
    const labels_type& labels() const;
    bool computed() const;
public:
    const distance_matrix_type& matrix() const;
};

}

#endif // PROVA_ALIGN_KNN_CLUSTER_H
