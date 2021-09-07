#ifndef WANDER_INTERNAL_H
#define WANDER_INTERNAL_H

#include <signal.h> /* struct sigaction */

#include "wander_platform.h"

typedef struct wander_handler wander_handler_t;

struct wander_handler {
    int              signo;
#ifdef __unix__
    struct sigaction old_sigaction;
#endif
};
struct wander_handlers {
#ifdef _WIN32
    LPTOP_LEVEL_EXCEPTION_FILTER old_se_handler;
#endif
    size_t num_handlers;
    wander_handler_t handlers[];
};
struct wander_global {
    wander_platform_t platform;
    wander_resolver_t* resolver;
    wander_handlers_t* signal_handlers;
};

WANDER_INTERNAL(struct wander_global) wander_global;

#endif /* !defined(WANDER_INTERNAL_H) */
