#include "prova/alignment.h"
#include "prova/multi_alignment.h"
#include <iostream>

#include <boost/icl/interval.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/separate_interval_set.hpp>

// struct matched_val{
//     std::size_t id;
//     std::size_t ref_pos;
//     std::size_t base_pos;

//     bool operator<(const matched_val& other) const {
//         return id < other.id;
//     }

//     bool operator==(const matched_val& other) const {
//         return id == other.id && ref_pos == other.ref_pos && base_pos == other.base_pos;
//     }
// };

// struct zone{
//     bool        _constant;
//     std::size_t _begin;
//     std::size_t _end;

//     inline zone(std::size_t begin, std::size_t end, bool constant = true): _begin(begin), _end(end), _constant(constant) {}
//     inline std::size_t begin() const { return _begin; }
//     inline std::size_t end() const { return _end; }
//     inline std::size_t length() const { return _end - _begin; }
//     inline bool constant() const { return _constant; }
// };

// inline std::ostream& operator<<(std::ostream& stream, const zone& z) {
//     stream << "[";
//     if(z.constant())
//         stream << "* ";
//     stream << z.begin() << "," << z.end() << "]";
//     return stream;
// }

// enum class zone{ constant, placeholder };
// inline std::ostream& operator<<(std::ostream& stream, const zone& z) {
//     if(z == zone::constant) {
//         stream << "C";
//     } else {
//         stream << "P";
//     }
//     return stream;
// }



int main() {
    prova::align::alignment alignment;
    alignment.add("Hello W Here I am do you hear me 579");
    alignment.add("J Here Jx am do you hear me 18303");
    alignment.add("Hola W Here We are do you hear me");

    prova::align::alignment::matrix_type matrix;
    alignment.bubble_all_pairwise(matrix, 2);
    prova::align::multi_alignment malign(alignment.inputs(), matrix, 0);
    prova::align::multi_alignment::region_map regions = malign.align();

    malign.print_regions(regions, std::cout) << std::endl;
    std::cout << "-----" << std::endl;
    regions = malign.fixture_word_booundary(regions);
    malign.print_regions(regions, std::cout) << std::endl;

    // for(auto& candidate: regions) {
    //     for(const auto& z: candidate.second) {
    //         prova::align::zone tag = *z.second.cbegin();
    //         const std::string& ref = alignment.inputs().at(candidate.first);
    //         std::cout << z.first << " <" << ref.substr(z.first.lower(), z.first.upper()-z.first.lower()) << "> " << tag << std::endl;
    //     }

    //     std::cout << std::endl;
    // }
}

// int main2() {
//     prova::align::alignment alignment;
//     // alignment.add("[Sun Dec 04 04:47:44 2005] [notice] workerEnv.init() ok /etc/httpd/conf/workers2.properties");
//     // alignment.add("[Sun Dec 04 04:51:14 2005] [notice] workerEnv.init() ok /etc/httpd/conf/workers2.properties");
//     // alignment.add("[Sun Dec 04 04:51:52 2005] [notice] workerEnv.init() ok /etc/httpd/conf/workers2.properties");
//     // alignment.add("[Sun Dec 04 04:52:12 2005] [notice] workerEnv.init() ok /etc/httpd/conf/workers2.properties");

//     // alignment.add("ABCDEFdhweuxnhue7896543");
//     // alignment.add("ABC123DEFada7896543ddadaada");
//     // alignment.add("ABC1234567DEFdwdwwdwdwwdwdwdw789654");
//     // alignment.add("ABCDEF04558sou78654");

//     // alignment.add("ABCDEFG89634146EFG89");
//     // alignment.add("ABCDEFG66EFG89");
//     // alignment.add("ABCDEFG6EFG89");

//     alignment.add("Hello W Here I am do you hear me 579");
//     alignment.add("J Here Jx am do you hear me 18303");
//     alignment.add("Hola W Here We are do you hear me");

//     // prova::align::graph graph = alignment.bubble_all(2);

//     // for(const auto& segment: graph) {
//     //     std::cout << segment << std::endl;
//     // }

//     // graph.build();
//     // std::ofstream graphml{"out.graphml"};
//     // graph.print(graphml);

//     // std::cout << "Shortest Path" << std::endl;
//     // prova::align::path path = graph.shortest_path();
//     // path.print(std::cout) << std::endl;

//     std::size_t base_index = 0;

//     prova::align::alignment::matrix_type matrix;
//     alignment.bubble_all_pairwise(matrix, 2);

