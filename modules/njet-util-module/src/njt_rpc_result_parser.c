

/* This file was generated by JSON Schema to C.
 * Any changes made to it will be lost on regeneration. 

 * Copyright (C) 2021-2023  TMLake(Beijing) Technology Co., Ltd.
 */

#include "njt_rpc_result_parser.h"
#include "njt_core.h"
#include "js2c_njet_builtins.h"
/* ========================== Generated parsers ========================== */


static bool parse_rpc_result_data(njt_pool_t *pool, parse_state_t *parse_state, rpc_result_data_t *out, js2c_parse_error_t *err_ret) {
    int i;
    js2c_check_type(JSMN_ARRAY);
    const int n = parse_state->tokens[parse_state->current_token].size;
    parse_state->current_token += 1;
    for (i = 0; i < n; ++i) {
        int token_size =  CURRENT_STRING_LENGTH(parse_state) ;
        ((&((rpc_result_data_item_t*)out->elts)[i]))->data = (u_char*)njt_palloc(pool, (size_t)(token_size + 1));
        js2c_malloc_check(((&((rpc_result_data_item_t*)out->elts)[i]))->data);
        ((&((rpc_result_data_item_t*)out->elts)[i]))->len = token_size;
        if (builtin_parse_string(pool, parse_state, (&((rpc_result_data_item_t*)out->elts)[i]), 0, ((&((rpc_result_data_item_t*)out->elts)[i]))->len, err_ret)) {
            return true;
        }
        out->nelts ++;
    }
    return false;
}


static bool parse_rpc_result(njt_pool_t *pool, parse_state_t *parse_state, rpc_result_t *out, js2c_parse_error_t *err_ret) {
    njt_uint_t i;

    js2c_check_type(JSMN_OBJECT);
    const int object_start_token = parse_state->current_token;
    const uint64_t n = parse_state->tokens[parse_state->current_token].size;
    parse_state->current_token += 1;
    for (i = 0; i < n; ++i) {
        js2c_key_children_check_for_obj();
        if (current_string_is(parse_state, "data")) {
            js2c_check_field_set(out->is_data_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "data";
            js2c_null_check();
            out->data = njt_array_create(pool, parse_state->tokens[parse_state->current_token].size ,sizeof(njt_str_t));
            js2c_malloc_check(out->data);

            if (parse_rpc_result_data(pool, parse_state, (out->data), err_ret)) {
                return true;
            }
            out->is_data_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "code")) {
            js2c_check_field_set(out->is_code_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "code";
            int64_t int_parse_tmp;
            if (builtin_parse_signed(pool, parse_state, true, false, 10, &int_parse_tmp, err_ret)) {
                return true;
            }
            *(&out->code) = int_parse_tmp;
            out->is_code_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "msg")) {
            js2c_check_field_set(out->is_msg_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "msg";
            int token_size =  CURRENT_STRING_LENGTH(parse_state) ;
            ((&out->msg))->data = (u_char*)njt_palloc(pool, (size_t)(token_size + 1));
            js2c_malloc_check(((&out->msg))->data);
            ((&out->msg))->len = token_size;
            if (builtin_parse_string(pool, parse_state, (&out->msg), 0, ((&out->msg))->len, err_ret)) {
                return true;
            }
            out->is_msg_set = 1;
            parse_state->current_key = saved_key;
        } else {
            LOG_ERROR_JSON_PARSE(UNKNOWN_FIELD_ERR, parse_state->current_key, CURRENT_TOKEN(parse_state).start, "Unknown field in '%s': %.*s", parse_state->current_key, CURRENT_STRING_FOR_ERROR(parse_state));
            return true;
        }
    }
    const int saved_current_token = parse_state->current_token;
    parse_state->current_token = object_start_token;
    // set default
    if (!out->is_data_set) {
        out->data = njt_palloc(pool, sizeof(njt_array_t));
        memset(out->data, 0, sizeof(njt_array_t));
    }
    parse_state->current_token = saved_current_token;
    return false;
}


static void get_json_length_rpc_result_data_item(njt_pool_t *pool, rpc_result_data_item_t *out, size_t *length, njt_int_t flags) {
    njt_str_t *dst = handle_escape_on_write(pool, out);
    *length += dst->len + 2; //  "str" 
}

static void get_json_length_rpc_result_data(njt_pool_t *pool, rpc_result_data_t *out, size_t *length, njt_int_t flags) {
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
        omit = ((flags & OMIT_NULL_STR) && (&((rpc_result_data_item_t*)out->elts)[i])->data == NULL) ? 1 : 0;
        if (omit == 0) {
            get_json_length_rpc_result_data_item(pool, (&((rpc_result_data_item_t*)out->elts)[i]), length, flags);
            *length += 1; // ","
            count++; // ","
        }
    }
    if (count != 0) {
        *length -= 1; // "\b"
    }
}

static void get_json_length_rpc_result_code(njt_pool_t *pool, rpc_result_code_t *out, size_t *length, njt_int_t flags) {
    u_char str[24];
    u_char *cur;
    cur = njt_sprintf(str, "%L", *out);
    *length += cur - str;
}

static void get_json_length_rpc_result_msg(njt_pool_t *pool, rpc_result_msg_t *out, size_t *length, njt_int_t flags) {
    njt_str_t *dst = handle_escape_on_write(pool, out);
    *length += dst->len + 2; //  "str" 
}

