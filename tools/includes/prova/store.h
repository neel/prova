// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef PROVA_STORE_H
#define PROVA_STORE_H


#include <memory>
#include <functional>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/mem_fun.hpp>


#include "prova/process.h"
#include "prova/session.h"
#include "prova/artifact.h"

namespace prova{

struct execution_unit;

struct store{
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

    inline const boost::multi_index::index<session_set, by_first_id>::type&  index_by_first_id() const { return _sessions.get<by_first_id>();  }
    inline const boost::multi_index::index<session_set, by_last_id>::type& index_by_last_id() const { return _sessions.get<by_last_id>(); }
    inline const boost::multi_index::index<session_set, by_start>::type&  index_by_start() const { return _sessions.get<by_start>();  }
    inline const boost::multi_index::index<session_set, by_finish>::type& index_by_finish() const { return _sessions.get<by_finish>(); }

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

    bool insert(prova::session::ptr session);

		void fetch();
    std::ostream& uml(std::ostream& stream) const;
    void extract(std::vector<std::shared_ptr<prova::execution_unit>>& units);

		std::map<std::size_t, prova::process::ptr> _processes;
    std::map<std::string, prova::artifact::ptr> _artifacts;
};

}

#endif // PROVA_STORE_H
