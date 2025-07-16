// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#include "prova/store.h"
#include "prova/artifact.h"
#include "prova/action.h"
#include <tash/arango.h>
#include "prova/execution_unit.h"
#include <boost/process.hpp>
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/child.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/process/v1/pipe.hpp>
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/io.hpp>

bool prova::store::insert(prova::session::ptr session){
	return _sessions.insert(session).second;
}

void prova::store::fetch(std::string database,std::string host, unsigned port, std::string user, std::string pass){
    tash::shell spade(database, host, port, user, pass); // shell("school", "localhost", 8529, "root", "root")
    if(spade.exists() == boost::beast::http::status::not_found){
        throw std::runtime_error{"Cannot connect to ArangoDB server"};
    }

    // Find (undirected) edges between a pair of vertices u, v such that
    //      either u or v is a Process vertex
    // group these edges into unique ebdpoints
    // returns a collection of {u, v, edges: all edges between (u, v), N: number of edges }
    //      such that N > 1

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
        // FILTER src.pid == '13713' || tgt.pid == '13713'
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
    nlohmann::json events = nlohmann::json::array();

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

        // transform each entry {u, v, edges: all edges between (u, v), N: number of edges } into a session between process and the related artifact
        // an action in that session will denote an edge of the entry
        prova::session::ptr s = std::make_shared<prova::session>(related_fs);           // s defines the session of I/O interaction between the related_ps process and the I/O artifact related_fs
        for(const nlohmann::json& edge_json: record["edges"]){                          // each edge e defines a I/O event between related_ps and related_fs
            bool is_syscall = (edge_json["source"].get<std::string>() == "syscall");
            if(is_syscall){
                std::uint32_t    event_id  = std::stoi(edge_json["event id"].get<std::string>());
                std::string      operation = edge_json["operation"].get<std::string>();
                prova::action::category type = prova::action::category::unknown;

                if(operation == "accept" || operation == "open" || operation == "create" || operation.starts_with("mmap")  || operation.starts_with("connect")){
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

        // If a process already exists with the same pid in the _processes map then merge the related_ps into that
        // otherwise add the related_ps into the _processes map

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

    // Now the sessions associated with the process have two characteristics
    //      1. They are all flat, none of them have any children
    //      2. One session may contain any number of acquire or release type actions
    // Ideally we want a session to consist of a pair of action, one acquire followed by one release.
    // But, the query may return edges like {Open, Close, Open, Close}
    // In that case we need to replace that big session into two small sessions
    // In general {Acquire, Release, Acquire, Release} should be broken down to {Acquire, Release}, {Acquire, Release}

    // However, beware of the cases like {Open, Read, Read, Close} are possible.
    // To be safe our approach should also be able to handle the following cases gracefull without crashing
    //      {Acquire, Acquire, ..., Release}    -> In this case put all of them into same session
    //      {Acquire, Acquire, ...}             -> This is actually a CWE the actuired resource can not been released.
    //                                             Create a fake release action and put it inside the session
    //      {Release, Release, ...}             -> Very bad case should not happen. For now log a message


    // { split big sessions into small sessions
    for(auto& pair: _processes){
        std::size_t pid = pair.first;
        std::vector<prova::session::ptr> new_sessions;
        for(auto sess_it = pair.second->begin(); sess_it != pair.second->end();){
            prova::session::ptr s = *sess_it;

            std::sort(s->begin(), s->end(), [](const auto& a, const auto& b){ return a->id() < b->id(); });
            auto action_count = s->size();

            bool is_simple = (action_count == 2 && s->at(0)->type() == prova::action::category::acquire && s->at(1)->type() == prova::action::category::release);
            bool should_delete = false;
            if(!is_simple){
                bool acquire_found = false;
                for(auto it = s->begin(); it != s->end(); ++it){
                    if((*it)->type() == prova::action::category::acquire){
                        acquire_found = true;
                        prova::session::ptr new_session = std::make_shared<prova::session>(s->artifact());
                        new_session->add_action(*it);
                        bool session_closed = false;
                        for(auto jt = it+1; jt != s->end(); ++jt) {
                            new_session->add_action(*jt);
                            if((*jt)->type() == prova::action::category::release){
                                new_sessions.push_back(new_session);
                                it = jt;
                                session_closed = true;
                                if(std::distance(it, s->end()) > 0){
                                    should_delete = true;
                                } else {
                                    new_session->clear();
                                    // The existing session is not a "big" session, as it does not include multiple release calls
                                }
                                break;
                            }
                        }

                        if(!session_closed){
                            std::cout << "Error: expecting release call after an acquire call for pid " << pid << " exe " << pair.second->exe() << std::endl;
                            for(auto ait = new_session->begin(); ait != new_session->end(); ++ait){
                                nlohmann::json json_ack;
                                prova::to_json(json_ack, *(*ait));
                                std::cout << json_ack << std::endl;
                            }
                            std::cout << std::endl;
                            auto fake_close = std::make_shared<prova::action>(0, prova::action::category::release);
                            fake_close->operation("missing");
                            fake_close->time((*it)->time());
                            fake_close->id((*it)->id());

                            new_session->add_action(fake_close);
                            session_closed = true;
                            should_delete = true;
                        } else {
                            // std::cout << "Good: session for pid " << pid << " actions " << new_session->size() << std::endl;
                        }
                        new_sessions.push_back(new_session);
                    }
                }

                if(!acquire_found && action_count > 0) {
                    // No acquire call has been observed but there are multiple actions
                    // therefore all these actions must be release actions
                    std::cout << "Error: expecting acquire call before a release call for pid " << pid << " exe " << pair.second->exe() << " artifact " << s->artifact()->properties() << std::endl;
                    for(auto it = s->begin(); it != s->end(); ++it){
                        auto fake_open = std::make_shared<prova::action>(0, prova::action::category::acquire);
                        fake_open->operation("missing");
                        fake_open->time((*it)->time());
                        fake_open->id((*it)->id());

                        prova::session::ptr new_session = std::make_shared<prova::session>(s->artifact());
                        new_session->add_action(fake_open);
                        new_session->add_action(*it);
                        new_sessions.push_back(new_session);

                        acquire_found  = true;
                        should_delete  = true;

                        nlohmann::json json_rel;
                        prova::to_json(json_rel, *(*it));
                        std::cout << json_rel << std::endl;
                    }
                }
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
            prova::session::ptr  s = *sess_it;                                                      // take session s
            prova::session::ptr  p = s->_parent;                                                    // take p the parent of session s
            if(p){
                for(auto it = s->_children.begin(); it != s->_children.end(); ++it){
                    prova::session::ptr c = *it;                                                    // c is a child of session s
                    for(auto x_sess_it = p->_children.begin(); x_sess_it != p->_children.end();){   // iterate through all children of p
                        if(*x_sess_it == c){                                                        // c is already a grandchild of p and child of p at the same time
                            x_sess_it = p->_children.erase(x_sess_it);                              // therefore it should not be a child of p also
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

    // { Debug
    // for(auto& pair: _processes){
    //     std::cout << "Process: " << pair.first << std::endl;
    //     for(auto sess_it = pair.second->begin(); sess_it != pair.second->end(); ++sess_it){
    //         prova::session::ptr  s = *sess_it;
    //         std::cout << "Session " << s->first_id() << " " << s->last_id() << std::endl;
    //         for(const auto& a: *s){
    //             std::cout << "\tAction " << a->id() << " " << a->operation() << std::endl;
    //         }
    //     }
    // }
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

std::ostream& prova::store::dataset(std::ostream& stream) const{
    nlohmann::json units = nlohmann::json::array();

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
    auto& index_by_start = index_by_first_id();
    for (auto it = index_by_start.begin(); it != index_by_start.end(); ++it) {
        prova::session::ptr sess = *it;
        if(!sess->_parent && sess->_children.size() > 0){
            stream << "group " << "\n";
            decorate_session(sess->_process->pid(), sess, 0);
            stream << "end " << "\n";
        }
    }
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
        if(!sess->_parent){
            prova::process::ptr process = sess->_process;
            auto unit = std::make_shared<prova::execution_unit>(process, sess);
            extract_artifacts(sess, unit);
            units.emplace_back(unit);
        }
    }
}

std::size_t prova::store::extract_all(const std::string& plantuml_jar_path){
    std::vector<std::shared_ptr<prova::execution_unit>> units;
    extract(units);

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
        boost::process::v1::opstream in_stream;
        boost::process::v1::ipstream out_stream;
        std::string command = "java -jar \"" + plantuml_jar_path + "\" -tsvg -pipe";
        boost::process::v1::child plantuml(command, boost::process::v1::std_in < in_stream, boost::process::v1::std_out > out_stream);

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
            std::cout << "SVG generated: " << image_path << std::endl;
        }

        ++i;
    }
    return i;
}

