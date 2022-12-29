
/*
 * !!! DO NOT EDIT DIRECTLY !!!
 * This file was automatically generated from the following template:
 *
 * src/subsys/njt_subsys_lua_ssl.c.tt2
 */


/*
 * Copyright (C) Yichun Zhang (agentzh)
 * Copyright (C) TMLake, Inc.
 */


#ifndef DDEBUG
#define DDEBUG 0
#endif
#include "ddebug.h"


#if (NJT_STREAM_SSL)


int njt_stream_lua_ssl_ctx_index = -1;


njt_int_t
njt_stream_lua_ssl_init(njt_log_t *log)
{
    if (njt_stream_lua_ssl_ctx_index == -1) {
        njt_stream_lua_ssl_ctx_index = SSL_get_ex_new_index(0, NULL,
                                                            NULL,
                                                            NULL,
                                                            NULL);

        if (njt_stream_lua_ssl_ctx_index == -1) {
            njt_ssl_error(NJT_LOG_ALERT, log, 0,
                          "lua: SSL_get_ex_new_index() for ctx failed");
            return NJT_ERROR;
        }
    }

    return NJT_OK;
}


#endif /* NJT_STREAM_SSL */