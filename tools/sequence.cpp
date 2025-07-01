#include <iostream>
#include <cstdint>
#include <map>
#include <memory>
#include <functional>
#include <filesystem>
#include <fstream>
#include "prova/trace_parser.h"

int main(){
    trace_parser parser;

    std::string a = "[Sun Dec 04 05:15:09 2005] [error] [client 222.166.160.184] Directory index forbidden by rule: /var/www/html/";
    std::string b = "[Sun Dec 04 07:45:45 2005] [error] [client 63.13.186.196] Directory index forbidden by rule: /var/www/html/";
    auto alignment = parser.align(a, b);
    alignment.apply(std::cout, a) << std::endl;

    // parser.parse("Apache_2k.log");

    // parser.compute();
    // std::cout << "Computed" << std::endl;
    // // parser.cluster();

    parser.load("Apache_2k.big");

    // // parser.print(std::cout);

    std::vector<std::vector<zone>> zones;
    trace_parser::graph_type malignment = parser.align(2, zones);
    malignment.apply(std::cout) << std::endl;

    // expand placeholder P
    // aP -> if a is not space and constant then expand P
    //       if a is space and constant then don't
    // Pa -> if a is not space and constant then expand P
    //       if a is space then and constant don't
    // PQ -> if both PQ are placeholders then merge

    // static constexpr char space = ' ';
    // std::string left;
    std::string alphabets = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_:/.";
    for(auto it = malignment.begin(); it != malignment.end(); ++it) {
        auto& [matched, component] = *it;
        if(!matched) {
            assert(std::holds_alternative<placeholder>(component));
            placeholder& p = std::get<placeholder>(component);

            bool starts_with_alphabets = true, ends_with_alphabets = true;
            for(auto& v: p._values) {
                if(starts_with_alphabets && !v.empty() && alphabets.find(v.front()) == std::string::npos){
                    starts_with_alphabets = false;
                }
                if(ends_with_alphabets && !v.empty() && alphabets.find(v.back()) == std::string::npos){
                    ends_with_alphabets = false;
                }
            }

            if(it != malignment.begin() && starts_with_alphabets) {
                auto& [_, previous_component] = *(it-1);
                assert(std::holds_alternative<subsequence>(previous_component));
                subsequence& pre_sub = std::get<subsequence>(previous_component);

                auto pos = pre_sub._str.find_last_not_of(alphabets);
                std::string::size_type start = (pos != std::string::npos) ? pos+1 : 0;

                std::string left_over  = pre_sub._str.substr(0, start);
                std::string carry_over = pre_sub._str.substr(start, pre_sub._str.size() - start);
                pre_sub._str = left_over;

                if(carry_over.size() > 0) {
                    p._range.second += carry_over.size();
                    std::size_t min = p._range.second;
                    std::set<std::string> possibilities;
                    for(auto& v: p._values) {
                        std::string carried_over = carry_over+v;
                        possibilities.insert(carried_over);

                        if(min > carried_over.size()) {
                            min = carried_over.size();
                        }
                    }
                    p._range.first = min;
                    p._values = possibilities;
                }
            }

            if(it != malignment.end() -1 && ends_with_alphabets) {
                auto& [_, next_component] = *(it+1);
                assert(std::holds_alternative<subsequence>(next_component));
                subsequence& next_sub = std::get<subsequence>(next_component);

                auto pos = next_sub._str.find_first_not_of(alphabets);
                std::string::size_type end = (pos != std::string::npos && pos > 0) ? pos : 0;

                std::string left_over  = next_sub._str.substr(end, next_sub._str.size() - end);
                std::string carry_over = next_sub._str.substr(0, end);
                next_sub._str = left_over;

                if(carry_over.size() > 0) {
                    p._range.second += carry_over.size();
                    std::size_t min = p._range.second;
                    std::set<std::string> possibilities;
                    for(auto& v: p._values) {
                        std::string carried_over = v+carry_over;
                        possibilities.insert(carried_over);

                        if(min > carried_over.size()) {
                            min = carried_over.size();
                        }
                    }
                    p._range.first = min;
                    p._values = possibilities;
                }
            }
        }
    }

    trace_parser::graph_type modified_malignment;
    for(auto it = malignment.begin(); it != malignment.end(); ++it) {
        auto& [matched, component] = *it;
        if(matched) {
            assert(std::holds_alternative<subsequence>(component));
            subsequence& sub = std::get<subsequence>(component);
            if(sub.size() > 0) {
                modified_malignment.emplace_back(matched, std::move(component));
            }
        } else {
            modified_malignment.emplace_back(matched, std::move(component));
        }
    }
    malignment = modified_malignment;
    modified_malignment.clear();
    for(auto it = malignment.begin(); it != malignment.end(); ++it) {
        auto& [matched, component] = *it;
        if(!matched) {
            assert(std::holds_alternative<placeholder>(component));
            placeholder& p = std::get<placeholder>(component);

            auto jt = it+1;
            for(; jt != malignment.end(); ++jt) {
                auto& [_, next_component] = *(jt);
                if(std::holds_alternative<placeholder>(next_component)) {
                    placeholder& next_placeholder = std::get<placeholder>(next_component);
                    // merge with p
                    p._range.second += next_placeholder._range.second;
                    std::set<std::string> possibilities;
                    for(auto& u: p._values) {
                        for(auto& v: next_placeholder._values) {
                            possibilities.insert(u+v);
                        }
                    }
                    p._values = possibilities;
                } else {
                    break;
                }
            }
            it = jt-1;

            modified_malignment.emplace_back(matched, placeholder(p));
        } else {
            modified_malignment.emplace_back(matched, std::move(component));
        }
    }

    std::cout << std::endl << std::endl;
    modified_malignment.apply(std::cout) << std::endl;

    return 0;
}
