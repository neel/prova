// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#include "prova/execution_unit.h"
#include "prova/session.h"
#include "prova/action.h"
#include <format>
#include <string>
#include "prova/process.h"
#include "prova/artifact.h"
#include <fstream>
#include <boost/process.hpp>

std::ostream& prova::execution_unit::uml(std::ostream& stream) const{
    std::function<void (std::size_t, const prova::session::ptr&, std::uint8_t)> decorate_session;
    decorate_session = [&decorate_session, &stream](std::size_t pid, const prova::session::ptr& sess, std::uint8_t indent) -> void {
        for(std::uint8_t i = 0; i != indent; ++i) stream << "  ";
        stream << std::format("P{} -> {}: <<acquire>> {} [{}]", pid, sess->artifact()->properties()["_key"].get<std::string>(), sess->at(0)->operation(), sess->at(0)->id()) << "\n";
        if(sess->_children.size() > 0){
            for(const auto& sess_child: sess->_children){
                decorate_session(pid, sess_child, indent +1);
            }
        }
        for(std::uint8_t i = 0; i != indent; ++i) stream << "  ";
        stream << std::format("P{} --> {}: <<release>> {} [{}]", pid, sess->artifact()->properties()["_key"].get<std::string>(), sess->at(1)->operation(), sess->at(1)->id()) << "\n";
    };

    stream << "@startuml" << "\n";
		stream << std::format("participant \"{}\" as P{}", _process->exe(), _process->pid()) << "\n";
    for(const auto& a: _artifacts){
        std::string artifact_name;
        if(a->subtype() == "file"){
            artifact_name = a->properties()["path"];
        } else if(a->subtype() == "network socket"){
            artifact_name = std::format("{}:{}", a->properties()["remote address"].get<std::string>(), a->properties()["remote port"].get<std::string>());
        } else if(a->subtype() == "unnamed pipe") {
            artifact_name = std::format("{} w{} -> r{}", a->properties()["_key"].get<std::string>(), a->properties()["write fd"].get<std::string>(), a->properties()["read fd"].get<std::string>());
        } else if(a->subtype() == "directory") {
            artifact_name = a->properties()["path"];
        } else if(a->subtype() == "character device") {
            artifact_name = a->properties()["path"];
        } else {
           artifact_name = "Unknown";
        }
        stream << std::format("participant \"{}\" as {}", artifact_name, a->properties()["_key"].get<std::string>()) << "\n";
    }

		decorate_session(_root_session->_process->pid(), _root_session, 0);
    stream << "@enduml" << "\n";

    return stream;
}

void prova::execution_unit::save_uml(const std::filesystem::path& uml_path) const {
    std::ofstream uml_file{uml_path};
    uml(uml_file);
    uml_file.close();
}

void prova::execution_unit::render_svg(const std::filesystem::path& image_path) const {
    std::stringstream uml_buffer;
    uml(uml_buffer);

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
}
