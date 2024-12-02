// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#include "prova/action.h"

void prova::action::parse_time(const std::string& ts_str) {
	std::size_t dot_pos = ts_str.find('.');
	std::string seconds_str = ts_str.substr(0, dot_pos);
	std::string microseconds_str = ts_str.substr(dot_pos + 1);
	long long seconds = std::stoll(seconds_str);
	long long microseconds = std::stoll(microseconds_str);
	microseconds *= 1000;

	_time = std::chrono::system_clock::from_time_t(seconds) + std::chrono::microseconds(microseconds);
}

void prova::action::serialize(nlohmann::json& j) const {
	auto tp = _time;
	std::time_t t = std::chrono::system_clock::to_time_t(tp);
	std::tm tm = *std::localtime(&t);
	std::stringstream ss;
	ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

	j = nlohmann::json{
			{"event id", _event},
			{"operation", _operation},
			{"time", ss.str()}
	};
	j.update(_properties);
}

void prova::action::deserialize(const nlohmann::json& j) {
	_event = std::stoi(j.at("event id").get<std::string>());
	_operation = j.at("operation").get<std::string>();
	parse_time(j.at("time").get<std::string>());

	_properties = j;
	_properties.erase("event id");
	_properties.erase("operation");
	_properties.erase("time");
}

void prova::to_json(nlohmann::json& j, const prova::action& a) {
	a.serialize(j);
}

void prova::from_json(const nlohmann::json& j, prova::action& a) {
	a.deserialize(j);
}
