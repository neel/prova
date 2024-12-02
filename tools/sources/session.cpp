// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#include "prova/session.h"
#include "prova/action.h"

std::uint32_t prova::session::first_id() const {
		return _actions.front()->id();
}
prova::session::time_type prova::session::start() const {
		return _actions.front()->time();
}
prova::session::time_type prova::session::finish() const {
		return _actions.back()->time();
}
std::uint32_t prova::session::last_id() const {
		return _actions.back()->id();
}

bool prova::session::completely_overlaps(prova::session::ptr other) const {
		// std::cout << std::format("this [{} -> {}], other [{} -> {}]", start(), finish(), other->start(), other->finish()) << std::endl;
		if((other->start() >= start()) && (other->finish() <= finish()))
				return ((other->first_id() > first_id()) && (other->last_id() < last_id()));
		return false;
}
