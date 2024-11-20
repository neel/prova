// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#include "sysaudit/reporter.h"

prova::sysaudit::reporter::reporter(const std::string& agent):  _shell("provenance"), _events(_shell, "audit"), _agent(agent) {
    if(_shell.exists() == boost::beast::http::status::not_found){
        _shell.create();
    }
    if(_events.exists() == boost::beast::http::status::not_found){
        _events.create();
    }
}

void prova::sysaudit::reporter::report(const std::vector<log>& entries){
    nlohmann::json logs = entries;
    for (auto& l: logs){
        l["agent"] = _agent;
        boost::beast::http::status status = _events.add(l);
        if(status == boost::beast::http::status::accepted){
            std::cout << std::format("Added event {}", l["_key"].get<std::string>()) << std::endl;
        }else{
            std::cout << "Failed to add event " << status << std::endl;
        }
    }
}
