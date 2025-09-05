#include "prova/algorithms.h"
#include <iostream>
#include <fstream>

int main() {
    prova::algorithms::alignment alignment;
    // alignment.add("[Sun Dec 04 04:47:44 2005] [notice] workerEnv.init() ok /etc/httpd/conf/workers2.properties");
    // alignment.add("[Sun Dec 04 04:51:14 2005] [notice] workerEnv.init() ok /etc/httpd/conf/workers2.properties");
    // alignment.add("[Sun Dec 04 04:51:52 2005] [notice] workerEnv.init() ok /etc/httpd/conf/workers2.properties");
    // alignment.add("[Sun Dec 04 04:52:12 2005] [notice] workerEnv.init() ok /etc/httpd/conf/workers2.properties");

    // alignment.add("ABCDEFdhweuxnhue7896543");
    // alignment.add("ABC123DEFada7896543ddadaada");
    // alignment.add("ABC1234567DEFdwdwwdwdwwdwdwdw789654");
    // alignment.add("ABCDEF04558sou78654");

    // alignment.add("ABCDEFG89634146EFG89");
    // alignment.add("ABCDEFG66EFG89");
    // alignment.add("ABCDEFG6EFG89");

    alignment.add("Hello W Here I am do you hear me");
    alignment.add("J Here Jx am do you hear me");

    prova::algorithms::graph graph = alignment.bubble_all(2);

    for(const auto& segment: graph) {
        std::cout << segment << std::endl;
    }

    graph.build();
    std::ofstream graphml{"out.graphml"};
    graph.print(graphml);

    std::cout << "Shortest Path" << std::endl;
    prova::algorithms::path path = graph.shortest_path();
    path.print(std::cout) << std::endl;

    return 0;
}
