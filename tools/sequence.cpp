#include <iostream>
#include <tash/arango.h>
#include <cstdint>
#include <map>
#include <memory>
#include <functional>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace prova{

struct action {
    using ptr = std::shared_ptr<action>;
    using time_type = std::chrono::system_clock::time_point;
    enum class category {
        acquire, release, message, unknown
    };

    action(std::uint32_t event = 0, category t = category::unknown) : _event(event), _type(t) {}
    action(std::uint32_t event, category t, const std::string& operation) : _event(event), _type(t), _operation(operation) {}
    action(const action&) = delete;

    void parse_time(const std::string& ts_str) {
        std::size_t dot_pos = ts_str.find('.');
        std::string seconds_str = ts_str.substr(0, dot_pos);
        std::string microseconds_str = ts_str.substr(dot_pos + 1);
        long long seconds = std::stoll(seconds_str);
        long long microseconds = std::stoll(microseconds_str);
        microseconds *= 1000;

        _time = std::chrono::system_clock::from_time_t(seconds) + std::chrono::microseconds(microseconds);
    }

    std::uint32_t id() const { return _event; }
    void id(std::uint32_t event_id) { _event = event_id; }
    category type() const { return _type; }
    void type(category t) { _type = t; }
    const std::string& operation() const { return _operation; }
    void operation(const std::string& op) { _operation = op; }
    time_type time() const { return _time; }
    const nlohmann::json& properties() const { return _properties; }

    void serialize(nlohmann::json& j) const {
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

    void deserialize(const nlohmann::json& j) {
        _event = std::stoi(j.at("event id").get<std::string>());
        _operation = j.at("operation").get<std::string>();
        parse_time(j.at("time").get<std::string>());

        _properties = j;
        _properties.erase("event id");
        _properties.erase("operation");
        _properties.erase("time");
    }

    friend void to_json(nlohmann::json& j, const action& a) {
        a.serialize(j);
    }

    friend void from_json(const nlohmann::json& j, action& a) {
        a.deserialize(j);
    }

private:
    std::uint32_t _event;
    category _type;
    std::string _operation;
    time_type _time;
    nlohmann::json _properties;
};

struct artifact {
    using ptr = std::shared_ptr<artifact>;

    std::string _subtype;
    nlohmann::json _properties;

    std::string subtype() const { return _subtype; }
    void subtype(const std::string& subtype) { _subtype = subtype; }

    const nlohmann::json& properties() const { return _properties; }
    nlohmann::json& properties() { return _properties; }

    friend void to_json(nlohmann::json& j, const artifact& a) {
        j = {
            {"subtype", a._subtype},
        };
        j.update(a._properties);
    }


    friend void from_json(const nlohmann::json& j, artifact& a) {
        a._subtype = j.at("subtype").get<std::string>();
        a._properties = j;
        a._properties.erase("subtype");
    }
};

class process;

struct session{
    using ptr = std::shared_ptr<session>;
    using time_type = prova::action::time_type;

    ptr                             _parent;
    std::shared_ptr<prova::process> _process;
    prova::artifact::ptr            _artifact;
    std::vector<prova::action::ptr> _actions;
    std::vector<ptr>                _children;

    session(prova::artifact::ptr artifact): _artifact(artifact) {}

    void add_action(prova::action::ptr action){ _actions.push_back(action); }
    void clear() { _actions.clear(); }
    auto begin() const { return _actions.begin(); }
    auto end()   const { return _actions.end(); }
    auto size()  const { return _actions.size(); }
    prova::action::ptr at(std::size_t n) { return _actions.at(n); }
    prova::artifact::ptr artifact() const { return _artifact; }

    std::uint32_t first_id() const {
        return _actions.front()->id();
    }
    time_type start() const {
        return _actions.front()->time();
    }
    time_type finish() const {
        return _actions.back()->time();
    }
    std::uint32_t last_id() const {
        return _actions.back()->id();
    }

    bool completely_overlaps(ptr other) const {
        // std::cout << std::format("this [{} -> {}], other [{} -> {}]", start(), finish(), other->start(), other->finish()) << std::endl;
        if((other->start() >= start()) && (other->finish() <= finish()))
            return ((other->first_id() > first_id()) && (other->last_id() < last_id()));
        return false;
    }
};

/**
 * @brief stores sessions in a multi index container that provides ordered view based on the start as well as the finish time of the session
 */
struct session_store{
    struct by_first_id {};
    struct by_last_id {};
    struct by_start {};
    struct by_finish {};

