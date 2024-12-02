// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef PROVA_ARTIFACT_H
#define PROVA_ARTIFACT_H

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace prova{

struct artifact {
    using ptr = std::shared_ptr<artifact>;

    std::string _subtype;
    nlohmann::json _properties;

    inline std::string subtype() const { return _subtype; }
    inline void subtype(const std::string& subtype) { _subtype = subtype; }
    inline const nlohmann::json& properties() const { return _properties; }
    inline nlohmann::json& properties() { return _properties; }

    friend void to_json(nlohmann::json& j, const artifact& a);
    friend void from_json(const nlohmann::json& j, artifact& a);
};


void to_json(nlohmann::json& j, const artifact& a);
void from_json(const nlohmann::json& j, artifact& a);

}

#endif // PROVA_ARTIFACT_H
