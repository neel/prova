#include <loga/tokenized_alignment.h>
#include <loga/segment.h>
#include <loga/graph.h>
#include <loga/path.h>
#include <thread>
#include <boost/asio.hpp>
#include <array>

std::uint64_t prova::loga::tokenized_alignment::pair_hash::operator()(const key_type &key) const noexcept {
    std::uint64_t res = key.first;
    res = std::rotl(res, 32) + key.second;
    return res;
}


// Iterative: least-significant index 0 rolls fastest.
static void enumerate_mixed_radix(const std::vector<std::size_t>& L, std::size_t j, const std::function<void(std::vector<std::size_t>)>& visit) {
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

prova::loga::tokenized_alignment::tokenized_alignment(const tokenized_collection &collection): _collection(collection) {

}

const prova::loga::tokenized_collection &prova::loga::tokenized_alignment::inputs() const {
    return _collection;
}

void prova::loga::tokenized_alignment::bubble_pairwise(const_iterator u, const_iterator v, const index &idx, memo_type& memo, std::size_t threshold, std::size_t carry) const {
    assert(idx.count() == 2);
    const prova::loga::wrapped& wu = u->at(idx.at(0));
    const prova::loga::wrapped& wv = v->at(idx.at(1));

    bool color = (wu == wv);

    if(!color && wu.category() == wv.category()) {
        // Problem:
        //     lhs: db52/bn544 -> w2 d2 @1 w2 d3 ->    w2 d2 @1 w2 d3     -> w2 d2    /  @1  \  w2 d3
        //     rhs: #82#/#943# -> @1 d2 @3 d3 @1 -> @1 d2 @1 @1 @1 d3 @1  -> @1 d2   / @1@1@1 \ d3 @1
        // Solution: LCS
        //     wu: /
        //     wv: #/#
        //     wt: _/#
    }

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


void prova::loga::tokenized_alignment::bubble_all_pairwise(matrix_type &mat, const_iterator base, std::size_t threshold, std::size_t threads) {
    assert(threshold > 0);
    std::size_t N = 2;

    unsigned int T = !threads ? std::thread::hardware_concurrency() : threads;
    std::mutex mutex;
    boost::asio::thread_pool pool(T);
    std::atomic_uint32_t jobs_completed = 0;
    std::size_t total_jobs = _collection.count() - 1;

    std::size_t u = std::distance(_collection.begin(), base);

    std::size_t v = 0;
    for(auto ref = _collection.begin(); ref != _collection.end(); ++ref) {
        if(base == ref) {
            ++v;
            continue;
        }

        auto key = std::make_pair(u, v);
        auto lambda = [key, base, ref, threshold, this, N, &mutex, u, v, &jobs_completed, total_jobs, &mat](){
            std::vector<std::size_t> L{base->count()-1, ref->count()-1};
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
            segment finish{u, index{{base->count(), ref->count()}}, 0};

            prova::loga::graph graph{std::move(segments), std::move(start), std::move(finish)};
            graph.build();
            prova::loga::path path = graph.shortest_path();

            // std::cout << "score: " << path.score() << std::endl;
            std::lock_guard lock(mutex);
            mat.emplace(key, std::move(path));
            // std::cout << "score: " << mat.at(key).score() << std::endl;
            // std::cout << std::format("\rPairwise {}/{}", ++jobs_completed, total_jobs) << std::flush;
            double percent = ((double)++jobs_completed / (double)total_jobs) * 10.0f;
            std::string progress(20, '=');
            std::fill(progress.begin()+(((int)percent)*2), progress.end(), '-');
            std::cout << std::format("\r{:.2f}% {}", percent*10, progress) << std::flush;
        };

        if(!mat.contains(key)) {
            boost::asio::post(pool, lambda);
        }

        ++v;
    }

    pool.join();
    std::cout << std::endl;
}

void prova::loga::tokenized_alignment::bubble_all_pairwise(matrix_type &mat, std::size_t threshold, std::size_t threads) {
    assert(threshold > 0);
    std::size_t N = 2;

    unsigned int T = !threads ? std::thread::hardware_concurrency() : threads;
    std::mutex mutex;
    boost::asio::thread_pool pool(T);
    std::atomic_uint32_t jobs_completed = 0;
    std::size_t total_jobs = _collection.count() * _collection.count() - _collection.count();

    std::size_t u = 0;
    for(auto base = _collection.begin(); base != _collection.end(); ++base) {
        std::size_t v = 0;
        for(auto ref = _collection.begin(); ref != _collection.end(); ++ref) {
            if(base == ref) {
                ++v;
                continue;
            }

            auto key = std::make_pair(u, v);
            auto lambda = [key, base, ref, threshold, this, N, &mutex, u, v, &jobs_completed, total_jobs, &mat](){
                std::vector<std::size_t> L{base->count()-1, ref->count()-1};
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
                segment finish{u, index{{base->count(), ref->count()}}, 0};

                prova::loga::graph graph{std::move(segments), std::move(start), std::move(finish)};
                graph.build();
                prova::loga::path path = graph.shortest_path();

                // std::cout << "score: " << path.score() << std::endl;
                std::lock_guard lock(mutex);
                mat.emplace(key, std::move(path));
                // std::cout << "score: " << mat.at(key).score() << std::endl;
                // std::cout << std::format("\rPairwise {}/{}", ++jobs_completed, total_jobs) << std::flush;
                double percent = ((double)++jobs_completed / (double)total_jobs) * 10.0f;
                std::string progress(20, '=');
                std::fill(progress.begin()+(((int)percent)*2), progress.end(), '-');
                std::cout << std::format("\r{:.2f}% {}", percent*10, progress) << std::flush;
            };

            if(!mat.contains(key)) {
                boost::asio::post(pool, lambda);
            }

            ++v;
        }
        ++u;
    }

    pool.join();
    std::cout << std::endl;
}

