// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef SYSAUDIT_PARSER_H
#define SYSAUDIT_PARSER_H

#include <string_view>
#include <auparse.h>
#include <vector>

namespace prova::sysaudit{

    struct log;

    struct parser{
        parser(const std::string_view& view);
        ~parser();
        void parse();
        inline const std::vector<log>& entries() const { return _entries; }
        private:
            std::size_t parse_event();
        private:
            auparse_state_t* _au = 0x0;
            std::vector<log> _entries;
    };

}

#endif // SYSAUDIT_PARSER_H