    using session_set = boost::multi_index_container<
        prova::session::ptr,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<by_start>,
                boost::multi_index::const_mem_fun<prova::session, prova::session::time_type, &prova::session::start>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<by_finish>,
                boost::multi_index::const_mem_fun<prova::session, prova::session::time_type, &prova::session::finish>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<by_first_id>,
                boost::multi_index::const_mem_fun<prova::session, std::uint32_t, &prova::session::first_id>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<by_last_id>,
                boost::multi_index::const_mem_fun<prova::session, std::uint32_t, &prova::session::last_id>
            >
        >
    >;

    session_set _sessions;

    const boost::multi_index::index<session_set, by_first_id>::type&  index_by_first_id()  { return _sessions.get<by_first_id>();  }
    const boost::multi_index::index<session_set, by_last_id>::type& index_by_last_id() { return _sessions.get<by_last_id>(); }

    const boost::multi_index::index<session_set, by_start>::type&  index_by_start()  { return _sessions.get<by_start>();  }
    const boost::multi_index::index<session_set, by_finish>::type& index_by_finish() { return _sessions.get<by_finish>(); }

    /**
     * @brief apply function f on all actions sorted by the start function based index and passes the n'th action from that session to f
     * Function f should have callback Result (prova::action::ptr)
     */
    template <typename Function>
    void apply_by_start(Function&& f){
        auto& index = _sessions.get<by_start>();
        for (auto& sess : index) {
            f(sess->at(0));
        }
    }

    template <typename Function>
    void apply_by_finish(Function&& f){
        auto& index = _sessions.get<by_finish>();
        for (auto& sess : index) {
            f(sess->at(1));
        }
    }

    bool insert(prova::session::ptr session){
        return _sessions.insert(session).second;
    }
};

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

    const std::string& name() const { return _name; }
    const std::string& exe() const  { return _exe; }
    std::uint32_t egid() const      { return _egid; }
    std::uint32_t euid() const      { return _euid; }
    std::uint32_t gid() const       { return _gid; }
    std::uint32_t pid() const       { return _pid; }
    std::uint32_t ppid() const      { return _ppid; }
    std::uint32_t uid() const       { return _uid; }

    process& name(const std::string& value) { _name = value; return *this; }
    process& exe(const std::string& value)  { _exe = value;  return *this; }
    process& egid(std::uint32_t value)      { _egid = value; return *this; }
    process& euid(std::uint32_t value)      { _euid = value; return *this; }
    process& gid(std::uint32_t value)       { _gid = value;  return *this; }
    process& pid(std::uint32_t value)       { _pid = value;  return *this; }
    process& ppid(std::uint32_t value)      { _ppid = value; return *this; }
    process& uid(std::uint32_t value)       { _uid = value;  return *this; }

    void add_session(const std::shared_ptr<session>& s){
        _sessions.push_back(s);
    }
    auto begin() { return _sessions.begin(); }
    auto end() { return _sessions.end(); }
    auto size() const { return _sessions.size(); }
    void clear() { _sessions.clear(); }
    template <typename It>
    void add_sessions(It begin, It end) {
        for(It i = begin; i != end; ++i){
            add_session(*i);
        }
    }
    std::vector<std::shared_ptr<session>>::iterator erase(std::vector<std::shared_ptr<session>>::iterator it){
        return _sessions.erase(it);
    }

    nlohmann::json to_json() const {
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

    static process from_json(const nlohmann::json& j) {
        process proc;
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

    void merge(std::shared_ptr<process> other){
        for(const std::shared_ptr<session>& s: *other){
            add_session(s);
        }
    }

};

}


int main(){
    tash::shell spade("spade"); // shell("school", "localhost", 8529, "root", "root")
    if(spade.exists() == boost::beast::http::status::not_found){
        throw std::runtime_error{"Cannot connect to ArangoDB server"};
    }

    auto aql = R"AQL(
        FOR e IN edges
        LET endpoints = [e._from, e._to]
        LET sortedEndpoints = (endpoints[0] < endpoints[1] ? endpoints : [endpoints[1], endpoints[0]]) // Sort the endpoints to ensure direction agnosticism
        COLLECT u = sortedEndpoints[0], v = sortedEndpoints[1] INTO sessions = {
            original_from: e._from,
            original_to: e._to,
            edge: e
        }
        LET src = DOCUMENT(vertices, sessions[0].original_from)
        LET tgt = DOCUMENT(vertices, sessions[0].original_to)
        FILTER src.type == 'Process' || tgt.type == 'Process'
        // FILTER src.exe == '/usr/sbin/nginx' || tgt.exe == '/usr/sbin/nginx'
        LET N = LENGTH(sessions)
        FILTER N > 1
        RETURN {
            "from": UNSET(src, ["_id", "_rev", "epoch", "version"]),
            "to": UNSET(tgt, ["_id", "_rev", "epoch", "version"]),
            "edges": sessions[* RETURN UNSET(CURRENT.edge, ["_key", "_from", "_to", "_id", "_rev", "type"])],
            "count": N
        }
    )AQL";
    tash::cursor cursor = spade.aql(aql);

