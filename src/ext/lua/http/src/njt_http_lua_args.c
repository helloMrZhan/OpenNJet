
/*
 * Copyright (C) Xiaozhe Wang (chaoslawful)
 * Copyright (C) Yichun Zhang (agentzh)
 * Copyright (C) 2021-2023  TMLake(Beijing) Technology Co., Ltd.
 */


#ifndef DDEBUG
#define DDEBUG 0
#endif
#include "ddebug.h"


#include "njt_http_lua_args.h"
#include "njt_http_lua_util.h"


static int njt_http_lua_njt_req_set_uri_args(lua_State *L);
static int njt_http_lua_njt_req_get_post_args(lua_State *L);


uintptr_t
njt_http_lua_escape_args(u_char *dst, u_char *src, size_t size)
{
    njt_uint_t      n;
    static u_char   hex[] = "0123456789ABCDEF";

                    /* %00-%20 %7F*/

    static uint32_t   escape[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x00000001, /* 0000 0000 0000 0000  0000 0000 0000 0001 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */

        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    };

    if (dst == NULL) {

        /* find the number of the characters to be escaped */

        n = 0;

        while (size) {
            if (escape[*src >> 5] & (1 << (*src & 0x1f))) {
                n++;
            }

            src++;
            size--;
        }

        return (uintptr_t) n;
    }

    while (size) {
        if (escape[*src >> 5] & (1 << (*src & 0x1f))) {
            *dst++ = '%';
            *dst++ = hex[*src >> 4];
            *dst++ = hex[*src & 0xf];
            src++;

        } else {
            *dst++ = *src++;
        }

        size--;
    }

    return (uintptr_t) dst;
}


static int
njt_http_lua_njt_req_set_uri_args(lua_State *L)
{
    njt_http_request_t          *r;
    njt_str_t                    args;
    const char                  *msg;
    size_t                       len;
    u_char                      *p;
    uintptr_t                    escape;

    if (lua_gettop(L) != 1) {
        return luaL_error(L, "expecting 1 argument but seen %d",
                          lua_gettop(L));
    }

    r = njt_http_lua_get_req(L);
    if (r == NULL) {
        return luaL_error(L, "no request object found");
    }

    njt_http_lua_check_fake_request(L, r);

    switch (lua_type(L, 1)) {
    case LUA_TNUMBER:
        p = (u_char *) lua_tolstring(L, 1, &len);

        args.data = njt_palloc(r->pool, len);
        if (args.data == NULL) {
            return luaL_error(L, "no memory");
        }

        njt_memcpy(args.data, p, len);

        args.len = len;
        break;

    case LUA_TSTRING:
        p = (u_char *) lua_tolstring(L, 1, &len);

        escape = njt_http_lua_escape_args(NULL, p, len);
        if (escape > 0) {
            args.len = len + 2 * escape;
            args.data = njt_palloc(r->pool, args.len);
            if (args.data == NULL) {
                return NJT_ERROR;
            }

            njt_http_lua_escape_args(args.data, p, len);

        } else {
            args.data = njt_palloc(r->pool, len);
            if (args.data == NULL) {
                return luaL_error(L, "no memory");
            }

            njt_memcpy(args.data, p, len);

            args.len = len;
        }

        break;

    case LUA_TTABLE:
        njt_http_lua_process_args_option(r, L, 1, &args);

        dd("args: %.*s", (int) args.len, args.data);

        break;

    default:
        msg = lua_pushfstring(L, "string, number, or table expected, "
                              "but got %s", luaL_typename(L, 1));
        return luaL_argerror(L, 1, msg);
    }

    dd("args: %.*s", (int) args.len, args.data);

    r->args.data = args.data;
    r->args.len = args.len;

    r->valid_unparsed_uri = 0;

    return 0;
}


