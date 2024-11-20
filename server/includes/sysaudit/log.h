// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef SYSAUDIT_LOG_H
#define SYSAUDIT_LOG_H

#include <string>
#include <tuple>
#include <chrono>
#include <nlohmann/json.hpp>

namespace prova::sysaudit{

    struct log{
        using property   = std::tuple<int, std::string, std::string>;
        using collection = std::vector<property>;
        using iterator   = collection::const_iterator;
        using size_type  = collection::size_type;
        using value_type = property;
        using time_type  = std::chrono::system_clock::time_point;

        inline log() = default;
        inline log(int type, const char* label, const time_type& time, std::size_t serial, unsigned nfields): _type(type), _label(label), _time(time), _serial(serial), _fields(nfields) {}
        inline int type() const { return _type; }
        inline const std::string& label() const { return _label; }
        inline std::size_t fields() const { return _fields; }
        inline iterator begin() const { return _properties.begin(); }
        inline iterator end() const { return _properties.end(); }
        inline size_type size() const { return _properties.size(); }
        inline std::size_t serial() const { return _serial; }
        inline const time_type& time() const { return _time; }

        void add_field(int type, const char* name, const char* value);
        private:
            int         _type;
            std::string _label;
            time_type   _time;
            std::size_t _serial;
            std::size_t _fields;
            collection _properties;
    };

void to_json(nlohmann::json& j, const sysaudit::log::property& p);
void to_json(nlohmann::json& j, const sysaudit::log& l);

}




#endif // SYSAUDIT_LOG_H
