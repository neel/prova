// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef SYSAUDIT_REPORTER_H
#define SYSAUDIT_REPORTER_H

#include <string>
#include <vector>
#include <tash/arango.h>

namespace prova::sysaudit{

    struct log;

    struct reporter{
        reporter(const std::string& agent);
        void report(const std::vector<log>& entries);
        private:
            tash::shell      _shell;
            tash::vertex     _events;
            std::string      _agent;
    };

}

#endif // SYSAUDIT_REPORTER_H