static int
njt_http_lua_njt_req_get_post_args(lua_State *L)
{
    njt_http_request_t          *r;
    u_char                      *buf;
    int                          retval;
    size_t                       len;
    njt_chain_t                 *cl;
    u_char                      *p;
    u_char                      *last;
    int                          n;
    int                          max;

    n = lua_gettop(L);

    if (n != 0 && n != 1) {
        return luaL_error(L, "expecting 0 or 1 arguments but seen %d", n);
    }

    if (n == 1) {
        max = luaL_checkinteger(L, 1);
        lua_pop(L, 1);

    } else {
        max = NJT_HTTP_LUA_MAX_ARGS;
    }

    r = njt_http_lua_get_req(L);
    if (r == NULL) {
        return luaL_error(L, "no request object found");
    }

    njt_http_lua_check_fake_request(L, r);

    if (r->discard_body) {
        lua_createtable(L, 0, 0);
        return 1;
    }

    if (r->request_body == NULL) {
        return luaL_error(L, "no request body found; "
                          "maybe you should turn on lua_need_request_body?");
    }

    if (r->request_body->temp_file) {
        lua_pushnil(L);
        lua_pushliteral(L, "request body in temp file not supported");
        return 2;
    }

    if (r->request_body->bufs == NULL) {
        lua_createtable(L, 0, 0);
        return 1;
    }

    /* we copy r->request_body->bufs over to buf to simplify
     * unescaping query arg keys and values */

    len = 0;
    for (cl = r->request_body->bufs; cl; cl = cl->next) {
        len += cl->buf->last - cl->buf->pos;
    }

    dd("post body length: %d", (int) len);

    if (len == 0) {
        lua_createtable(L, 0, 0);
        return 1;
    }

    buf = njt_palloc(r->pool, len);
    if (buf == NULL) {
        return luaL_error(L, "no memory");
    }

    lua_createtable(L, 0, 4);

    p = buf;
    for (cl = r->request_body->bufs; cl; cl = cl->next) {
        p = njt_copy(p, cl->buf->pos, cl->buf->last - cl->buf->pos);
    }

    dd("post body: %.*s", (int) len, buf);

    last = buf + len;

    retval = njt_http_lua_parse_args(L, buf, last, max);

    njt_pfree(r->pool, buf);

    return retval;
}


int
njt_http_lua_parse_args(lua_State *L, u_char *buf, u_char *last, int max)
{
    u_char                      *p, *q;
    u_char                      *src, *dst;
    unsigned                     parsing_value;
    size_t                       len;
    int                          count = 0;
    int                          top;

    top = lua_gettop(L);

    p = buf;

    parsing_value = 0;
    q = p;

    while (p != last) {
        if (*p == '=' && ! parsing_value) {
            /* key data is between p and q */

            src = q; dst = q;

            njt_http_lua_unescape_uri(&dst, &src, p - q,
                                      NJT_UNESCAPE_URI_COMPONENT);

            dd("pushing key %.*s", (int) (dst - q), q);

            /* push the key */
            lua_pushlstring(L, (char *) q, dst - q);

            /* skip the current '=' char */
            p++;

            q = p;
            parsing_value = 1;

        } else if (*p == '&') {
            /* reached the end of a key or a value, just save it */
            src = q; dst = q;

            njt_http_lua_unescape_uri(&dst, &src, p - q,
                                      NJT_UNESCAPE_URI_COMPONENT);

            dd("pushing key or value %.*s", (int) (dst - q), q);

            /* push the value or key */
            lua_pushlstring(L, (char *) q, dst - q);

            /* skip the current '&' char */
            p++;

            q = p;

            if (parsing_value) {
                /* end of the current pair's value */
                parsing_value = 0;

            } else {
                /* the current parsing pair takes no value,
                 * just push the value "true" */
                dd("pushing boolean true");

                lua_pushboolean(L, 1);
            }

            (void) lua_tolstring(L, -2, &len);

            if (len == 0) {
                /* ignore empty string key pairs */
                dd("popping key and value...");
                lua_pop(L, 2);

            } else {
                dd("setting table...");
                njt_http_lua_set_multi_value_table(L, top);
            }

            if (max > 0 && ++count == max) {
                lua_pushliteral(L, "truncated");

                njt_log_debug1(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                               "lua hit query args limit %d", max);
                return 2;
            }

        } else {
            p++;
        }
    }

    if (p != q || parsing_value) {
        src = q; dst = q;

        njt_http_lua_unescape_uri(&dst, &src, p - q,
                                  NJT_UNESCAPE_URI_COMPONENT);

        dd("pushing key or value %.*s", (int) (dst - q), q);

        lua_pushlstring(L, (char *) q, dst - q);

        if (!parsing_value) {
            dd("pushing boolean true...");
            lua_pushboolean(L, 1);
        }

        (void) lua_tolstring(L, -2, &len);

        if (len == 0) {
            /* ignore empty string key pairs */
            dd("popping key and value...");
            lua_pop(L, 2);

        } else {
            dd("setting table...");
            njt_http_lua_set_multi_value_table(L, top);
        }
    }

    dd("gettop: %d", lua_gettop(L));
    dd("type: %s", lua_typename(L, lua_type(L, 1)));

    if (lua_gettop(L) != top) {
        return luaL_error(L, "internal error: stack in bad state");
    }

    return 1;
}