    std::map<std::size_t, prova::process::ptr> ps;

    // { parse json and make the objects
    for(const nlohmann::json& record: cursor.results()){
        // std::cout << record << std::endl;
        // std::cout << std::endl;

        prova::process::ptr related_ps;
        prova::artifact::ptr related_fs;
        const nlohmann::json& from_json = record["from"], to_json = record["to"];
        if(from_json["type"] == "Process"){
            related_ps = std::make_shared<prova::process>(prova::process::from_json(from_json));
        }
        if(to_json["type"] == "Process"){
            related_ps = std::make_shared<prova::process>(prova::process::from_json(to_json));
        }
        if(from_json["type"] == "Artifact"){
            related_fs = std::make_shared<prova::artifact>(from_json.get<prova::artifact>());
        }
        if(to_json["type"] == "Artifact"){
            related_fs = std::make_shared<prova::artifact>(to_json.get<prova::artifact>());
        }

        assert(related_ps);
        if(!related_fs){
            continue;
        }

        prova::session::ptr s = std::make_shared<prova::session>(related_fs);
        for(const nlohmann::json& edge_json: record["edges"]){
            bool is_syscall = (edge_json["source"].get<std::string>() == "syscall");
            if(is_syscall){
                std::uint32_t    event_id  = std::stoi(edge_json["event id"].get<std::string>());
                std::string      operation = edge_json["operation"].get<std::string>();
                prova::action::category type = prova::action::category::unknown;

                if(operation == "accept" || operation == "open" || operation == "create" || operation.starts_with("mmap")){
                    type = prova::action::category::acquire;
                } else if (operation == "close" || operation.starts_with("munmap")) {
                    type = prova::action::category::release;
                }

                prova::action::ptr a = std::make_shared<prova::action>(event_id, type);
                a->deserialize(edge_json);
                s->add_action(a);
            }
        }
        related_ps->add_session(s);

        std::size_t pid = related_ps->pid();
        auto it = ps.find(pid);
        if(it != ps.end()){
            prova::process::ptr target_ps = it->second;
            target_ps->merge(related_ps);
        } else {
            ps.insert(std::make_pair(pid, related_ps));
        }
    }
    // }

    // { split big sessions into small sessions
    for(auto& pair: ps){
        std::size_t pid = pair.first;
        std::vector<prova::session::ptr> new_sessions;
        for(auto sess_it = pair.second->begin(); sess_it != pair.second->end();){
            prova::session::ptr s = *sess_it;
            bool should_delete = false;
            if(s->size() > 2){
                // std::cout << "Breaking down" << std::endl;

                for(auto it = s->begin(); it != s->end(); ++it){
                    if((*it)->type() == prova::action::category::acquire){
                        if((*(it+1))->type() == prova::action::category::release){
                            prova::session::ptr new_session = std::make_shared<prova::session>(s->artifact());
                            new_session->add_action(*it);
                            new_session->add_action(*(it+1));
                            new_sessions.push_back(new_session);
                            ++it;
                        }
                    }
                }
                should_delete = true;
            }
            if(should_delete){
                sess_it = pair.second->erase(sess_it);
                should_delete = false;
            } else {
                ++sess_it;
            }
        }
        if(new_sessions.size() > 0){
            pair.second->add_sessions(new_sessions.begin(), new_sessions.end());
            new_sessions.clear();
        }
    }
    // }

