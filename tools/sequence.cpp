#include <iostream>
#include <cstdint>
#include <map>
#include <memory>
#include <functional>
#include <filesystem>
#include <fstream>
#include "prova/process.h"
#include "prova/artifact.h"
#include "prova/session.h"
#include "prova/action.h"
#include "prova/store.h"
#include "prova/execution_unit.h"
#include <boost/process.hpp>


int main(){
    prova::store store;
    store.fetch();
    // store.uml(std::cout);

    std::vector<std::shared_ptr<prova::execution_unit>> units;
    store.extract(units);

    std::size_t i = 0;
    for(const auto& unit: units){
        std::filesystem::path uml_path{std::format("{}.uml", i)};
        std::filesystem::path image_path{std::format("{}.svg", i)};
        std::ofstream uml_file{uml_path};
        unit->uml(uml_file);
        uml_file.close();
        std::cout << "UML generated: " << uml_path << std::endl;

        std::stringstream uml_buffer;
        unit->uml(uml_buffer);

        std::ostringstream os;
        boost::process::opstream in_stream;
        boost::process::ipstream out_stream;
        boost::process::child plantuml("java -jar /home/sunanda/Projects/provenance/plantuml.jar -tsvg -pipe", boost::process::std_in < in_stream, boost::process::std_out > out_stream);

        in_stream << uml_buffer.rdbuf();
        in_stream.flush();
        in_stream.pipe().close(); // Close the input stream to signal end of input

        // Read the output from the child process while it is running
        std::ofstream image_file{image_path};
        std::string line;
        while (plantuml.running() && std::getline(out_stream, line)) {
            image_file << line << std::endl;
        }

        // Drain any remaining output after the process has finished
        while (std::getline(out_stream, line)) {
            image_file << line << std::endl;
        }

        // Wait for the child process to finish
        plantuml.wait();

        // Check if the process completed successfully
        if (plantuml.exit_code() != 0) {
            std::cerr << "PlantUML process failed for " << uml_path << std::endl;
        } else {
            std::cout << "SVG Image generated: " << image_path << std::endl;
        }

        ++i;
    }

    return 0;
}
