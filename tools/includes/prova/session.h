// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef PROVA_SESSION_H
#define PROVA_SESSION_H

#include <string>
#include <memory>
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>


namespace prova{

class process;
struct action;
struct action;
struct artifact;

struct session{
    using ptr = std::shared_ptr<session>;
    using time_type = std::chrono::system_clock::time_point;
		using action_container = std::vector<std::shared_ptr<prova::action>>;

    ptr                              _parent;
    std::shared_ptr<prova::process>  _process;
    std::shared_ptr<prova::artifact> _artifact;
    action_container								 _actions;
    std::vector<ptr>                 _children;

    inline session(std::shared_ptr<prova::artifact> artifact): _artifact(artifact) {}

    inline void add_action(std::shared_ptr<prova::action> action){ _actions.push_back(action); }
    inline void clear() { _actions.clear(); }
    inline auto begin() const { return _actions.begin(); }
    inline auto end()   const { return _actions.end(); }
    inline auto size()  const { return _actions.size(); }
    inline std::shared_ptr<prova::action> at(std::size_t n) { return _actions.at(n); }
    inline std::shared_ptr<prova::artifact> artifact() const { return _artifact; }

    std::uint32_t first_id() const;
    time_type start() const;
    time_type finish() const;
    std::uint32_t last_id() const;

    bool completely_overlaps(ptr other) const;
};

}

#endif // PROVA_SESSION_H
