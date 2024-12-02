// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef PROVA_ACTION_H
#define PROVA_ACTION_H

#include <string>
#include <memory>
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace prova{

struct action;

struct action {
    using ptr = std::shared_ptr<action>;
    using time_type = std::chrono::system_clock::time_point;
    enum class category {
        acquire, release, message, unknown
    };

    inline action(std::uint32_t event = 0, category t = category::unknown) : _event(event), _type(t) {}
    inline action(std::uint32_t event, category t, const std::string& operation) : _event(event), _type(t), _operation(operation) {}
    inline action(const action&) = delete;

    void parse_time(const std::string& ts_str);

    inline std::uint32_t id() const { return _event; }
    inline void id(std::uint32_t event_id) { _event = event_id; }
    inline category type() const { return _type; }
    inline void type(category t) { _type = t; }
    inline const std::string& operation() const { return _operation; }
    inline void operation(const std::string& op) { _operation = op; }
    inline time_type time() const { return _time; }
    inline const nlohmann::json& properties() const { return _properties; }

    void serialize(nlohmann::json& j) const;
    void deserialize(const nlohmann::json& j);
    friend void to_json(nlohmann::json& j, const action& a);
    friend void from_json(const nlohmann::json& j, action& a);

private:
    std::uint32_t _event;
    category _type;
    std::string _operation;
    time_type _time;
    nlohmann::json _properties;
};


}

#endif // PROVA_ACTION_H
