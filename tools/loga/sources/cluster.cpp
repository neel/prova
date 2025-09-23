#include <loga/cluster.h>
#include <mlpack/methods/dbscan/dbscan.hpp>
#include <mlpack/methods/neighbor_search/neighbor_search.hpp>

struct Metric{
    Metric(): _distances(nullptr){}
    inline explicit Metric(const arma::mat& distances) : _distances(&distances) {}
    inline explicit Metric(const Metric& other): _distances(other._distances) {}
    inline double Evaluate(std::size_t a, std::size_t b) const { return (*_distances)(a, b); }
    template<typename VecA, typename VecB>
    double Evaluate(const VecA& a, const VecB& b) const{
        return Evaluate(static_cast<std::size_t>(a[0]), static_cast<std::size_t>(b[0]));
    }

private:
   const arma::mat* _distances;
};

prova::loga::cluster::cluster(const distance_matrix_type &distances): _distances(distances) {}

void prova::loga::cluster::compute(double eps, std::size_t minPts, std::size_t threads) {
    arma::mat coords(1, _distances.n_cols);
    for (arma::uword i = 0; i < coords.n_cols; ++i)
        coords(0, i) = static_cast<double>(i);

    using range_search_type = mlpack::RangeSearch<Metric, arma::mat, mlpack::BallTree>;
    range_search_type rangeSearch(coords, true, false, Metric(_distances));
    mlpack::DBSCAN<range_search_type, mlpack::RandomPointSelection> dbscan(eps, minPts, true, rangeSearch);

    _labels.set_size(coords.n_cols);
    dbscan.Cluster(coords, _labels);
}

const prova::loga::cluster::labels_type &prova::loga::cluster::labels() const{
    return _labels;
}
