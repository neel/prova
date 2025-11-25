#include <loga/outliers.h>
#include <condition_variable>
#include <boost/asio.hpp>
#include <format>

arma::vec prova::loga::outliers::lof(const prova::loga::tokenized_collection& subcollection) {
    std::size_t count = subcollection.count();
    std::vector<std::string> outliers;
    std::vector<double> confidence;
    arma::vec lof(count, arma::fill::none);
    if(count <= 2){
        lof.fill(0);
        return lof;
    }

    std::size_t K = std::min<std::size_t>(3, count -1);
    arma::mat local_distances(count, count, arma::fill::zeros);
    arma::vec kdist(count, arma::fill::none);
    {
        std::condition_variable observer;
        std::mutex mutex;
        std::atomic_uint32_t jobs_completed = 0;
        boost::asio::thread_pool pool(std::thread::hardware_concurrency());
        for (std::size_t i = 0; i < count; ++i) {
            boost::asio::post(pool, [i,  count, &subcollection, &local_distances, &kdist, K, &jobs_completed, &observer]() {
                for (std::size_t j = i + 1; j < count; ++j) {
                    const std::string& si = subcollection.at(i).raw();
                    const std::string& sj = subcollection.at(j).raw();
                    std::size_t lev_dist = prova::loga::levenshtein_distance(si.cbegin(), si.cend(), sj.cbegin(), sj.cend());
                    local_distances(i,j) = local_distances(j,i) = lev_dist;
                }
                jobs_completed.fetch_add(1);
                observer.notify_one();
            });
        }

        std::uint32_t printed = 0;
        while (printed < count) {
            std::unique_lock<std::mutex> lock(mutex);
            observer.wait(lock, [&]{
                return jobs_completed.load() > printed;
            });

            const auto upto = jobs_completed.load();
            while (printed < upto) {
                ++printed;
                double percent = ((double)printed / (double)count) * 10.0f;
                std::string progress(20, '|');
                std::fill(progress.begin()+(((int)percent)*2), progress.end(), '~');
                std::cout << std::format("\r{:.2f}% {}", percent*10, progress) << std::flush;
            }
        }

        pool.join();
        std::cout << std::endl << std::endl << std::flush;
        std::cout.flush();
    }
    {
        boost::asio::thread_pool pool(std::thread::hardware_concurrency());
        for (std::size_t i = 0; i < count; ++i) {
            boost::asio::post(pool, [i, K, &local_distances, &kdist]() {
                arma::rowvec sorted = arma::sort(local_distances.row(i), "ascend");
                kdist(i) = sorted(K);
            });
        }
        pool.join();
    }

    arma::field<arma::uvec> neighbours(count);  // each entry can have a different length
    for (std::size_t i = 0; i < count; ++i) {
        arma::uvec idx = arma::find(local_distances.row(i).t() <= kdist(i));
        idx = idx(arma::find(idx != i));
        neighbours(i) = idx;
        assert(neighbours(i).n_elem >= K);
    }

    auto reach = [&](std::size_t x, std::size_t y) {
        return std::max(kdist(y), local_distances(x, y));
    };

    arma::vec lrd(count, arma::fill::none);
    for (arma::uword i = 0; i < count; ++i) {
        const arma::uvec& locals_i = neighbours(i);
        arma::vec r(locals_i.n_elem);
        for (std::size_t t = 0; t < locals_i.n_elem; ++t)
            r(t) = reach(i, locals_i(t));

        double mrd = arma::mean(r);
        lrd(i) = 1.0 / std::max(mrd, std::numeric_limits<double>::epsilon());
    }

    for (arma::uword i = 0; i < count; ++i) {
        const arma::uvec& N = neighbours(i);
        double avgNbr = arma::mean(lrd(N));
        lof(i) = avgNbr / lrd(i);
    }

    return lof;
}

