#include <loga/alignment.h>
#include <loga/collection.h>
#include <loga/segment.h>
#include <loga/graph.h>
#include <loga/path.h>
#include <thread>
#include <boost/asio.hpp>

prova::loga::alignment::alignment(const collection &collection): _collection(collection) {}

const prova::loga::collection &prova::loga::alignment::inputs() const { return _collection; }

void prova::loga::alignment::bubble(const prova::loga::index& idx, std::size_t threshold, std::size_t carry) {
    bool color = _collection.unanimous_concensus(idx);
    if (color) {
        if(!idx.is_top()) {
            bubble(idx.top_left(), threshold, carry + 1);
        } else {
            if(carry >= threshold-1) {
                _memo[idx] = carry + 1;
            }
        }
    } else {
        if (carry >= threshold) {
            _memo[idx.bottom_right()] = carry;
        }
        if(!idx.is_top()) {
            bubble(idx.top_left(), threshold, 0);
        }
    }
}

void prova::loga::alignment::bubble_pairwise(const_iterator u, const_iterator v, const index &idx, memo_type& memo, std::size_t threshold, std::size_t carry) const {
    assert(idx.count() == 2);
    bool color = (u->at(idx.at(0)) == v->at(idx.at(1)));
    if (color) {
        if(!idx.is_top()) {
            bubble_pairwise(u, v, idx.top_left(), memo, threshold, carry + 1);
        } else {
            if(carry >= threshold-1) {
                memo[idx] = carry + 1;
            }
        }
    } else {
        if (carry >= threshold) {
            memo[idx.bottom_right()] = carry;
        }
        if(!idx.is_top()) {
            bubble_pairwise(u, v, idx.top_left(), memo, threshold, 0);
        }
    }
}

// Iterative: least-significant index 0 rolls fastest.
void enumerate_mixed_radix(const std::vector<std::size_t>& L, std::size_t j, const std::function<void(std::vector<std::size_t>)>& visit) {
    const std::size_t N = L.size();
    std::vector<std::size_t> x(N, 0);
    x[j] = L[j];

    while (true) {
        visit(x);

        std::size_t i = 0;
        while (i < N && x[i] == L[i]) {
            if(i != j) x[i] = 0;
            ++i;
        }
        if (i == N) {
            break; // finished
        }
        if(i != j)
            ++x[i];
    }
}

prova::loga::graph prova::loga::alignment::bubble_all(std::size_t threshold, std::size_t threads) {
    assert(threshold > 0);
    std::size_t N = _collection.count();
    std::vector<std::size_t> L;
    L.reserve(N);
    std::transform(_collection.begin(), _collection.end(), std::back_inserter(L), [](const std::string& str){
        return str.size()-1;
    });

    unsigned int T = !threads ? std::thread::hardware_concurrency() : threads;
    std::mutex mutex;
    boost::asio::thread_pool pool(T);
    std::atomic_uint32_t jobs_completed = 0;
    std::size_t total_jobs = _collection.count() * _collection.count();


    for(std::size_t j = 0; j < N; ++j) {
        auto lambda = [threshold, this, N, &mutex, &jobs_completed, total_jobs, j, &L](){
            enumerate_mixed_radix(L, j, [threshold, this](std::vector<std::size_t> x){
                // std::cout << "[";
                // std::ranges::copy(x, std::ostream_iterator<std::size_t>(std::cout, ","));
                // std::cout << "]" << std::endl;
                bubble(index{std::move(x)}, threshold, 0);
            });
            std::lock_guard lock(mutex);
            std::cout << std::format("\rJobs {}/{}", jobs_completed++, total_jobs) << std::flush;
        };
        // boost::asio::post(pool, lambda);
        lambda();
    }

    pool.join();
    std::cout << std::endl;

    segment_collection_type segments;
    for(const auto& [idx, length]: _memo) {
        segments.emplace_back(segment{0, idx, length});
    }

    segment start{0, index{_collection.count()}, 0};

    std::vector<std::size_t> last_indices;
    std::transform(_collection.begin(), _collection.end(), std::back_inserter(last_indices), [](const std::string& str){
        return str.size();
    });

    segment finish{0, index{std::move(last_indices)}, 0};

    return prova::loga::graph{std::move(segments), std::move(start), std::move(finish)};
}

void prova::loga::alignment::bubble_all_pairwise(prova::loga::alignment::matrix_type& mat, std::size_t threshold, std::size_t threads){
    assert(threshold > 0);
    std::size_t N = 2;

    unsigned int T = !threads ? std::thread::hardware_concurrency() : threads;
    std::mutex mutex;
    boost::asio::thread_pool pool(T);
    std::atomic_uint32_t jobs_completed = 0;
    std::size_t total_jobs = _collection.count() * _collection.count();

    std::size_t u = 0;
    for(auto base = _collection.begin(); base != _collection.end(); ++base) {
        std::size_t v = 0;
        for(auto ref = _collection.begin(); ref != _collection.end(); ++ref) {
            if(base == ref) {
                ++v;
                continue;
            }

            auto lambda = [base, ref, threshold, this, N, &mutex, u, v, &jobs_completed, total_jobs, &mat](){
                std::vector<std::size_t> L{base->size()-1, ref->size()-1};
                memo_type memo;

                for(std::size_t j = 0; j < N; ++j) {
                    enumerate_mixed_radix(L, j, [threshold, this, base, ref, &memo](std::vector<std::size_t> x){
                        // std::cout << "[";
                        // std::ranges::copy(x, std::ostream_iterator<std::size_t>(std::cout, ","));
                        // std::cout << "]" << std::endl;
                        bubble_pairwise(base, ref, index{std::move(x)}, memo, threshold, 0);
                    });
                }

                segment_collection_type segments;
                for(const auto& [idx, length]: memo) {
                    segments.emplace_back(segment{u, idx, length});
                }
                segment start{u, index{2}, 0};
                segment finish{u, index{{base->size(), ref->size()}}, 0};

                prova::loga::graph graph{std::move(segments), std::move(start), std::move(finish)};
                graph.build();
                prova::loga::path path = graph.shortest_path();

                auto key = std::make_pair(u, v);
                // std::cout << "score: " << path.score() << std::endl;
                std::lock_guard lock(mutex);
                mat.emplace(key, std::move(path));
                // std::cout << "score: " << mat.at(key).score() << std::endl;
                std::cout << std::format("\rPairwise {}/{}", jobs_completed++, total_jobs) << std::flush;
            };

            boost::asio::post(pool, lambda);

            ++v;
        }
        ++u;
    }

    pool.join();
    std::cout << std::endl;
}

std::uint64_t prova::loga::alignment::pair_hash::operator()(const key_type &key) const noexcept {
    std::uint64_t res = key.first;
    res = std::rotl(res, 32) + key.second;
    return res;
}
