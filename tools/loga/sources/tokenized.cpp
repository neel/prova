#include <loga/token.h>
#include <loga/collection.h>
#include <loga/tokenized_collection.h>
#include <loga/tokenized_alignment.h>
#include <loga/alignment.h>
#include <loga/tokenized_distance.h>
#include <loga/cluster.h>
#include <loga/path.h>
#include <loga/tokenized_group.h>
#include <loga/group.h>
#include <loga/tokenized_multi_alignment.h>
#include <loga/multi_alignment.h>
#include <iostream>
#include <filesystem>
#include <cereal/archives/portable_binary.hpp>
#include <igraph/igraph.h>
#include <boost/program_options.hpp>

class parsed{
    std::size_t _id;
    std::size_t _cluster;
    prova::loga::tokenized_multi_alignment::interval_set _intervals;

public:
    inline explicit parsed(std::size_t id, std::size_t cluster, prova::loga::tokenized_multi_alignment::interval_set&& intervals): _id(id), _cluster(cluster), _intervals(std::move(intervals)) {}
    const prova::loga::tokenized_multi_alignment::interval_set& intervals() const { return _intervals; }
    std::size_t id() const { return _id; }
    std::size_t cluster() const { return _cluster; }
};

int main(int argc, char** argv) {
    boost::program_options::variables_map vm;
    try{
        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
            ("help,h",            "Print help message")
            ("input",             boost::program_options::value<std::string>(),                "Log file path")
        ;
        boost::program_options::positional_options_description p;
        p.add("input", 1);

        auto options = boost::program_options::command_line_parser(argc, argv).options(desc).positional(p).run();
        boost::program_options::store(options, vm);
        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << "Usage: " << argv[0] << " <path>\n";
            std::cout << desc << "\n";
            return 0;
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    if (!vm.count("input")) {
        throw boost::program_options::error("missing required positional argument <path>");
    }

    prova::loga::tokenized_collection collection;
    std::string log_path = vm["input"].as<std::string>();

    std::string archive_file_path = std::format("{}.1g.paths",    log_path);
    std::string dist_file_path    = std::format("{}.lev.dmat",    log_path);
    std::string graphml_file_path = std::format("{}.1nn.graphml", log_path);

    std::ifstream log(log_path);
    collection.parse(log);

    prova::loga::tokenized_alignment::matrix_type all_paths;
    if(std::filesystem::exists(archive_file_path)){
        std::ifstream archive_file(archive_file_path);
        cereal::PortableBinaryInputArchive archive(archive_file);
        archive(all_paths);
    } else {
        std::ofstream archive_file(archive_file_path);
        cereal::PortableBinaryOutputArchive archive(archive_file);
        archive(all_paths);
    }
    prova::loga::tokenized_distance distance(all_paths, collection.count());
    if(std::filesystem::exists(dist_file_path)){
        std::ifstream dist_file(dist_file_path);
        if(!distance.load(dist_file)){
            std::cout << "failed to load dmat" << std::endl;
            return 1;
        }
    } else {
        distance.compute(collection);
        std::ofstream dist_file(dist_file_path);
        if(!distance.save(dist_file)){
            std::cout << "failed to save dmat" << std::endl;
            return 1;
        }
    }
    prova::loga::tokenized_distance::distance_matrix_type dmat = distance.matrix();
    assert(dmat.n_rows == collection.count());
    assert(dmat.n_cols == collection.count());

    auto bgl_graph = distance.knn_graph(1, false);
    {
        std::ofstream graphml(graphml_file_path);
        prova::loga::print_graphml(bgl_graph, graphml);
    }
    igraph_t igraph;
    igraph_vector_t edges;
    prova::loga::bgl_to_igraph(bgl_graph, &igraph, &edges);
    prova::loga::cluster::labels_type labels(collection.count());
    prova::loga::leiden_membership(&igraph, &edges, labels);

    const std::array<std::string, 20> fruits = {
        "Apple", "Banana", "Orange", "Mango", "Grape", "Pineapple", "Strawberry", "Cherry", "Peach", "Pear",
        "Watermelon", "Papaya", "Coconut", "Lemon", "Lime", "Kiwi", "Plum", "Apricot", "Pomegranate", "Guava"
    };

    std::map<std::size_t, parsed> parsed_log;
    prova::loga::tokenized_group group(collection, labels);
    std::cout << "Clusters: " << group.labels() << std::endl;
    for(std::size_t c = 0; /*c != group.labels()*/; ++c) {
        if(c == group.labels()) {
            if(group.unclustered() == 0){
                break;
            } else {
                prova::loga::tokenized_group::label_proxy proxy = group.proxy(std::numeric_limits<std::size_t>::max());
                std::cout << std::format("Unlabeled: ({})", proxy.count()) << std::endl;
                for(std::size_t i = 0; i != proxy.count(); ++i) {
                    auto v = proxy.at(i);
                    std::cout << v.str() << std::endl;
                }
                break;
            }
        }
        prova::loga::tokenized_group::label_proxy proxy = group.proxy(c);
        std::size_t count = proxy.count();

        std::string cluster_name = (c < std::numeric_limits<std::size_t>::max()) ? ((c < fruits.size()) ? fruits.at(c) : std::format("C{}", c)) : " Failed ";
        std::cout << "Label: " << cluster_name << std::format(" ({})", count) << std::endl;
        // std::cout << std::resetiosflags(std::ios::showbase) << std::right << std::setw(3) << "*" << min_matched_id << "|" << "\033[4m" << collection.at(min_matched_id) << "\033[0m" << std::resetiosflags(std::ios::showbase) << std::endl;

        prova::loga::tokenized_collection subcollection;
        std::vector<std::size_t> references;
        for(std::size_t i = 0; i < count; ++i) {
            prova::loga::tokenized_group::label_proxy::value v = proxy.at(i);
            const std::string& str = v.str();
            subcollection.add(str);
            references.push_back(v.id());
        }

        std::vector<std::string> outliers;
        std::vector<double> confidence;
        arma::vec lof(count, arma::fill::none);
        if(count > 2){
            std::size_t K = std::min<std::size_t>(3, count -1);
            arma::mat local_distances(count, count, arma::fill::zeros);
            arma::vec kdist(count, arma::fill::none);
            for (std::size_t i = 0; i < count; ++i) {
                for (std::size_t j = i + 1; j < count; ++j) {
                    const std::string& si = subcollection.at(i).raw();
                    const std::string& sj = subcollection.at(j).raw();

                    // std::size_t lcs_len  = prova::loga::lcs(si.cbegin(), si.cend(), sj.cbegin(), sj.cend());
                    // std::size_t lcs_dist = std::max(si.size(), sj.size()) - lcs_len;
                    // local_distances(i,j) = local_distances(j,i) = lcs_dist;

                    // local_distances(i,j) = local_distances(j,i) = dmat(i, j);

                    std::size_t lev_dist = prova::loga::levenshtein_distance(si.cbegin(), si.cend(), sj.cbegin(), sj.cend());
                    local_distances(i,j) = local_distances(j,i) = lev_dist;// /std::max(si.size(), sj.size());
                }
                arma::rowvec sorted = arma::sort(local_distances.row(i), "ascend");
                kdist(i) = sorted(K);
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

            // std::cout << lof << std::endl;
            std::vector<std::size_t> excluded;
            for (arma::uword i = 0; i < count; ++i) {
                if(lof(i) >= 1.1f){
                    excluded.push_back(i);
                }
                confidence.push_back(lof(i));
            }

            for(auto it = excluded.rbegin(); it != excluded.rend(); ++it) {
                std::size_t index = *it;
                outliers.push_back(subcollection.at(index).raw());
                subcollection.remove(index);
                references.erase(references.begin() + index);
                confidence.erase(confidence.begin() + index);
            }
        }

        std::size_t base = 0;
        std::size_t cache_hits = 0;
        prova::loga::tokenized_alignment::matrix_type paths;
        std::size_t ref_c_i = 0;
        for(auto ref_i = references.cbegin(); ref_i != references.cend(); ++ref_i) {
            std::size_t ref_c_j = 0;
            for(auto ref_j = references.cbegin(); ref_j != references.cend(); ++ref_j) {
                if(ref_i != ref_j){
                    auto global_key = std::make_pair(*ref_i, *ref_j);
                    auto it = all_paths.find(global_key);
                    if(it != all_paths.cend()) {
                        auto local_key = std::make_pair(ref_c_i, ref_c_j);
                        paths.insert(std::make_pair(local_key, it->second));
                        ++cache_hits;
                    }
                }
                ++ref_c_j;
            }
            ++ref_c_i;
        }
        std::cout << std::format("Cache hits {}/{}", cache_hits, (references.size() * references.size()) - references.size()) << std::endl;

        prova::loga::tokenized_alignment subalignment(subcollection);
        subalignment.bubble_all_pairwise(paths, subcollection.begin(), 1);

        for(const auto& [key, path]: paths) {
            auto global_key = std::make_pair(references.at(key.first), references.at(key.second));
            if(!all_paths.contains(global_key)) {
                all_paths.insert(std::make_pair(global_key, path));
            }
        }
        {
            std::ofstream archive_file(archive_file_path);
            cereal::PortableBinaryOutputArchive archive(archive_file);
            archive(all_paths);
        }

        prova::loga::tokenized_multi_alignment malign(subcollection, paths, base);
        prova::loga::tokenized_multi_alignment::region_map regions = malign.align();
        // regions = malign.fixture_word_boundary(regions);
        // malign.print_regions_string(regions, std::cout) << std::endl;
        for(const auto& [id, zones]: regions) {
            std::cout << std::right << std::setw(5) << id;
            if(count > 2) {
                std::cout << ":" << std::fixed << std::setprecision(2) << std::abs(confidence.at(id) - 1.0f);
            }
            std::cout << "|" << std::resetiosflags(std::ios::showbase);
            prova::loga::tokenized_multi_alignment::print_interval_set(zones, subcollection.at(id), std::cout);
            std::cout << std::endl;
        }

        if(outliers.size() > 0){
            std::cout << prova::loga::colors::bright_red << "    Excluded " << outliers.size() << std::fixed << std::setprecision(2) << lof.t() << prova::loga::colors::reset << std::endl;
            for(const std::string& outlier: outliers){
                std::cout << prova::loga::colors::bright_red << "    X| " << prova::loga::colors::reset << outlier << std::endl;
            }
            std::cout << std::endl;
        }

        for(auto& [id, zones]: regions) {
            parsed p(id, c, std::move(zones));
            parsed_log.emplace(std::make_pair(id, p));
        }
    }
    return 0;
}
