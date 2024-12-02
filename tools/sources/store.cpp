// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#include "prova/store.h"
#include "prova/artifact.h"
#include "prova/action.h"
#include <tash/arango.h>
#include "prova/execution_unit.h"

bool prova::store::insert(prova::session::ptr session){
	return _sessions.insert(session).second;
}

void prova::store::fetch(){
    tash::shell spade("spade"); // shell("school", "localhost", 8529, "root", "root")
    if(spade.exists() == boost::beast::http::status::not_found){
        throw std::runtime_error{"Cannot connect to ArangoDB server"};
    }

    auto aql = R"AQL(
        FOR e IN edges
        LET endpoints = [e._from, e._to]
        LET sortedEndpoints = (endpoints[0] < endpoints[1] ? endpoints : [endpoints[1], endpoints[0]]) // Sort the endpoints to ensure direction agnosticism
        COLLECT u = sortedEndpoints[0], v = sortedEndpoints[1] INTO sessions = {
            original_from: e._from,
            original_to: e._to,
            edge: e
        }
        LET src = DOCUMENT(vertices, sessions[0].original_from)
        LET tgt = DOCUMENT(vertices, sessions[0].original_to)
        FILTER src.type == 'Process' || tgt.type == 'Process'
        // FILTER src.exe == '/usr/sbin/nginx' || tgt.exe == '/usr/sbin/nginx'
        LET N = LENGTH(sessions)
        FILTER N > 1
        RETURN {
            "from": UNSET(src, ["_id", "_rev", "epoch", "version"]),
            "to": UNSET(tgt, ["_id", "_rev", "epoch", "version"]),
            "edges": sessions[* RETURN UNSET(CURRENT.edge, ["_key", "_from", "_to", "_id", "_rev", "type"])],
            "count": N
        }
    )AQL";
    tash::cursor cursor = spade.aql(aql);

    // { parse json and make the objects
    for(const nlohmann::json& record: cursor.results()){
        // std::cout << record << std::endl;
        // std::cout << std::endl;

        prova::process::ptr related_ps;
        prova::artifact::ptr related_fs;
        const nlohmann::json& from_json = record["from"], to_json = record["to"];
        if(from_json["type"] == "Process"){
            related_ps = std::make_shared<prova::process>(prova::process::from_json(from_json));
        }
        if(to_json["type"] == "Process"){
            related_ps = std::make_shared<prova::process>(prova::process::from_json(to_json));
        }
        if(from_json["type"] == "Artifact"){
            related_fs = std::make_shared<prova::artifact>(from_json.get<prova::artifact>());
        }
        if(to_json["type"] == "Artifact"){
            related_fs = std::make_shared<prova::artifact>(to_json.get<prova::artifact>());
        }

        assert(related_ps);
        if(!related_fs){
            continue;
        }

        prova::session::ptr s = std::make_shared<prova::session>(related_fs);
        for(const nlohmann::json& edge_json: record["edges"]){
            bool is_syscall = (edge_json["source"].get<std::string>() == "syscall");
            if(is_syscall){
                std::uint32_t    event_id  = std::stoi(edge_json["event id"].get<std::string>());
                std::string      operation = edge_json["operation"].get<std::string>();
                prova::action::category type = prova::action::category::unknown;

                if(operation == "accept" || operation == "open" || operation == "create" || operation.starts_with("mmap")){
                    type = prova::action::category::acquire;
                } else if (operation == "close" || operation.starts_with("munmap")) {
                    type = prova::action::category::release;
                }

                prova::action::ptr a = std::make_shared<prova::action>(event_id, type);
                a->deserialize(edge_json);
                s->add_action(a);
            }
        }
        related_ps->add_session(s);

        std::size_t pid = related_ps->pid();
        auto it = _processes.find(pid);
        if(it != _processes.end()){
            prova::process::ptr target_ps = it->second;
            target_ps->merge(related_ps);
        } else {
            _processes.insert(std::make_pair(pid, related_ps));
        }
    }
    // }

    // { split big sessions into small sessions
    for(auto& pair: _processes){
        std::size_t pid = pair.first;
        std::vector<prova::session::ptr> new_sessions;
        for(auto sess_it = pair.second->begin(); sess_it != pair.second->end();){
            prova::session::ptr s = *sess_it;
            bool should_delete = false;
            if(s->size() > 2){
                // std::cout << "Breaking down" << std::endl;

                for(auto it = s->begin(); it != s->end(); ++it){
                    if((*it)->type() == prova::action::category::acquire){
                        if((*(it+1))->type() == prova::action::category::release){
                            prova::session::ptr new_session = std::make_shared<prova::session>(s->artifact());
                            new_session->add_action(*it);
                            new_session->add_action(*(it+1));
                            new_sessions.push_back(new_session);
                            ++it;
                        }
                    }
                }
                should_delete = true;
            }
            if(should_delete){
                sess_it = pair.second->erase(sess_it);
                should_delete = false;
            } else {
                ++sess_it;
            }
        }
        if(new_sessions.size() > 0){
            pair.second->add_sessions(new_sessions.begin(), new_sessions.end());
            new_sessions.clear();
        }
    }
    // }

    // { Trivial temporal interdependence
    for(auto& pair: _processes){
        for(auto sess_it = pair.second->begin(); sess_it != pair.second->end(); ++sess_it){
            prova::session::ptr  s = *sess_it;
            prova::artifact::ptr a = s->artifact();
            std::string a_key      = a->properties()["_key"];

            for(auto child_sess_it = pair.second->begin(); child_sess_it != pair.second->end(); ++child_sess_it){
                prova::session::ptr  child_s = *child_sess_it;
                prova::artifact::ptr child_a = child_s->artifact();
                std::string child_a_key      = child_a->properties()["_key"];

                if(s == child_s){
                    continue;
                }

                bool overlaps = s->completely_overlaps(child_s);
                if(overlaps){
                    s->_children.push_back(child_s);
                    child_s->_parent = s;               // if p = parent(c) then parent(p) should not contain c
                }
            }
        }
    }

    for(auto& pair: _processes){
        for(auto sess_it = pair.second->begin(); sess_it != pair.second->end(); ++sess_it){
            prova::session::ptr  s = *sess_it;
            prova::session::ptr  p = s->_parent;
            if(p){
                for(auto it = s->_children.begin(); it != s->_children.end(); ++it){
                    for(auto x_sess_it = p->_children.begin(); x_sess_it != p->_children.end();){
                        if(*x_sess_it == *it){
                            x_sess_it = p->_children.erase(x_sess_it);
                        } else {
                            ++x_sess_it;
                        }
                    }
                }
            }
        }
    }
    // }

    // { store the sessions and the artifacts
    for(auto& pair: _processes){
        // std::size_t pid = pair.first;
        for(auto sess_it = pair.second->begin(); sess_it != pair.second->end(); ++sess_it){
            prova::session::ptr s = *sess_it;
            s->_process = pair.second;
            prova::artifact::ptr a = s->artifact();
            // std::cout << a->properties() << std::endl;
            std::string a_key = a->properties()["_key"].get<std::string>();
            _artifacts.insert(std::make_pair(a_key, a));
            insert(s);
        }
    }
    // }
}

