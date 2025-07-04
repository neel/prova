#include <iostream>
#include <cstdint>
#include <map>
#include <memory>
#include <functional>
#include <filesystem>
#include <fstream>
#include "prova/trace_parser.h"
#include <boost/program_options.hpp>

struct operations{
    bool distance = false;
    bool cluster  = false;
    bool align    = false;
    bool pairwise = false;

    inline bool none() const { return !distance && !cluster && !align && !pairwise; }
    inline bool multiple() const { return distance || cluster || align; }
};

int main(int argc, char* argv[]){
    boost::program_options::options_description desc("Allowed options");

    desc.add_options()
        ("help,h", "print this help message")

        ("distance,d",  boost::program_options::bool_switch()->default_value(false), "calculate distance matrix")
        ("cluster,c",   boost::program_options::bool_switch()->default_value(false), "perform clustering (may use --eps / --minpts)")
        ("align,a",     boost::program_options::bool_switch()->default_value(false), "perform alignment")

        ("eps",         boost::program_options::value<double>(), "DBSCAN ε radius (requires -c / --cluster)")
        ("minpts",      boost::program_options::value<int>(),    "DBSCAN min-pts (requires -c / --cluster)")

        ("status,s",    boost::program_options::value<std::filesystem::path>(), "status directory")

        ("pairwise, p", boost::program_options::value< std::vector<std::string> >()->multitoken(), "exactly two quoted strings")

        ("input",       boost::program_options::value<std::filesystem::path>(), "input file (for -d) *or* working directory (for -c / -a)")
        ("output,o",       boost::program_options::value<std::filesystem::path>(), "output directory (default: ./out)");

    boost::program_options::positional_options_description pos;
    pos.add("input", 1);
    pos.add("output", 1);

    boost::program_options::variables_map vm;

    try {
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).positional(pos).style(boost::program_options::command_line_style::unix_style).run(), vm);
        if (vm.count("help")) {
            std::cout << desc << '\n';
            return 0;
        }
        boost::program_options::notify(vm);

        operations op;
        op.distance = vm["distance"].as<bool>();
        op.cluster  = vm["cluster"].as<bool>();
        op.align    = vm["align"].as<bool>();
        op.pairwise = vm.count("pairwise");

        if(op.none()) {
            std::cerr << "Error: no action requested (use -d, -c, -a or --pairwise).\n";
            return 1;
        }

        if(op.multiple() && !vm.count("input")) {
            std::cerr << "Error: missing positional <input> (file or directory).\n";
            return 1;
        }

        if(op.pairwise) {
            const std::vector<std::string>& p = vm["pairwise"].as< std::vector<std::string> >();
            if (p.size() != 2) {
                std::cerr << "Error: --pairwise expects exactly two quoted strings.\n";
                return 1;
            } else {
                trace_parser parser;
                auto alignment = parser.align(p[0], p[1]);
                alignment.apply(std::cout, p[0]) << std::endl;

                return 0;
            }
        }

        std::filesystem::path output;
        if(vm.count("output")) {
            output = vm["output"].as<std::filesystem::path>();
        } else {
            output = vm["input"].as<std::filesystem::path>().string()+"_d";
        }
        if(!std::filesystem::is_directory(output)) {
            std::filesystem::create_directory(output);
        }

        bool loaded = false;
        trace_parser parser;
        if(op.distance || op.cluster || op.align) {
            std::filesystem::path input = vm["input"].as<std::filesystem::path>();
            if (!std::filesystem::is_regular_file(input)) {
                std::cerr << "Error: " << input << " is not a readable file." << std::endl;
                return 1;
            } else {
                // parse input -> compute distance matrix -> store output/distance.matrix
                parser.parse(input);
                if(op.distance) {
                    parser.compute();
                    loaded = true;
                }
            }
        }

        if(op.cluster || op.align) {
            if(!loaded) {
                std::filesystem::path matrix_path = output / "distances.bin";
                if(std::filesystem::is_directory(output) && std::filesystem::is_regular_file(matrix_path)) {
                    parser.load(output);
                    loaded = true;
                } else {
                    std::cerr << "Error: " << "Failed to load matrix from " << matrix_path << std::endl;
                    return 1;
                }
            }
        }

        if(op.cluster) {
            double eps = 0.20;
            std::size_t minPts = 2;

            if(vm.count("eps")) {
                eps = vm["eps"].as<double>();
            }
            if(vm.count("minpts")) {
                minPts = vm["minPts"].as<double>();
            }
            parser.cluster(eps, minPts);
        }

        if(op.align) {
            for(auto i = 0; i < parser.cluster_count(); ++i) {
                std::vector<std::vector<zone>> zones;
                trace_parser::graph_type malignment = parser.align(i, zones);
                trace_parser::adjust(malignment, zones);
                std::cout << std::endl << std::format("cluster {} ", i) << std::endl << std::endl;
                malignment.apply(std::cout) << std::endl;
                std::cout << std::endl;
                parser.print_aligned(i, std::cout, zones);
                std::cout << std::endl;
                parser.save_alignments(output, i, malignment, zones);
            }
        }

        parser.save(output);
        return 0;
    } catch (const boost::program_options::error& ex) {
        std::cerr << "Command-line error: " << ex.what() << '\n';
        return 1;
    }
    return 0;

}
