

/* This file was generated by JSON Schema to C.
 * Any changes made to it will be lost on regeneration. 

 * Copyright (C) 2021-2023  TMLake(Beijing) Technology Co., Ltd.
 */

#include "njt_http_upstream_api_server_list_parser.h"
#include "njt_core.h"
#include "js2c_njet_builtins.h"
/* ========================== Generated parsers ========================== */

 bool parse_server_list_serverDef(njt_pool_t *pool, parse_state_t *parse_state, server_list_serverDef_t *out, js2c_parse_error_t *err_ret); //forward decl for public definition
 void get_json_length_server_list_serverDef(njt_pool_t *pool, server_list_serverDef_t *out, size_t *length, njt_int_t flags); //forward decl for public definition
 void to_oneline_json_server_list_serverDef(njt_pool_t *pool, server_list_serverDef_t *out, njt_str_t *buf, njt_int_t flags); //forward decl for public definition

 bool parse_server_list_serverDef(njt_pool_t *pool, parse_state_t *parse_state, server_list_serverDef_t *out, js2c_parse_error_t *err_ret) {
    njt_uint_t i;

    js2c_check_type(JSMN_OBJECT);
    const int object_start_token = parse_state->current_token;
    const uint64_t n = parse_state->tokens[parse_state->current_token].size;
    parse_state->current_token += 1;
    for (i = 0; i < n; ++i) {
        js2c_key_children_check_for_obj();
        if (current_string_is(parse_state, "id")) {
            js2c_check_field_set(out->is_id_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "id";
            int64_t int_parse_tmp;
            if (builtin_parse_signed(pool, parse_state, true, false, 10, &int_parse_tmp, err_ret)) {
                return true;
            }
            js2c_int_range_check_min(0LL);
            *(&out->id) = int_parse_tmp;
            out->is_id_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "server")) {
            js2c_check_field_set(out->is_server_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "server";
            int token_size =  CURRENT_STRING_LENGTH(parse_state) ;
            ((&out->server))->data = (u_char*)njt_pcalloc(pool, (size_t)(token_size + 1));
            js2c_malloc_check(((&out->server))->data);
            ((&out->server))->len = token_size;
            if (builtin_parse_string(pool, parse_state, (&out->server), 0, ((&out->server))->len, err_ret)) {
                return true;
            }
            out->is_server_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "name")) {
            js2c_check_field_set(out->is_name_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "name";
            int token_size =  CURRENT_STRING_LENGTH(parse_state) ;
            ((&out->name))->data = (u_char*)njt_pcalloc(pool, (size_t)(token_size + 1));
            js2c_malloc_check(((&out->name))->data);
            ((&out->name))->len = token_size;
            if (builtin_parse_string(pool, parse_state, (&out->name), 0, ((&out->name))->len, err_ret)) {
                return true;
            }
            out->is_name_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "weight")) {
            js2c_check_field_set(out->is_weight_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "weight";
            int64_t int_parse_tmp;
            if (builtin_parse_signed(pool, parse_state, true, false, 10, &int_parse_tmp, err_ret)) {
                return true;
            }
            js2c_int_range_check_min(0LL);
            *(&out->weight) = int_parse_tmp;
            out->is_weight_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "max_conns")) {
            js2c_check_field_set(out->is_max_conns_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "max_conns";
            int64_t int_parse_tmp;
            if (builtin_parse_signed(pool, parse_state, true, false, 10, &int_parse_tmp, err_ret)) {
                return true;
            }
            js2c_int_range_check_min(0LL);
            *(&out->max_conns) = int_parse_tmp;
            out->is_max_conns_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "max_fails")) {
            js2c_check_field_set(out->is_max_fails_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "max_fails";
            int64_t int_parse_tmp;
            if (builtin_parse_signed(pool, parse_state, true, false, 10, &int_parse_tmp, err_ret)) {
                return true;
            }
            js2c_int_range_check_min(0LL);
            *(&out->max_fails) = int_parse_tmp;
            out->is_max_fails_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "fail_timeout")) {
            js2c_check_field_set(out->is_fail_timeout_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "fail_timeout";
            int token_size =  CURRENT_STRING_LENGTH(parse_state) ;
            ((&out->fail_timeout))->data = (u_char*)njt_pcalloc(pool, (size_t)(token_size + 1));
            js2c_malloc_check(((&out->fail_timeout))->data);
            ((&out->fail_timeout))->len = token_size;
            if (builtin_parse_string(pool, parse_state, (&out->fail_timeout), 0, ((&out->fail_timeout))->len, err_ret)) {
                return true;
            }
            out->is_fail_timeout_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "slow_start")) {
            js2c_check_field_set(out->is_slow_start_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "slow_start";
            int token_size =  CURRENT_STRING_LENGTH(parse_state) ;
            ((&out->slow_start))->data = (u_char*)njt_pcalloc(pool, (size_t)(token_size + 1));
            js2c_malloc_check(((&out->slow_start))->data);
            ((&out->slow_start))->len = token_size;
            if (builtin_parse_string(pool, parse_state, (&out->slow_start), 0, ((&out->slow_start))->len, err_ret)) {
                return true;
            }
            out->is_slow_start_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "route")) {
            js2c_check_field_set(out->is_route_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "route";
            js2c_null_check();
            int token_size =  CURRENT_STRING_LENGTH(parse_state) ;
            ((&out->route))->data = (u_char*)njt_pcalloc(pool, (size_t)(token_size + 1));
            js2c_malloc_check(((&out->route))->data);
            ((&out->route))->len = token_size;
            if (builtin_parse_string(pool, parse_state, (&out->route), 0, ((&out->route))->len, err_ret)) {
                return true;
            }
            out->is_route_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "backup")) {
            js2c_check_field_set(out->is_backup_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "backup";
            if (builtin_parse_bool(pool, parse_state, (&out->backup), err_ret)) {
                return true;
            }
            out->is_backup_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "down")) {
            js2c_check_field_set(out->is_down_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "down";
            if (builtin_parse_bool(pool, parse_state, (&out->down), err_ret)) {
                return true;
            }
            out->is_down_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "parent")) {
            js2c_check_field_set(out->is_parent_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "parent";
            js2c_null_check();
            int64_t int_parse_tmp;
            if (builtin_parse_signed(pool, parse_state, true, false, 10, &int_parse_tmp, err_ret)) {
                return true;
            }
            js2c_int_range_check_min(0LL);
            *(&out->parent) = int_parse_tmp;
            out->is_parent_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "host")) {
            js2c_check_field_set(out->is_host_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "host";
            js2c_null_check();
            int token_size =  CURRENT_STRING_LENGTH(parse_state) ;
            ((&out->host))->data = (u_char*)njt_pcalloc(pool, (size_t)(token_size + 1));
            js2c_malloc_check(((&out->host))->data);
            ((&out->host))->len = token_size;
            if (builtin_parse_string(pool, parse_state, (&out->host), 0, ((&out->host))->len, err_ret)) {
                return true;
            }
            out->is_host_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "drain")) {
            js2c_check_field_set(out->is_drain_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "drain";
            js2c_null_check();
            if (builtin_parse_bool(pool, parse_state, (&out->drain), err_ret)) {
                return true;
            }
            out->is_drain_set = 1;
            parse_state->current_key = saved_key;
        } else {
            LOG_ERROR_JSON_PARSE(UNKNOWN_FIELD_ERR, parse_state->current_key, CURRENT_TOKEN(parse_state).start, "Unknown field in '%s': %.*s", parse_state->current_key, CURRENT_STRING_FOR_ERROR(parse_state));
            return true;
        }
    }
    const int saved_current_token = parse_state->current_token;
    parse_state->current_token = object_start_token;
    // set default
    if (!out->is_route_set) {
        size_t token_size = strlen("");
        (out->route).data = (u_char*)njt_pcalloc(pool, token_size + 1);
        js2c_malloc_check((out->route).data);
        (out->route).len = token_size;
        if (out->route.len == 0) {
            (out->route).data[0] = 0;
        }
        if (token_size > 0) {
            njt_memcpy(out->route.data, "", token_size);
        }
    }
    // set default
    if (!out->is_parent_set) {
        out->parent = 0LL;
    }
    // set default
    if (!out->is_host_set) {
        size_t token_size = strlen("");
        (out->host).data = (u_char*)njt_pcalloc(pool, token_size + 1);
        js2c_malloc_check((out->host).data);
        (out->host).len = token_size;
        if (out->host.len == 0) {
            (out->host).data[0] = 0;
        }
        if (token_size > 0) {
            njt_memcpy(out->host.data, "", token_size);
        }
    }
    // set default
    if (!out->is_drain_set) {
        out->drain = false;
    }
    parse_state->current_token = saved_current_token;
    return false;
}


static void get_json_length_server_list_serverDef_id(njt_pool_t *pool, server_list_serverDef_id_t *out, size_t *length, njt_int_t flags) {
    u_char str[24];
    u_char *cur;
    cur = njt_sprintf(str, "%L", *out);
    *length += cur - str;
}

static void get_json_length_server_list_serverDef_server(njt_pool_t *pool, server_list_serverDef_server_t *out, size_t *length, njt_int_t flags) {
    njt_str_t *dst = handle_escape_on_write(pool, out);
    *length += dst->len + 2; //  "str" 
}

static void get_json_length_server_list_serverDef_name(njt_pool_t *pool, server_list_serverDef_name_t *out, size_t *length, njt_int_t flags) {
    njt_str_t *dst = handle_escape_on_write(pool, out);
    *length += dst->len + 2; //  "str" 
}

static void get_json_length_server_list_serverDef_weight(njt_pool_t *pool, server_list_serverDef_weight_t *out, size_t *length, njt_int_t flags) {
    u_char str[24];
    u_char *cur;
    cur = njt_sprintf(str, "%L", *out);
    *length += cur - str;
}

static void get_json_length_server_list_serverDef_max_conns(njt_pool_t *pool, server_list_serverDef_max_conns_t *out, size_t *length, njt_int_t flags) {
    u_char str[24];
    u_char *cur;
    cur = njt_sprintf(str, "%L", *out);
    *length += cur - str;
}

static void get_json_length_server_list_serverDef_max_fails(njt_pool_t *pool, server_list_serverDef_max_fails_t *out, size_t *length, njt_int_t flags) {
    u_char str[24];
    u_char *cur;
    cur = njt_sprintf(str, "%L", *out);
    *length += cur - str;
}

static void get_json_length_server_list_serverDef_fail_timeout(njt_pool_t *pool, server_list_serverDef_fail_timeout_t *out, size_t *length, njt_int_t flags) {
    njt_str_t *dst = handle_escape_on_write(pool, out);
    *length += dst->len + 2; //  "str" 
}

static void get_json_length_server_list_serverDef_slow_start(njt_pool_t *pool, server_list_serverDef_slow_start_t *out, size_t *length, njt_int_t flags) {
    njt_str_t *dst = handle_escape_on_write(pool, out);
    *length += dst->len + 2; //  "str" 
}

static void get_json_length_server_list_serverDef_route(njt_pool_t *pool, server_list_serverDef_route_t *out, size_t *length, njt_int_t flags) {
    njt_str_t *dst = handle_escape_on_write(pool, out);
    *length += dst->len + 2; //  "str" 
}

static void get_json_length_server_list_serverDef_backup(njt_pool_t *pool, server_list_serverDef_backup_t *out, size_t *length, njt_int_t flags) {
    if (*out) {
        *length += 4; // "true"
    } else {
        *length += 5; // "false"
    }
}

static void get_json_length_server_list_serverDef_down(njt_pool_t *pool, server_list_serverDef_down_t *out, size_t *length, njt_int_t flags) {
    if (*out) {
        *length += 4; // "true"
    } else {
        *length += 5; // "false"
    }
}

static void get_json_length_server_list_serverDef_parent(njt_pool_t *pool, server_list_serverDef_parent_t *out, size_t *length, njt_int_t flags) {
    u_char str[24];
    u_char *cur;
    cur = njt_sprintf(str, "%L", *out);
    *length += cur - str;
}

static void get_json_length_server_list_serverDef_host(njt_pool_t *pool, server_list_serverDef_host_t *out, size_t *length, njt_int_t flags) {
    njt_str_t *dst = handle_escape_on_write(pool, out);
    *length += dst->len + 2; //  "str" 
}

static void get_json_length_server_list_serverDef_drain(njt_pool_t *pool, server_list_serverDef_drain_t *out, size_t *length, njt_int_t flags) {
    if (*out) {
        *length += 4; // "true"
    } else {
        *length += 5; // "false"
    }
}

 void get_json_length_server_list_serverDef(njt_pool_t *pool, server_list_serverDef_t *out, size_t *length, njt_int_t flags) {
    if (out == NULL) {
        *length += 4; // null
        return;
    }
    *length += 1;
    njt_int_t omit;
    njt_int_t count = 0;
    omit = 0;
    omit = out->is_id_set ? 0 : 1;
    if (omit == 0) {
        *length += (2 + 3); // "id": 
        get_json_length_server_list_serverDef_id(pool, (&out->id), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_server_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->server.data) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (6 + 3); // "server": 
        get_json_length_server_list_serverDef_server(pool, (&out->server), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_name_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->name.data) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (4 + 3); // "name": 
        get_json_length_server_list_serverDef_name(pool, (&out->name), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_weight_set ? 0 : 1;
    if (omit == 0) {
        *length += (6 + 3); // "weight": 
        get_json_length_server_list_serverDef_weight(pool, (&out->weight), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_max_conns_set ? 0 : 1;
    if (omit == 0) {
        *length += (9 + 3); // "max_conns": 
        get_json_length_server_list_serverDef_max_conns(pool, (&out->max_conns), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_max_fails_set ? 0 : 1;
    if (omit == 0) {
        *length += (9 + 3); // "max_fails": 
        get_json_length_server_list_serverDef_max_fails(pool, (&out->max_fails), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_fail_timeout_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->fail_timeout.data) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (12 + 3); // "fail_timeout": 
        get_json_length_server_list_serverDef_fail_timeout(pool, (&out->fail_timeout), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_slow_start_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->slow_start.data) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (10 + 3); // "slow_start": 
        get_json_length_server_list_serverDef_slow_start(pool, (&out->slow_start), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_route_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->route.data) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (5 + 3); // "route": 
        get_json_length_server_list_serverDef_route(pool, (&out->route), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_backup_set ? 0 : 1;
    if (omit == 0) {
        *length += (6 + 3); // "backup": 
        get_json_length_server_list_serverDef_backup(pool, (&out->backup), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_down_set ? 0 : 1;
    if (omit == 0) {
        *length += (4 + 3); // "down": 
        get_json_length_server_list_serverDef_down(pool, (&out->down), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_parent_set ? 0 : 1;
    if (omit == 0) {
        *length += (6 + 3); // "parent": 
        get_json_length_server_list_serverDef_parent(pool, (&out->parent), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_host_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->host.data) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (4 + 3); // "host": 
        get_json_length_server_list_serverDef_host(pool, (&out->host), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_drain_set ? 0 : 1;
    if (omit == 0) {
        *length += (5 + 3); // "drain": 
        get_json_length_server_list_serverDef_drain(pool, (&out->drain), length, flags);
        *length += 1; // ","
        count++;
    }
    if (count != 0) {
        *length -= 1; // "\b"
    }
    *length += 1;
}

server_list_serverDef_id_t get_server_list_serverDef_id(server_list_serverDef_t *out) {
    return out->id;
}

server_list_serverDef_server_t* get_server_list_serverDef_server(server_list_serverDef_t *out) {
    return &out->server;
}

server_list_serverDef_name_t* get_server_list_serverDef_name(server_list_serverDef_t *out) {
    return &out->name;
}

server_list_serverDef_weight_t get_server_list_serverDef_weight(server_list_serverDef_t *out) {
    return out->weight;
}

server_list_serverDef_max_conns_t get_server_list_serverDef_max_conns(server_list_serverDef_t *out) {
    return out->max_conns;
}

server_list_serverDef_max_fails_t get_server_list_serverDef_max_fails(server_list_serverDef_t *out) {
    return out->max_fails;
}

server_list_serverDef_fail_timeout_t* get_server_list_serverDef_fail_timeout(server_list_serverDef_t *out) {
    return &out->fail_timeout;
}

server_list_serverDef_slow_start_t* get_server_list_serverDef_slow_start(server_list_serverDef_t *out) {
    return &out->slow_start;
}

server_list_serverDef_route_t* get_server_list_serverDef_route(server_list_serverDef_t *out) {
    return &out->route;
}

server_list_serverDef_backup_t get_server_list_serverDef_backup(server_list_serverDef_t *out) {
    return out->backup;
}

server_list_serverDef_down_t get_server_list_serverDef_down(server_list_serverDef_t *out) {
    return out->down;
}

server_list_serverDef_parent_t get_server_list_serverDef_parent(server_list_serverDef_t *out) {
    return out->parent;
}

server_list_serverDef_host_t* get_server_list_serverDef_host(server_list_serverDef_t *out) {
    return &out->host;
}

server_list_serverDef_drain_t get_server_list_serverDef_drain(server_list_serverDef_t *out) {
    return out->drain;
}
void set_server_list_serverDef_id(server_list_serverDef_t* obj, server_list_serverDef_id_t field) {
    obj->id = field;
    obj->is_id_set = 1;
}
void set_server_list_serverDef_server(server_list_serverDef_t* obj, server_list_serverDef_server_t* field) {
    njt_memcpy(&obj->server, field, sizeof(njt_str_t));
    obj->is_server_set = 1;
}
void set_server_list_serverDef_name(server_list_serverDef_t* obj, server_list_serverDef_name_t* field) {
    njt_memcpy(&obj->name, field, sizeof(njt_str_t));
    obj->is_name_set = 1;
}
void set_server_list_serverDef_weight(server_list_serverDef_t* obj, server_list_serverDef_weight_t field) {
    obj->weight = field;
    obj->is_weight_set = 1;
}
void set_server_list_serverDef_max_conns(server_list_serverDef_t* obj, server_list_serverDef_max_conns_t field) {
    obj->max_conns = field;
    obj->is_max_conns_set = 1;
}
void set_server_list_serverDef_max_fails(server_list_serverDef_t* obj, server_list_serverDef_max_fails_t field) {
    obj->max_fails = field;
    obj->is_max_fails_set = 1;
}
void set_server_list_serverDef_fail_timeout(server_list_serverDef_t* obj, server_list_serverDef_fail_timeout_t* field) {
    njt_memcpy(&obj->fail_timeout, field, sizeof(njt_str_t));
    obj->is_fail_timeout_set = 1;
}
void set_server_list_serverDef_slow_start(server_list_serverDef_t* obj, server_list_serverDef_slow_start_t* field) {
    njt_memcpy(&obj->slow_start, field, sizeof(njt_str_t));
    obj->is_slow_start_set = 1;
}
void set_server_list_serverDef_route(server_list_serverDef_t* obj, server_list_serverDef_route_t* field) {
    njt_memcpy(&obj->route, field, sizeof(njt_str_t));
    obj->is_route_set = 1;
}
void set_server_list_serverDef_backup(server_list_serverDef_t* obj, server_list_serverDef_backup_t field) {
    obj->backup = field;
    obj->is_backup_set = 1;
}
void set_server_list_serverDef_down(server_list_serverDef_t* obj, server_list_serverDef_down_t field) {
    obj->down = field;
    obj->is_down_set = 1;
}
void set_server_list_serverDef_parent(server_list_serverDef_t* obj, server_list_serverDef_parent_t field) {
    obj->parent = field;
    obj->is_parent_set = 1;
}
void set_server_list_serverDef_host(server_list_serverDef_t* obj, server_list_serverDef_host_t* field) {
    njt_memcpy(&obj->host, field, sizeof(njt_str_t));
    obj->is_host_set = 1;
}
void set_server_list_serverDef_drain(server_list_serverDef_t* obj, server_list_serverDef_drain_t field) {
    obj->drain = field;
    obj->is_drain_set = 1;
}
server_list_serverDef_t* create_server_list_serverDef(njt_pool_t *pool) {
    server_list_serverDef_t* out = njt_pcalloc(pool, sizeof(server_list_serverDef_t));
    return out;
}

static void to_oneline_json_server_list_serverDef_id(njt_pool_t *pool, server_list_serverDef_id_t *out, njt_str_t* buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    cur = njt_sprintf(cur, "%L", *out);
    buf->len = cur - buf->data;
}

static void to_oneline_json_server_list_serverDef_server(njt_pool_t *pool, server_list_serverDef_server_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    njt_str_t *dst = handle_escape_on_write(pool, out);
    cur = njt_sprintf(cur, "\"%V\"", dst);
    buf->len = cur - buf->data;
}

static void to_oneline_json_server_list_serverDef_name(njt_pool_t *pool, server_list_serverDef_name_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    njt_str_t *dst = handle_escape_on_write(pool, out);
    cur = njt_sprintf(cur, "\"%V\"", dst);
    buf->len = cur - buf->data;
}

static void to_oneline_json_server_list_serverDef_weight(njt_pool_t *pool, server_list_serverDef_weight_t *out, njt_str_t* buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    cur = njt_sprintf(cur, "%L", *out);
    buf->len = cur - buf->data;
}

static void to_oneline_json_server_list_serverDef_max_conns(njt_pool_t *pool, server_list_serverDef_max_conns_t *out, njt_str_t* buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    cur = njt_sprintf(cur, "%L", *out);
    buf->len = cur - buf->data;
}

static void to_oneline_json_server_list_serverDef_max_fails(njt_pool_t *pool, server_list_serverDef_max_fails_t *out, njt_str_t* buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    cur = njt_sprintf(cur, "%L", *out);
    buf->len = cur - buf->data;
}

static void to_oneline_json_server_list_serverDef_fail_timeout(njt_pool_t *pool, server_list_serverDef_fail_timeout_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    njt_str_t *dst = handle_escape_on_write(pool, out);
    cur = njt_sprintf(cur, "\"%V\"", dst);
    buf->len = cur - buf->data;
}

static void to_oneline_json_server_list_serverDef_slow_start(njt_pool_t *pool, server_list_serverDef_slow_start_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    njt_str_t *dst = handle_escape_on_write(pool, out);
    cur = njt_sprintf(cur, "\"%V\"", dst);
    buf->len = cur - buf->data;
}

static void to_oneline_json_server_list_serverDef_route(njt_pool_t *pool, server_list_serverDef_route_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    njt_str_t *dst = handle_escape_on_write(pool, out);
    cur = njt_sprintf(cur, "\"%V\"", dst);
    buf->len = cur - buf->data;
}

static void to_oneline_json_server_list_serverDef_backup(njt_pool_t *pool, server_list_serverDef_backup_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    if (*out) {
        njt_sprintf(cur, "true");
        buf->len += 4;
    } else {
        njt_sprintf(cur, "false");
        buf->len += 5;
    }
}

static void to_oneline_json_server_list_serverDef_down(njt_pool_t *pool, server_list_serverDef_down_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    if (*out) {
        njt_sprintf(cur, "true");
        buf->len += 4;
    } else {
        njt_sprintf(cur, "false");
        buf->len += 5;
    }
}

static void to_oneline_json_server_list_serverDef_parent(njt_pool_t *pool, server_list_serverDef_parent_t *out, njt_str_t* buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    cur = njt_sprintf(cur, "%L", *out);
    buf->len = cur - buf->data;
}

static void to_oneline_json_server_list_serverDef_host(njt_pool_t *pool, server_list_serverDef_host_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    njt_str_t *dst = handle_escape_on_write(pool, out);
    cur = njt_sprintf(cur, "\"%V\"", dst);
    buf->len = cur - buf->data;
}

static void to_oneline_json_server_list_serverDef_drain(njt_pool_t *pool, server_list_serverDef_drain_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    if (*out) {
        njt_sprintf(cur, "true");
        buf->len += 4;
    } else {
        njt_sprintf(cur, "false");
        buf->len += 5;
    }
}

 void to_oneline_json_server_list_serverDef(njt_pool_t *pool, server_list_serverDef_t *out, njt_str_t* buf, njt_int_t flags) {
    njt_int_t omit;
    u_char* cur = buf->data + buf->len;
    if (out == NULL) {
        cur = njt_sprintf(cur, "null");
        buf->len += 4;
        return;
    }
    cur = njt_sprintf(cur, "{");
    buf->len ++;
    omit = 0;
    omit = out->is_id_set ? 0 : 1;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"id\":");
        buf->len = cur - buf->data;
        to_oneline_json_server_list_serverDef_id(pool, (&out->id), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_server_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->server.data) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"server\":");
        buf->len = cur - buf->data;
        to_oneline_json_server_list_serverDef_server(pool, (&out->server), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_name_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->name.data) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"name\":");
        buf->len = cur - buf->data;
        to_oneline_json_server_list_serverDef_name(pool, (&out->name), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_weight_set ? 0 : 1;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"weight\":");
        buf->len = cur - buf->data;
        to_oneline_json_server_list_serverDef_weight(pool, (&out->weight), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_max_conns_set ? 0 : 1;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"max_conns\":");
        buf->len = cur - buf->data;
        to_oneline_json_server_list_serverDef_max_conns(pool, (&out->max_conns), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_max_fails_set ? 0 : 1;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"max_fails\":");
        buf->len = cur - buf->data;
        to_oneline_json_server_list_serverDef_max_fails(pool, (&out->max_fails), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_fail_timeout_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->fail_timeout.data) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"fail_timeout\":");
        buf->len = cur - buf->data;
        to_oneline_json_server_list_serverDef_fail_timeout(pool, (&out->fail_timeout), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_slow_start_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->slow_start.data) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"slow_start\":");
        buf->len = cur - buf->data;
        to_oneline_json_server_list_serverDef_slow_start(pool, (&out->slow_start), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_route_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->route.data) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"route\":");
        buf->len = cur - buf->data;
        to_oneline_json_server_list_serverDef_route(pool, (&out->route), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_backup_set ? 0 : 1;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"backup\":");
        buf->len = cur - buf->data;
        to_oneline_json_server_list_serverDef_backup(pool, (&out->backup), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_down_set ? 0 : 1;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"down\":");
        buf->len = cur - buf->data;
        to_oneline_json_server_list_serverDef_down(pool, (&out->down), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_parent_set ? 0 : 1;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"parent\":");
        buf->len = cur - buf->data;
        to_oneline_json_server_list_serverDef_parent(pool, (&out->parent), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_host_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->host.data) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"host\":");
        buf->len = cur - buf->data;
        to_oneline_json_server_list_serverDef_host(pool, (&out->host), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_drain_set ? 0 : 1;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"drain\":");
        buf->len = cur - buf->data;
        to_oneline_json_server_list_serverDef_drain(pool, (&out->drain), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    cur--;
    if (cur[0] == ',') {
        buf->len --;
    } else {
        cur ++;
    }
    cur = njt_sprintf(cur, "}");
    buf->len ++;
}

static bool parse_server_list(njt_pool_t *pool, parse_state_t *parse_state, server_list_t *out, js2c_parse_error_t *err_ret) {
    int i;
    js2c_check_type(JSMN_ARRAY);
    const int n = parse_state->tokens[parse_state->current_token].size;
    parse_state->current_token += 1;
    for (i = 0; i < n; ++i) {
        ((server_list_item_t**)out->elts)[i] = njt_pcalloc(pool, sizeof(server_list_serverDef_t));
        memset(((server_list_item_t**)out->elts)[i], 0, sizeof(server_list_item_t));
        if (parse_server_list_serverDef(pool, parse_state, ((server_list_item_t**)out->elts)[i], err_ret)) {
            return true;
        }
        out->nelts ++;
    }
    return false;
}


static void get_json_length_server_list_item(njt_pool_t *pool, server_list_item_t *out, size_t *length, njt_int_t flags) {
    get_json_length_server_list_serverDef(pool, out, length, flags);
}

 void get_json_length_server_list(njt_pool_t *pool, server_list_t *out, size_t *length, njt_int_t flags) {
    njt_uint_t i;
    njt_uint_t omit;
    njt_int_t count = 0;
    if (out == NULL) {
        *length += 2; // "[]"
        return;
    }
    *length += 2; // "[]"
    for (i = 0; i < out->nelts; ++i) {
        omit = 0;
        omit = ((flags & OMIT_NULL_OBJ) && ((server_list_item_t**)out->elts)[i] == NULL) ? 1 : 0;
        if (omit == 0) {
            get_json_length_server_list_item(pool, ((server_list_item_t**)out->elts)[i], length, flags);
            *length += 1; // ","
            count++; // ","
        }
    }
    if (count != 0) {
        *length -= 1; // "\b"
    }
}
server_list_item_t* get_server_list_item(server_list_t *out, size_t idx) {
    return ((server_list_item_t**)out->elts)[idx];

}
int add_item_server_list(server_list_t *src, server_list_item_t* item) {
    void *new = njt_array_push(src);
    if (new == NULL) {
        return NJT_ERROR;
    }
    njt_memcpy(new, &item, src->size);
    return NJT_OK;
}

server_list_t* create_server_list(njt_pool_t *pool, size_t nelts) {
    return njt_array_create(pool, nelts, sizeof(server_list_item_t*));
}
// to_oneline_json_REF

static void to_oneline_json_server_list_item(njt_pool_t *pool, server_list_item_t *out, njt_str_t* buf, njt_int_t flags) {
    to_oneline_json_server_list_serverDef(pool, out, buf, flags);
}

void to_oneline_json_server_list(njt_pool_t *pool, server_list_t *out, njt_str_t* buf, njt_int_t flags) {
    njt_int_t omit;
    u_char *cur = buf->data + buf->len;
    njt_uint_t i;
    if (out == NULL || out->nelts == 0) {
        cur = njt_sprintf(cur, "[]");
        buf->len += 2;
        return;
    }
    cur = njt_sprintf(cur,  "[");
    buf->len ++;
    for (i = 0; i < out->nelts; ++i) {
        omit = 0;
        omit = ((flags & OMIT_NULL_OBJ) && ((server_list_item_t**)out->elts)[i] == NULL) ? 1 : 0;
        if (omit == 0) {
            to_oneline_json_server_list_item(pool, ((server_list_item_t**)out->elts)[i], buf, flags);
            cur = buf->data + buf->len;
            cur = njt_sprintf(cur, ",");
            buf->len ++;
        }
    }
    cur--;
    if (cur[0] == ',') {
        buf->len --;
    } else {
        cur ++;
    }
    cur = njt_sprintf(cur,  "]");
    buf->len ++;
}
server_list_t* json_parse_server_list(njt_pool_t *pool, const njt_str_t *json_string, js2c_parse_error_t *err_ret) {
    server_list_t* out;
    parse_state_t parse_state_var;
    parse_state_t *parse_state = &parse_state_var;
    uint64_t max_token_number = 1024;
    jsmntok_t *token_buffer;
    int parse_result;
    for ( ; /* parse unsuccessful */; ) {
        token_buffer = njt_pcalloc(pool, sizeof(jsmntok_t)*max_token_number);
        parse_result = builtin_parse_json_string(pool, parse_state, token_buffer, max_token_number, (char *)json_string->data, json_string->len, err_ret);
        if (parse_result == JSMN_ERROR_INVAL) {
            LOG_ERROR_JSON_PARSE(INVALID_JSON_CHAR_ERR, "", -1, "%s", "Invalid character inside JSON string");
            return NULL;
        }
        if (parse_result == JSMN_ERROR_PART) {
            LOG_ERROR_JSON_PARSE(PARTIAL_JSON_ERR, "", -1, "%s", "The string is not a full JSON packet, more bytes expected");
            return NULL;
        }
        if (parse_result == JSMN_ERROR_NOMEM) {
            max_token_number += max_token_number;
            continue;
        }
        if (parse_result == 0) {
            LOG_ERROR_JSON_PARSE(NULL_JSON_ERR, "", 0, "String did not contain %s JSON tokens", "any");
            return NULL;
        }
        break; // parse success
    }
    out = njt_array_create(pool, parse_state->tokens[parse_state->current_token].size ,sizeof(server_list_item_t*));;
    memset(out, 0, sizeof(server_list_t));
    if (parse_server_list(pool, parse_state, out, err_ret)) {
        return NULL;
    }
    return out;
}

njt_str_t* to_json_server_list(njt_pool_t *pool, server_list_t* out, njt_int_t flags) {
    njt_str_t *json_str;
    json_str = njt_pcalloc(pool, sizeof(njt_str_t));
    size_t str_len = 0;
    get_json_length_server_list(pool, out, &str_len, flags);
    json_str->data = (u_char*)njt_pcalloc(pool, str_len + 1);
    json_str->len = 0;
    to_oneline_json_server_list(pool, out, json_str, flags);
    return json_str;
}