std::ostream& prova::store::uml(std::ostream& stream) const{
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
    for(const auto& p: _processes){
        stream << std::format("participant \"{}\" as P{}", p.second->exe(), p.first) << "\n";
    }
    for(const auto& a: _artifacts){
        std::string artifact_name;
        if(a.second->subtype() == "file"){
            artifact_name = a.second->properties()["path"];
        } else if(a.second->subtype() == "network socket"){
            artifact_name = std::format("{}:{}", a.second->properties()["remote address"].get<std::string>(), a.second->properties()["remote port"].get<std::string>());
        } else if(a.second->subtype() == "unnamed pipe") {
            artifact_name = std::format("{} w{} -> r{}", a.second->properties()["_key"].get<std::string>(), a.second->properties()["write fd"].get<std::string>(), a.second->properties()["read fd"].get<std::string>());
        } else if(a.second->subtype() == "directory") {
            artifact_name = a.second->properties()["path"];
        } else if(a.second->subtype() == "character device") {
            artifact_name = a.second->properties()["path"];
        } else {
           artifact_name = "Unknown";
        }
        stream << std::format("participant \"{}\" as {}", artifact_name, a.first) << "\n";
    }
    auto& index_by_start = index_by_first_id();
    for (auto it = index_by_start.begin(); it != index_by_start.end(); ++it) {
        prova::session::ptr sess = *it;
        if(!sess->_parent && sess->_children.size() > 0){
            stream << "group " << "\n";
            decorate_session(sess->_process->pid(), sess, 0);
            stream << "end " << "\n";
        }
    }
    stream << "@enduml" << "\n";

    return stream;
}

void prova::store::extract(std::vector<std::shared_ptr<prova::execution_unit>>& units){
    std::function<void (const prova::session::ptr&, std::shared_ptr<prova::execution_unit>&)> extract_artifacts;
    extract_artifacts = [&extract_artifacts](const prova::session::ptr& sess, std::shared_ptr<prova::execution_unit>& unit) -> void {
        unit->add_artifact(sess->artifact());
        if(sess->_children.size() > 0){
            for(const auto& sess_child: sess->_children){
                extract_artifacts(sess_child, unit);
            }
        }
    };

    auto& index_by_start = index_by_first_id();
    for (auto it = index_by_start.begin(); it != index_by_start.end(); ++it) {
        prova::session::ptr sess = *it;
        if(!sess->_parent && sess->_children.size() > 0){
            prova::process::ptr process = sess->_process;
            auto unit = std::make_shared<prova::execution_unit>(process, sess);
            extract_artifacts(sess, unit);
            units.emplace_back(unit);
        }
    }
}

