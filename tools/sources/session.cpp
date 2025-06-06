// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#include "prova/session.h"
#include "prova/process.h"
#include "prova/artifact.h"
#include "prova/action.h"
#include "prova/linux_open_flags.h"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <fcntl.h>

std::uint32_t prova::session::first_id() const {
    return _actions.front()->id();
}
prova::session::time_type prova::session::start() const {
    return _actions.front()->time();
}
prova::session::time_type prova::session::finish() const {
    return _actions.back()->time();
}
std::uint32_t prova::session::last_id() const {
    return _actions.back()->id();
}

bool prova::session::completely_overlaps(prova::session::ptr other) const {
		// std::cout << std::format("this [{} -> {}], other [{} -> {}]", start(), finish(), other->start(), other->finish()) << std::endl;
		if((other->start() >= start()) && (other->finish() <= finish()))
            return ((other->first_id() >= first_id()) && (other->last_id() <= last_id()));
		return false;
}

nlohmann::json& prova::session::flatten(nlohmann::json& json) const{
	static std::unordered_map<std::string, int> flag_map{
			{"O_ACCMODE", O_ACCMODE}, {"O_RDONLY", O_RDONLY}, {"O_WRONLY", O_WRONLY},
			{"O_RDWR", O_RDWR}, {"O_CREAT", O_CREAT}, {"O_EXCL", O_EXCL},
			{"O_NOCTTY", O_NOCTTY}, {"O_TRUNC", O_TRUNC}, {"O_APPEND", O_APPEND},
			{"O_NONBLOCK", O_NONBLOCK}, {"O_NDELAY", O_NDELAY}, {"O_SYNC", O_SYNC},
			{"O_FSYNC", O_FSYNC}, {"O_ASYNC", O_ASYNC}, {"__O_LARGEFILE", __O_LARGEFILE},
			{"__O_DIRECTORY", __O_DIRECTORY}, {"__O_NOFOLLOW", __O_NOFOLLOW},
			{"__O_CLOEXEC", __O_CLOEXEC}, {"__O_DIRECT", __O_DIRECT}, {"__O_NOATIME", __O_NOATIME},
			{"__O_PATH", __O_PATH}, {"__O_DSYNC", __O_DSYNC}, {"__O_TMPFILE", __O_TMPFILE},
			{"O_DIRECT", O_DIRECT}, {"O_NOATIME", O_NOATIME}, {"O_PATH", O_PATH},
			{"O_TMPFILE", O_TMPFILE}, {"O_DSYNC", O_DSYNC}, {"O_RSYNC", O_RSYNC},
			{"O_LARGEFILE", O_LARGEFILE}, {"O_DIRECTORY", O_DIRECTORY},
			{"O_NOFOLLOW", O_NOFOLLOW}, {"O_CLOEXEC", O_CLOEXEC}
	};

	nlohmann::json common = nlohmann::json::object();
	common.push_back({"process.name", 	boost::trim_copy_if(_process->name(), boost::is_any_of("\""))});
	common.push_back({"process.exe", 	boost::trim_copy_if(_process->exe(), boost::is_any_of("\""))});
	common.push_back({"process.egid",	_process->egid()});
	common.push_back({"process.euid", 	_process->euid()});
	common.push_back({"process.gid", 	_process->gid()});
	common.push_back({"process.pid", 	_process->pid()});
	common.push_back({"process.ppid", 	_process->ppid()});
	common.push_back({"process.uid", 	_process->uid()});

	common.push_back({"artifact.subtype", boost::trim_copy_if(_artifact->subtype(), boost::is_any_of("\""))});
	for (const auto& prop : _artifact->properties().items()) {
        if(prop.key() == "_key") continue;
        common.push_back({"artifact."+prop.key(), boost::trim_copy_if(prop.value().dump(), boost::is_any_of("\""))});
	}

    auto lambda_create_event_json = [common](const prova::action& action){
        nlohmann::json event = common;
        event.push_back({"action.id", 			action.id()});
        event.push_back({"action.operation", 	action.operation()});
        for (const auto& prop : action.properties().items()) {
            if(prop.key() == "_key") continue;
            if(prop.key() == "event id"){
                std::string value = prop.value();
                event.push_back({"action."+prop.key(), boost::lexical_cast<std::uint32_t>(value)});
            } else if(prop.key() == "time"){
                std::string value = prop.value();
                event.push_back({"action."+prop.key(), std::size_t(boost::lexical_cast<double>(value) *10000)});
            } else if(prop.key() == "flags"){
                std::string svalue = prop.value();
                int value = (flag_map.find(svalue) != flag_map.end()) ? flag_map[svalue] : -1;
                event.push_back({"action."+prop.key(), value});
            } else {
                event.push_back({"action."+prop.key(), boost::trim_copy_if(prop.value(), boost::is_any_of("\""))});
            }
            event.push_back({"action."+prop.key(), boost::trim_copy_if(prop.value().dump(), boost::is_any_of("\""))});
        }
        return event;
    };

    for(const auto& action: _actions) {
        if(action->type() != prova::action::category::release) {
            nlohmann::json event = lambda_create_event_json(*action);
            json.emplace_back(event);
        }
    }

	for(const auto& child: _children){
		child->flatten(json);
	}

    for(const auto& action: _actions) {
        if(action->type() == prova::action::category::release) {
            nlohmann::json event = lambda_create_event_json(*action);
            json.emplace_back(event);
        }
    }

	return json;
}