static void get_json_length_rpc_result(njt_pool_t *pool, rpc_result_t *out, size_t *length, njt_int_t flags) {
    if (out == NULL) {
        *length += 4; // null
        return;
    }
    *length += 1;
    njt_int_t omit;
    njt_int_t count = 0;
    omit = 0;
    omit = out->is_data_set ? 0 : 1;
    omit = (flags & OMIT_NULL_ARRAY) && (out->data) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (4 + 3); // "data": 
        get_json_length_rpc_result_data(pool, (out->data), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_code_set ? 0 : 1;
    if (omit == 0) {
        *length += (4 + 3); // "code": 
        get_json_length_rpc_result_code(pool, (&out->code), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_msg_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->msg.data) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (3 + 3); // "msg": 
        get_json_length_rpc_result_msg(pool, (&out->msg), length, flags);
        *length += 1; // ","
        count++;
    }
    if (count != 0) {
        *length -= 1; // "\b"
    }
    *length += 1;
}
rpc_result_data_item_t* get_rpc_result_data_item(rpc_result_data_t *out, size_t idx) {
    return &((rpc_result_data_item_t*)out->elts)[idx];

}

rpc_result_data_t* get_rpc_result_data(rpc_result_t *out) {
    return out->data;
}

rpc_result_code_t get_rpc_result_code(rpc_result_t *out) {
    return out->code;
}

rpc_result_msg_t* get_rpc_result_msg(rpc_result_t *out) {
    return &out->msg;
}
int add_item_rpc_result_data(rpc_result_data_t *src, rpc_result_data_item_t* item) {
    void *new = njt_array_push(src);
    if (new == NULL) {
        return NJT_ERROR;
    }
    njt_memcpy(new, item, src->size);
    return NJT_OK;
}

rpc_result_data_t* create_rpc_result_data(njt_pool_t *pool, size_t nelts) {
    return njt_array_create(pool, nelts, sizeof(rpc_result_data_item_t));
}
void set_rpc_result_data(rpc_result_t* obj, rpc_result_data_t* field) {
    obj->data = field;
    obj->is_data_set = 1;
}
void set_rpc_result_code(rpc_result_t* obj, rpc_result_code_t field) {
    obj->code = field;
    obj->is_code_set = 1;
}
void set_rpc_result_msg(rpc_result_t* obj, rpc_result_msg_t* field) {
    njt_memcpy(&obj->msg, field, sizeof(njt_str_t));
    obj->is_msg_set = 1;
}
rpc_result_t* create_rpc_result(njt_pool_t *pool) {
    rpc_result_t* out = njt_palloc(pool, sizeof(rpc_result_t));
    memset(out, 0, sizeof(rpc_result_t));
    return out;
}

static void to_oneline_json_rpc_result_data_item(njt_pool_t *pool, rpc_result_data_item_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    njt_str_t *dst = handle_escape_on_write(pool, out);
    cur = njt_sprintf(cur, "\"%V\"", dst);
    buf->len = cur - buf->data;
}

static void to_oneline_json_rpc_result_data(njt_pool_t *pool, rpc_result_data_t *out, njt_str_t* buf, njt_int_t flags) {
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
        omit = ((flags & OMIT_NULL_STR) && (&((rpc_result_data_item_t*)out->elts)[i])->data == NULL) ? 1 : 0;
        if (omit == 0) {
            to_oneline_json_rpc_result_data_item(pool, (&((rpc_result_data_item_t*)out->elts)[i]), buf, flags);
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

static void to_oneline_json_rpc_result_code(njt_pool_t *pool, rpc_result_code_t *out, njt_str_t* buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    cur = njt_sprintf(cur, "%L", *out);
    buf->len = cur - buf->data;
}

static void to_oneline_json_rpc_result_msg(njt_pool_t *pool, rpc_result_msg_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    njt_str_t *dst = handle_escape_on_write(pool, out);
    cur = njt_sprintf(cur, "\"%V\"", dst);
    buf->len = cur - buf->data;
}

static void to_oneline_json_rpc_result(njt_pool_t *pool, rpc_result_t *out, njt_str_t* buf, njt_int_t flags) {
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
    omit = out->is_data_set ? 0 : 1;
    omit = (flags & OMIT_NULL_ARRAY) && (out->data) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"data\":");
        buf->len = cur - buf->data;
        to_oneline_json_rpc_result_data(pool, (out->data), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_code_set ? 0 : 1;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"code\":");
        buf->len = cur - buf->data;
        to_oneline_json_rpc_result_code(pool, (&out->code), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_msg_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->msg.data) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"msg\":");
        buf->len = cur - buf->data;
        to_oneline_json_rpc_result_msg(pool, (&out->msg), buf, flags);
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
rpc_result_t* json_parse_rpc_result(njt_pool_t *pool, const njt_str_t *json_string, js2c_parse_error_t *err_ret) {
    rpc_result_t* out;
    parse_state_t parse_state_var;
    parse_state_t *parse_state = &parse_state_var;
    uint64_t max_token_number = 1024;
    jsmntok_t *token_buffer;
    int parse_result;
    for ( ; /* parse unsuccessful */; ) {
        token_buffer = njt_palloc(pool, sizeof(jsmntok_t)*max_token_number);
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
    out = njt_palloc(pool, sizeof(rpc_result_t));;
    if (parse_rpc_result(pool, parse_state, out, err_ret)) {
        return NULL;
    }
    return out;
}

njt_str_t* to_json_rpc_result(njt_pool_t *pool, rpc_result_t* out, njt_int_t flags) {
    njt_str_t *json_str;
    json_str = njt_palloc(pool, sizeof(njt_str_t));
    size_t str_len = 0;
    get_json_length_rpc_result(pool, out, &str_len, flags);
    json_str->data = (u_char*)njt_palloc(pool, str_len + 1);
    json_str->len = 0;
    to_oneline_json_rpc_result(pool, out, json_str, flags);
    return json_str;
}
