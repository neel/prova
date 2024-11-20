#include "au.h"
#include <stdio.h>

static void handle_event(auparse_state_t* au, auparse_cb_event_t cb_event_type, void* user_data){
    if (cb_event_type != AUPARSE_CB_EVENT_READY)
    return;

    auparse_first_record(au);
    do {
        char buff[32];
        int type          = auparse_get_type(au);
        const char* label = auparse_get_type_name(au);
        if (label == NULL) {
            snprintf(buff, sizeof(buff), "%d", type);
            label = buff;
        }

        do {
            int type          = auparse_get_field_type(au);
            const char* name  = auparse_get_field_name(au);
            const char* value = auparse_get_field_str(au);
            printf("[%d] %s: %s; ", type, name, value);
        } while (auparse_next_field(au) > 0);

    } while (auparse_next_record(au) > 0);
}
