// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef PROVA_PROCESS_H
#define PROVA_PROCESS_H

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace prova{

struct session;

class process {
private:
    std::string _name;
    std::string _exe;
    std::uint32_t _egid;
    std::uint32_t _euid;
    std::uint32_t _gid;
    std::uint32_t _pid;
    std::uint32_t _ppid;
    std::uint32_t _uid;
    std::vector<std::shared_ptr<session>> _sessions;

public:
    using ptr = std::shared_ptr<process>;

    inline const std::string& name() const { return _name; }
    inline const std::string& exe() const  { return _exe; }
    inline std::uint32_t egid() const      { return _egid; }
    inline std::uint32_t euid() const      { return _euid; }
    inline std::uint32_t gid() const       { return _gid; }
    inline std::uint32_t pid() const       { return _pid; }
    inline std::uint32_t ppid() const      { return _ppid; }
    inline std::uint32_t uid() const       { return _uid; }

    inline process& name(const std::string& value) { _name = value; return *this; }
    inline process& exe(const std::string& value)  { _exe = value;  return *this; }
    inline process& egid(std::uint32_t value)      { _egid = value; return *this; }
    inline process& euid(std::uint32_t value)      { _euid = value; return *this; }
    inline process& gid(std::uint32_t value)       { _gid = value;  return *this; }
    inline process& pid(std::uint32_t value)       { _pid = value;  return *this; }
    inline process& ppid(std::uint32_t value)      { _ppid = value; return *this; }
    inline process& uid(std::uint32_t value)       { _uid = value;  return *this; }

    inline void add_session(const std::shared_ptr<session>& s){ _sessions.push_back(s); }
    inline auto begin() { return _sessions.begin(); }
    inline auto end() { return _sessions.end(); }
    inline auto size() const { return _sessions.size(); }
    inline void clear() { _sessions.clear(); }
    template <typename It>
    void add_sessions(It begin, It end) {
        for(It i = begin; i != end; ++i){
            add_session(*i);
        }
    }

    inline std::vector<std::shared_ptr<session>>::iterator erase(std::vector<std::shared_ptr<session>>::iterator it){ return _sessions.erase(it); }

    nlohmann::json to_json() const;
    static process from_json(const nlohmann::json& j);
    void merge(std::shared_ptr<process> other);

};


}

#endif // PROVA_PROCESS_H
