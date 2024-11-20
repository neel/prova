// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#include "sysaudit/log.h"

void prova::sysaudit::log::add_field(int type, const char* name, const char* value){
    property prop{type, std::string{name}, std::string{value}};
    _properties.emplace_back(std::move(prop));
}

void prova::sysaudit::to_json(nlohmann::json& j, const sysaudit::log::property& p) {
    j = nlohmann::json{{"type",  std::get<0>(p)},
                       {"name",  std::get<1>(p)},
                       {"value", std::get<2>(p)}};
}

void prova::sysaudit::to_json(nlohmann::json& j, const sysaudit::log& l) {
    nlohmann::json props = nlohmann::json::array();
    for(const sysaudit::log::property& p: l){
        nlohmann::json prop_json = p;
        props.emplace_back(std::move(prop_json));
    }

    j = nlohmann::json{
        {"type",        l.type()},
        {"label",       l.label()},
        {"time",        std::format("{:%FT%TZ}", l.time())},
        {"serial",      l.serial()},
        {"fields",      l.fields()},
        {"properties",  props}
    };
}
