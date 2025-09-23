#include <loga/distance.h>
#include <thread>
#include <boost/asio.hpp>
#include <syncstream>

prova::loga::distance::distance(const path_matrix_type &paths, std::size_t count): _paths(paths), _count(count) {}

double prova::loga::distance::dist(std::size_t i, std::size_t j) const{
    if(i == j) {
        return 0.0;
    } else {
        key_type key{i, j};
        try{
            const prova::loga::path& path = _paths.at(key);
            double score = path.score();
            return 1-score;
        } catch(const std::out_of_range& ex) {
            std::cout << std::format("key {}x{} out of range", i, j) << std::endl;
            throw ex;
        }
    }
}

void prova::loga::distance::compute(std::size_t threads){
    _distances.set_size(_count, _count);
    _distances.zeros();

    unsigned int T = !threads ? std::thread::hardware_concurrency() : threads;

    std::condition_variable observer;
    std::mutex mutex;
    std::atomic_uint32_t jobs_completed = 0;

    std::cout << "Computing distance matrix " << std::format("{}x{}", _count, _count)  << std::endl;

    boost::asio::thread_pool pool(T);
    for (std::size_t i = 0; i < _count; ++i) {
        boost::asio::post(pool, [this, i,  &jobs_completed, &observer]() {
            for (std::size_t j = 0; j < _count; ++j) {
                _distances(i, j) = dist(i, j);
            }
            jobs_completed.fetch_add(1);
            observer.notify_one();
        });
    }

    std::uint32_t printed = 0;
    while (printed < _count) {
        std::unique_lock<std::mutex> lock(mutex);
        observer.wait(lock, [&]{
            return jobs_completed.load() > printed;
        });

        const auto upto = jobs_completed.load();
        while (printed < upto) {
            ++printed;
            std::cout << std::format("\rDistance Matrix rows {}/{}", printed, _count) << std::flush;
        }
    }

    pool.join();
    std::cout << std::endl << std::endl << std::flush;
    std::cout.flush();
}

bool prova::loga::distance::computed() const {
    return _paths.size() > 0;
}

const prova::loga::distance::distance_matrix_type &prova::loga::distance::matrix() const { return _distances; }
