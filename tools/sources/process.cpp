// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#include "prova/process.h"

nlohmann::json prova::process::to_json() const {
	return {
			{"name", _name},
			{"exe",  _exe},
			{"egid", _egid},
			{"euid", _euid},
			{"gid",  _gid},
			{"pid",  _pid},
			{"ppid", _ppid},
			{"uid",  _uid}
	};
}

prova::process prova::process::from_json(const nlohmann::json& j) {
		prova::process proc;
		proc.name(j.at("name").get<std::string>());
		proc.exe( j.at("exe") .get<std::string>());
		proc.egid(std::stoi(j.at("egid").get<std::string>()));
		proc.euid(std::stoi(j.at("euid").get<std::string>()));
		proc.gid( std::stoi(j.at("gid") .get<std::string>()));
		proc.pid( std::stoi(j.at("pid") .get<std::string>()));
		proc.ppid(std::stoi(j.at("ppid").get<std::string>()));
		proc.uid( std::stoi(j.at("uid") .get<std::string>()));
		return proc;
}

void prova::process::merge(std::shared_ptr<prova::process> other){
		for(const std::shared_ptr<prova::session>& s: *other){
				add_session(s);
		}
}
