
/*
 * Copyright (C) Xiaozhe Wang (chaoslawful)
 * Copyright (C) Yichun Zhang (agentzh)
 */


#ifndef _NJT_HTTP_LUA_LOG_H_INCLUDED_
#define _NJT_HTTP_LUA_LOG_H_INCLUDED_


#include "njt_http_lua_common.h"


void njt_http_lua_inject_log_api(lua_State *L);
#ifdef HAVE_INTERCEPT_ERROR_LOG_PATCH
njt_int_t njt_http_lua_capture_log_handler(njt_log_t *log,
    njt_uint_t level, u_char *buf, size_t n);
#endif


#endif /* _NJT_HTTP_LUA_LOG_H_INCLUDED_ */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
