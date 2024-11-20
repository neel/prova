#ifndef AGENGT_AUDIT_AU_H
#define AGENGT_AUDIT_AU_H

#include <auparse.h>

static void handle_event(auparse_state_t* au, auparse_cb_event_t cb_event_type, void* user_data);


#endif // AGENGT_AUDIT_AU_H
