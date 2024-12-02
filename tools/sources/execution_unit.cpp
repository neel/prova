// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#include "prova/execution_unit.h"
#include "prova/session.h"
#include "prova/action.h"
#include <format>
#include <string>
#include "prova/process.h"
#include "prova/artifact.h"

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