void
njt_http_lua_inject_req_args_api(lua_State *L)
{
    lua_pushcfunction(L, njt_http_lua_njt_req_set_uri_args);
    lua_setfield(L, -2, "set_uri_args");

    lua_pushcfunction(L, njt_http_lua_njt_req_get_post_args);
    lua_setfield(L, -2, "get_post_args");
}


size_t
njt_http_lua_ffi_req_get_querystring_len(njt_http_request_t *r)
{
    return r->args.len;
}


int
njt_http_lua_ffi_req_get_uri_args_count(njt_http_request_t *r, int max,
    int *truncated)
{
    int                      count;
    u_char                  *p, *last;

    if (r->connection->fd == (njt_socket_t) -1) {
        return NJT_HTTP_LUA_FFI_BAD_CONTEXT;
    }

    *truncated = 0;

    if (max < 0) {
        max = NJT_HTTP_LUA_MAX_ARGS;
    }

    last = r->args.data + r->args.len;
    count = 0;

    for (p = r->args.data; p != last; p++) {
        if (*p == '&') {
            if (count == 0) {
                count += 2;

            } else {
                count++;
            }
        }
    }

    if (count) {
        if (max > 0 && count > max) {
            count = max;
            *truncated = 1;
            njt_log_debug1(NJT_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "lua hit query args limit %d", max);
        }

        return count;
    }

    if (r->args.len) {
        return 1;
    }

    return 0;
}


int
njt_http_lua_ffi_req_get_uri_args(njt_http_request_t *r, u_char *buf,
    njt_http_lua_ffi_table_elt_t *out, int count)
{
    int                          i, parsing_value = 0;
    u_char                      *last, *p, *q;
    u_char                      *src, *dst;

    if (count <= 0) {
        return NJT_OK;
    }

    njt_memcpy(buf, r->args.data, r->args.len);

    i = 0;
    last = buf + r->args.len;
    p = buf;
    q = p;

    while (p != last) {
        if (*p == '=' && !parsing_value) {
            /* key data is between p and q */

            src = q; dst = q;

            njt_http_lua_unescape_uri(&dst, &src, p - q,
                                      NJT_UNESCAPE_URI_COMPONENT);

            dd("saving key %.*s", (int) (dst - q), q);

            out[i].key.data = q;
            out[i].key.len = (int) (dst - q);

            /* skip the current '=' char */
            p++;

            q = p;
            parsing_value = 1;

        } else if (*p == '&') {
            /* reached the end of a key or a value, just save it */
            src = q; dst = q;

            njt_http_lua_unescape_uri(&dst, &src, p - q,
                                      NJT_UNESCAPE_URI_COMPONENT);

            dd("pushing key or value %.*s", (int) (dst - q), q);

            if (parsing_value) {
                /* end of the current pair's value */
                parsing_value = 0;

                if (out[i].key.len) {
                    out[i].value.data = q;
                    out[i].value.len = (int) (dst - q);
                    i++;
                }

            } else {
                /* the current parsing pair takes no value,
                 * just push the value "true" */
                dd("pushing boolean true");

                if (dst - q) {
                    out[i].key.data = q;
                    out[i].key.len = (int) (dst - q);
                    out[i].value.len = -1;
                    i++;
                }
            }

            if (i == count) {
                return i;
            }

            /* skip the current '&' char */
            p++;

            q = p;

        } else {
            p++;
        }
    }

    if (p != q || parsing_value) {
        src = q; dst = q;

        njt_http_lua_unescape_uri(&dst, &src, p - q,
                                  NJT_UNESCAPE_URI_COMPONENT);

        dd("pushing key or value %.*s", (int) (dst - q), q);

        if (parsing_value) {
            if (out[i].key.len) {
                out[i].value.data = q;
                out[i].value.len = (int) (dst - q);
                i++;
            }

        } else {
            if (dst - q) {
                out[i].key.data = q;
                out[i].key.len = (int) (dst - q);
                out[i].value.len = (int) -1;
                i++;
            }
        }
    }

    return i;
}


/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
