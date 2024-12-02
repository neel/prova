// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#include "prova/artifact.h"

void prova::to_json(nlohmann::json& j, const prova::artifact& a) {
		j = {
				{"subtype", a._subtype},
		};
		j.update(a._properties);
}

void prova::from_json(const nlohmann::json& j, prova::artifact& a) {
		a._subtype = j.at("subtype").get<std::string>();
		a._properties = j;
		a._properties.erase("subtype");
}
