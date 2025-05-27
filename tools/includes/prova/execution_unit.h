// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef PROVA_EXECUTION_UNIT_H
#define PROVA_EXECUTION_UNIT_H

#include <memory>
#include <vector>
#include <filesystem>

namespace prova{

struct process;
struct session;
struct artifact;

/**
 * @todo write docs
 */
struct execution_unit{
    using artifacts_collection_type = std::vector<std::shared_ptr<prova::artifact>>;
    using const_iterator = typename artifacts_collection_type::const_iterator;

	inline execution_unit(std::shared_ptr<prova::process> process, std::shared_ptr<prova::session> session) : _process(process), _root_session(session) {}
	inline void add_artifact(std::shared_ptr<prova::artifact> artifact){ _artifacts.emplace_back(artifact); }
	inline std::size_t artifacts_count() const { return _artifacts.size(); }
	std::ostream& uml(std::ostream& stream) const;
	inline const std::shared_ptr<prova::process>& process() const { return _process; }
	void save_uml(const std::filesystem::path& uml_path) const;
    void render_svg(const std::string& plantuml_jar_path, const std::filesystem::path& svg_path) const;
	std::shared_ptr<prova::session> root() const { return _root_session; }
    public:
        inline const_iterator begin() const { return _artifacts.begin(); }
        inline const_iterator end() const { return _artifacts.end(); }
	private:
		std::shared_ptr<prova::process> _process;
		std::shared_ptr<prova::session> _root_session;
        artifacts_collection_type _artifacts;
};

}

#endif // PROVA_EXECUTION_UNIT_H