//     using interval_val  = std::set<matched_val>;
//     using interval_map  = boost::icl::split_interval_map<std::size_t, interval_val>;
//     using interval_set  = boost::icl::split_interval_map<std::size_t, std::set<zone>>;
//     // using interval_set  = std::vector<zone>;
//     // using region_type   = interval_set::interval_type;
//     using region_type   = interval_set::interval_type;
//     using interval_type = interval_map::interval_type;
//     interval_map intervals;

//     for(const auto& [key, path]: matrix) {
//         if(key.first != base_index) continue;
//         std::cout << std::format("({},{})", key.first, key.second) << "| ";
//         for(const auto& s: path){
//             prova::align::index start = s.start();
//             std::size_t start_pos = start.at(0);
//             std::size_t end_pos   = start_pos + s.length();
//             interval_type::type interval = interval_type::right_open(start_pos, end_pos);
//             interval_val val;
//             matched_val matched;
//             matched.id = key.second;
//             matched.base_pos = start_pos;
//             matched.ref_pos = start.at(1);
//             val.insert(matched);
//             intervals.add(std::make_pair(interval, val)); //
//             std::cout << std::format("[{}, {})", start_pos, end_pos) << "-" << s.start();
//             // std::cout << s << "~~~";
//         }
//         std::cout << std::endl;
//     }

//     using region_map = std::map<std::size_t, interval_set>;

//     region_map regions;

//     for(std::size_t i = 0; i != alignment.inputs().count(); ++i) {
//         regions.insert(std::make_pair(i, interval_set{}));
//     }

//     const std::string& base_ref = alignment.inputs().at(base_index);
//     // auto base_begin = base_ref.begin();
//     for(const auto& iv: intervals) {
//         if(iv.second.size() == alignment.inputs().count()-1) {
//             std::size_t len = iv.first.upper() - iv.first.lower();
//             // auto start = base_begin+iv.first.lower();
//             // auto end   = start + len;
//             // std::string_view base_view(start, end);
//             // std::cout << iv.first << ": <" << base_view << ">" << std::endl;
//             // regions[base_index].add(region_type::right_open(iv.first.lower(), iv.first.lower() +len));
//             std::set<zone> zones;
//             zones.insert(zone::constant);
//             regions[base_index].add(std::make_pair(region_type::closed(iv.first.lower(), iv.first.lower() +len), zones));
//             for(const matched_val& v: iv.second) {
//                 // std::cout << "\t" << v.id << "-> " << v.ref_pos << " (" << v.base_pos << ")" << std::endl;
//                 // const std::string& ref = alignment.inputs().at(v.id);
//                 std::size_t delta = iv.first.lower() - v.base_pos;
//                 std::size_t ref_start = v.ref_pos+delta;
//                 std::size_t ref_end   = delta+v.ref_pos+len;
//                 // std::string_view ref_view(ref.begin()+ref_start, ref.begin()+ref_end);
//                 // std::cout << "\t<" << ref_view << ">" << std::endl;
//                 // regions[v.id].add(region_type::closed(ref_start, ref_end));
//                 std::set<zone> zones;
//                 zones.insert(zone::constant);
//                 regions[v.id].add(std::make_pair(region_type::closed(ref_start, ref_end), zones));
//             }
//         }
//     }

//     for(auto& candidate: regions) {
//         std::cout << candidate.first << " {" << candidate.second.size() << "}" << std::endl;
//         std::size_t last = 0;
//         std::vector<region_type> placeholders;
//         for(const auto& z: candidate.second) {
//             if(z.first.lower() > last) {
//                 placeholders.push_back(region_type::closed(last, z.first.upper()));
//             }
//             last = z.first.upper()+1;
//         }
//         std::size_t end = alignment.inputs().at(candidate.first).size();
//         if(end > last) {
//             placeholders.push_back(region_type::closed(last, end));
//         }


//         std::set<zone> zones;
//         zones.insert(zone::placeholder);
//         for(const auto& p: placeholders) {
//             candidate.second.add(std::make_pair(p, zones));
//         }

//         for(const auto& z: candidate.second) {
//             zone tag = *z.second.cbegin();
//             const std::string& ref = alignment.inputs().at(candidate.first);
//             std::cout << z.first << " <" << ref.substr(z.first.lower(), z.first.upper()-z.first.lower()) << "> " << tag << std::endl;
//         }

//         std::cout << std::endl;
//     }



//     return 0;
// }
