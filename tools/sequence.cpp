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

    auto malignment = parser.align(0);
    malignment.apply(std::cout) << std::endl;

    return 0;
}