    for(auto& pair: ps){
        for(auto sess_it = pair.second->begin(); sess_it != pair.second->end(); ++sess_it){
            prova::session::ptr  s = *sess_it;
            prova::artifact::ptr a = s->artifact();
            std::string a_key      = a->properties()["_key"];

            for(auto child_sess_it = pair.second->begin(); child_sess_it != pair.second->end(); ++child_sess_it){
                prova::session::ptr  child_s = *child_sess_it;
                prova::artifact::ptr child_a = child_s->artifact();
                std::string child_a_key      = child_a->properties()["_key"];

                if(s == child_s){
                    continue;
                }

                bool overlaps = s->completely_overlaps(child_s);
                if(overlaps){
                    s->_children.push_back(child_s);
                    child_s->_parent = s;               // if p = parent(c) then parent(p) should not contain c
                }
            }
        }
    }

    for(auto& pair: ps){
        for(auto sess_it = pair.second->begin(); sess_it != pair.second->end(); ++sess_it){
            prova::session::ptr  s = *sess_it;
            prova::session::ptr  p = s->_parent;
            if(p){
                for(auto it = s->_children.begin(); it != s->_children.end(); ++it){
                    for(auto x_sess_it = p->_children.begin(); x_sess_it != p->_children.end();){
                        if(*x_sess_it == *it){
                            x_sess_it = p->_children.erase(x_sess_it);
                        } else {
                            ++x_sess_it;
                        }
                    }
                }
            }
        }
    }

    // { store the sessions and the artifacts
    std::map<std::string, prova::artifact::ptr> artifacts;
    prova::session_store store;
    for(auto& pair: ps){
        // std::size_t pid = pair.first;
        for(auto sess_it = pair.second->begin(); sess_it != pair.second->end(); ++sess_it){
            prova::session::ptr s = *sess_it;
            s->_process = pair.second;
            prova::artifact::ptr a = s->artifact();
            // std::cout << a->properties() << std::endl;
            std::string a_key = a->properties()["_key"].get<std::string>();
            artifacts.insert(std::make_pair(a_key, a));
            store.insert(s);
        }
    }
    // }

    std::function<void (std::size_t, const prova::session::ptr&, std::uint8_t)> decorate_session;
    decorate_session = [&decorate_session](std::size_t pid, const prova::session::ptr& sess, std::uint8_t indent) -> void {
        for(std::uint8_t i = 0; i != indent; ++i) std::cout << "  ";
        std::cout << std::format("P{} -> {}: <<acquire>> {} [{}]", pid, sess->artifact()->properties()["_key"].get<std::string>(), sess->at(0)->operation(), sess->at(0)->id()) << std::endl;
        if(sess->_children.size() > 0){
            for(const auto& sess_child: sess->_children){
                decorate_session(pid, sess_child, indent +1);
            }
        }
        for(std::uint8_t i = 0; i != indent; ++i) std::cout << "  ";
        std::cout << std::format("P{} --> {}: <<release>> {} [{}]", pid, sess->artifact()->properties()["_key"].get<std::string>(), sess->at(1)->operation(), sess->at(1)->id()) << std::endl;
    };

    std::cout << "@startuml" << std::endl;
    for(const auto& p: ps){
        std::cout << std::format("participant \"{}\" as P{}", p.second->exe(), p.first) << std::endl;
    }
    for(const auto& a: artifacts){
        std::string artifact_name;
        if(a.second->subtype() == "file"){
            artifact_name = a.second->properties()["path"];
        } else if(a.second->subtype() == "network socket"){
            artifact_name = std::format("{}:{}", a.second->properties()["remote address"].get<std::string>(), a.second->properties()["remote port"].get<std::string>());
        } else if(a.second->subtype() == "unnamed pipe") {
            artifact_name = std::format("{} w{} -> r{}", a.second->properties()["_key"].get<std::string>(),  a.second->properties()["write fd"].get<std::string>(), a.second->properties()["read fd"].get<std::string>());
        } else if(a.second->subtype() == "directory") {
            artifact_name = a.second->properties()["path"];
        } else if(a.second->subtype() == "character device") {
            artifact_name = a.second->properties()["path"];
        } else {
           artifact_name = "Unknown";
        }
        std::cout << std::format("participant \"{}\" as {}", artifact_name, a.first) << std::endl;
    }
    auto& index_by_start = store.index_by_first_id();
    for (auto it = index_by_start.begin(); it != index_by_start.end(); ++it) {
        prova::session::ptr sess = *it;
        if(!sess->_parent && sess->_children.size() > 0){
            std::cout << "group " << std::endl;
            decorate_session(sess->_process->pid(), sess, 0);
            std::cout << "end " << std::endl;
        }
    }
    std::cout << "@enduml" << std::endl;


    return 0;
}
