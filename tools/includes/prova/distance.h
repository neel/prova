// SPDX-FileCopyrightText: 2025 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef DISTANCE_H
#define DISTANCE_H

namespace prova{

struct action;
struct artifact;
struct session;

double distance(const prova::action& l, const prova::action& r);
double distance(const prova::artifact& l, const prova::artifact& r);
double distance(const prova::session& l, const prova::session& r);

}

#endif // DISTANCE_H
