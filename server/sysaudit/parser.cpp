// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#include "sysaudit/parser.h"
#include "sysaudit/log.h"
#include <iostream>

prova::sysaudit::parser::parser(const std::string_view& view){
    _au = auparse_init(AUSOURCE_BUFFER, view.data());
    if (NULL == _au) {
        std::cerr << "Failed to initialize Auparse. Error: " << std::strerror(errno) << std::endl;
        return;
    }
}
prova::sysaudit::parser::~parser() {
    if (_au) {
        auparse_destroy(_au);
    }
}
void prova::sysaudit::parser::parse(){
    while (auparse_next_event(_au) > 0) {
        parse_event();
    }
}

std::size_t prova::sysaudit::parser::parse_event(){
    if (auparse_first_record(_au) == 0) return 0;
    do {
        char buff[32];
        int type             = auparse_get_type(_au);
        const au_event_t* ev = auparse_get_timestamp(_au);
        std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(ev->sec);
        tp += std::chrono::milliseconds(ev->milli);
        std::size_t serial = ev->serial;

        const char* label    = auparse_get_type_name(_au);
        if (label == NULL) {
            snprintf(buff, sizeof(buff), "%d", type);
            label = buff;
        }
        unsigned nfields = auparse_get_num_fields(_au);

        log entry{type, label, tp, serial, nfields};
        auparse_first_field(_au);
        do{
            const char* name = auparse_get_field_name(_au);
            const char* value = auparse_get_field_str(_au);
            int ftype = auparse_get_field_type(_au);
            entry.add_field(ftype, name ? name : "", value ? value : "");
        } while(auparse_next_field(_au));
        _entries.push_back(entry);

    } while (auparse_next_record(_au) > 0);
    return _entries.size();
}
