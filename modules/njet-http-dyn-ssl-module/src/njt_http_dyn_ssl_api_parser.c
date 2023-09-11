

/* This file was generated by JSON Schema to C.
 * Any changes made to it will be lost on regeneration. 

 * Copyright (C) 2021-2023  TMLake(Beijing) Technology Co., Ltd.
 */

#include "njt_http_dyn_ssl_api_parser.h"
#include "njt_core.h"
#include "js2c_njet_builtins.h"
/* ========================== Generated parsers ========================== */


static bool parse_dyn_ssl_api_listens(njt_pool_t *pool, parse_state_t *parse_state, dyn_ssl_api_listens_t *out, js2c_parse_error_t *err_ret) {
    int i;
    js2c_check_type(JSMN_ARRAY);
    const int n = parse_state->tokens[parse_state->current_token].size;
    parse_state->current_token += 1;
    for (i = 0; i < n; ++i) {
        int token_size =  CURRENT_STRING_LENGTH(parse_state) ;
        ((&((dyn_ssl_api_listens_item_t*)out->elts)[i]))->data = (u_char*)njt_pcalloc(pool, (size_t)(token_size + 1));
        js2c_malloc_check(((&((dyn_ssl_api_listens_item_t*)out->elts)[i]))->data);
        ((&((dyn_ssl_api_listens_item_t*)out->elts)[i]))->len = token_size;
        if (builtin_parse_string(pool, parse_state, (&((dyn_ssl_api_listens_item_t*)out->elts)[i]), 1, ((&((dyn_ssl_api_listens_item_t*)out->elts)[i]))->len, err_ret)) {
            return true;
        }
        out->nelts ++;
    }
    return false;
}


static bool parse_dyn_ssl_api_serverNames(njt_pool_t *pool, parse_state_t *parse_state, dyn_ssl_api_serverNames_t *out, js2c_parse_error_t *err_ret) {
    int i;
    js2c_check_type(JSMN_ARRAY);
    const int n = parse_state->tokens[parse_state->current_token].size;
    parse_state->current_token += 1;
    for (i = 0; i < n; ++i) {
        int token_size =  CURRENT_STRING_LENGTH(parse_state) ;
        ((&((dyn_ssl_api_serverNames_item_t*)out->elts)[i]))->data = (u_char*)njt_pcalloc(pool, (size_t)(token_size + 1));
        js2c_malloc_check(((&((dyn_ssl_api_serverNames_item_t*)out->elts)[i]))->data);
        ((&((dyn_ssl_api_serverNames_item_t*)out->elts)[i]))->len = token_size;
        if (builtin_parse_string(pool, parse_state, (&((dyn_ssl_api_serverNames_item_t*)out->elts)[i]), 0, ((&((dyn_ssl_api_serverNames_item_t*)out->elts)[i]))->len, err_ret)) {
            return true;
        }
        out->nelts ++;
    }
    return false;
}


static bool parse_dyn_ssl_api_type(njt_pool_t *pool, parse_state_t *parse_state, dyn_ssl_api_type_t *out, js2c_parse_error_t *err_ret) {
    js2c_check_type(JSMN_STRING);
    if (current_string_is(parse_state, "add")) {
        *out = DYN_SSL_API_TYPE_ADD;
    } else if (current_string_is(parse_state, "del")) {
        *out = DYN_SSL_API_TYPE_DEL;
    } else {
        LOG_ERROR_JSON_PARSE(UNKNOWN_ENUM_VALUE_ERR, parse_state->current_key, CURRENT_TOKEN(parse_state).start, "Unknown enum value in '%s': %.*s", parse_state->current_key, CURRENT_STRING_FOR_ERROR(parse_state));
        return true;
    }
    parse_state->current_token += 1;
    return false;
}


static bool parse_dyn_ssl_api_cert_info_cert_type(njt_pool_t *pool, parse_state_t *parse_state, dyn_ssl_api_cert_info_cert_type_t *out, js2c_parse_error_t *err_ret) {
    js2c_check_type(JSMN_STRING);
    if (current_string_is(parse_state, "rsa")) {
        *out = DYN_SSL_API_CERT_INFO_CERT_TYPE_RSA;
    } else if (current_string_is(parse_state, "ntls")) {
        *out = DYN_SSL_API_CERT_INFO_CERT_TYPE_NTLS;
    } else if (current_string_is(parse_state, "ecc")) {
        *out = DYN_SSL_API_CERT_INFO_CERT_TYPE_ECC;
    } else {
        LOG_ERROR_JSON_PARSE(UNKNOWN_ENUM_VALUE_ERR, parse_state->current_key, CURRENT_TOKEN(parse_state).start, "Unknown enum value in '%s': %.*s", parse_state->current_key, CURRENT_STRING_FOR_ERROR(parse_state));
        return true;
    }
    parse_state->current_token += 1;
    return false;
}


static bool parse_dyn_ssl_api_cert_info(njt_pool_t *pool, parse_state_t *parse_state, dyn_ssl_api_cert_info_t *out, js2c_parse_error_t *err_ret) {
    njt_uint_t i;

    js2c_check_type(JSMN_OBJECT);
    const int object_start_token = parse_state->current_token;
    const uint64_t n = parse_state->tokens[parse_state->current_token].size;
    parse_state->current_token += 1;
    for (i = 0; i < n; ++i) {
        js2c_key_children_check_for_obj();
        if (current_string_is(parse_state, "cert_type")) {
            js2c_check_field_set(out->is_cert_type_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "cert_type";
            if (parse_dyn_ssl_api_cert_info_cert_type(pool, parse_state, (&out->cert_type), err_ret)) {
                return true;
            }
            out->is_cert_type_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "certificate")) {
            js2c_check_field_set(out->is_certificate_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "certificate";
            int token_size =  CURRENT_STRING_LENGTH(parse_state) ;
            ((&out->certificate))->data = (u_char*)njt_pcalloc(pool, (size_t)(token_size + 1));
            js2c_malloc_check(((&out->certificate))->data);
            ((&out->certificate))->len = token_size;
            if (builtin_parse_string(pool, parse_state, (&out->certificate), 0, ((&out->certificate))->len, err_ret)) {
                return true;
            }
            out->is_certificate_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "certificateKey")) {
            js2c_check_field_set(out->is_certificateKey_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "certificateKey";
            int token_size =  CURRENT_STRING_LENGTH(parse_state) ;
            ((&out->certificateKey))->data = (u_char*)njt_pcalloc(pool, (size_t)(token_size + 1));
            js2c_malloc_check(((&out->certificateKey))->data);
            ((&out->certificateKey))->len = token_size;
            if (builtin_parse_string(pool, parse_state, (&out->certificateKey), 0, ((&out->certificateKey))->len, err_ret)) {
                return true;
            }
            out->is_certificateKey_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "certificateEnc")) {
            js2c_check_field_set(out->is_certificateEnc_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "certificateEnc";
            js2c_null_check();
            int token_size =  CURRENT_STRING_LENGTH(parse_state) ;
            ((&out->certificateEnc))->data = (u_char*)njt_pcalloc(pool, (size_t)(token_size + 1));
            js2c_malloc_check(((&out->certificateEnc))->data);
            ((&out->certificateEnc))->len = token_size;
            if (builtin_parse_string(pool, parse_state, (&out->certificateEnc), 0, ((&out->certificateEnc))->len, err_ret)) {
                return true;
            }
            out->is_certificateEnc_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "certificateKeyEnc")) {
            js2c_check_field_set(out->is_certificateKeyEnc_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "certificateKeyEnc";
            js2c_null_check();
            int token_size =  CURRENT_STRING_LENGTH(parse_state) ;
            ((&out->certificateKeyEnc))->data = (u_char*)njt_pcalloc(pool, (size_t)(token_size + 1));
            js2c_malloc_check(((&out->certificateKeyEnc))->data);
            ((&out->certificateKeyEnc))->len = token_size;
            if (builtin_parse_string(pool, parse_state, (&out->certificateKeyEnc), 0, ((&out->certificateKeyEnc))->len, err_ret)) {
                return true;
            }
            out->is_certificateKeyEnc_set = 1;
            parse_state->current_key = saved_key;
        } else {
            LOG_ERROR_JSON_PARSE(UNKNOWN_FIELD_ERR, parse_state->current_key, CURRENT_TOKEN(parse_state).start, "Unknown field in '%s': %.*s", parse_state->current_key, CURRENT_STRING_FOR_ERROR(parse_state));
            return true;
        }
    }
    const int saved_current_token = parse_state->current_token;
    parse_state->current_token = object_start_token;
    if (!out->is_certificate_set) {
        LOG_ERROR_JSON_PARSE(MISSING_REQUIRED_FIELD_ERR, parse_state->current_key, CURRENT_TOKEN(parse_state).start, "Missing required field in '%s': certificate", parse_state->current_key);
        return true;
    }
    if (!out->is_certificateKey_set) {
        LOG_ERROR_JSON_PARSE(MISSING_REQUIRED_FIELD_ERR, parse_state->current_key, CURRENT_TOKEN(parse_state).start, "Missing required field in '%s': certificateKey", parse_state->current_key);
        return true;
    }
    // set default
    if (!out->is_certificateEnc_set) {
        size_t token_size = strlen("");
        (out->certificateEnc).data = (u_char*)njt_pcalloc(pool, token_size + 1);
        js2c_malloc_check((out->certificateEnc).data);
        (out->certificateEnc).len = token_size;
        if (out->certificateEnc.len == 0) {
            (out->certificateEnc).data[0] = 0;
        }
        if (token_size > 0) {
            njt_memcpy(out->certificateEnc.data, "", token_size);
        }
    }
    // set default
    if (!out->is_certificateKeyEnc_set) {
        size_t token_size = strlen("");
        (out->certificateKeyEnc).data = (u_char*)njt_pcalloc(pool, token_size + 1);
        js2c_malloc_check((out->certificateKeyEnc).data);
        (out->certificateKeyEnc).len = token_size;
        if (out->certificateKeyEnc.len == 0) {
            (out->certificateKeyEnc).data[0] = 0;
        }
        if (token_size > 0) {
            njt_memcpy(out->certificateKeyEnc.data, "", token_size);
        }
    }
    parse_state->current_token = saved_current_token;
    return false;
}


static bool parse_dyn_ssl_api(njt_pool_t *pool, parse_state_t *parse_state, dyn_ssl_api_t *out, js2c_parse_error_t *err_ret) {
    njt_uint_t i;

    js2c_check_type(JSMN_OBJECT);
    const int object_start_token = parse_state->current_token;
    const uint64_t n = parse_state->tokens[parse_state->current_token].size;
    parse_state->current_token += 1;
    for (i = 0; i < n; ++i) {
        js2c_key_children_check_for_obj();
        if (current_string_is(parse_state, "listens")) {
            js2c_check_field_set(out->is_listens_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "listens";
            out->listens = njt_array_create(pool, parse_state->tokens[parse_state->current_token].size ,sizeof(njt_str_t));
            js2c_malloc_check(out->listens);

            if (parse_dyn_ssl_api_listens(pool, parse_state, (out->listens), err_ret)) {
                return true;
            }
            out->is_listens_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "serverNames")) {
            js2c_check_field_set(out->is_serverNames_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "serverNames";
            out->serverNames = njt_array_create(pool, parse_state->tokens[parse_state->current_token].size ,sizeof(njt_str_t));
            js2c_malloc_check(out->serverNames);

            if (parse_dyn_ssl_api_serverNames(pool, parse_state, (out->serverNames), err_ret)) {
                return true;
            }
            out->is_serverNames_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "type")) {
            js2c_check_field_set(out->is_type_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "type";
            if (parse_dyn_ssl_api_type(pool, parse_state, (&out->type), err_ret)) {
                return true;
            }
            out->is_type_set = 1;
            parse_state->current_key = saved_key;
        } else if (current_string_is(parse_state, "cert_info")) {
            js2c_check_field_set(out->is_cert_info_set);
            parse_state->current_token += 1;
            const char* saved_key = parse_state->current_key;
            parse_state->current_key = "cert_info";
            out->cert_info = njt_pcalloc(pool, sizeof(dyn_ssl_api_cert_info_t));
            js2c_malloc_check(out->cert_info);
            memset(out->cert_info, 0, sizeof(dyn_ssl_api_cert_info_t));

            if (parse_dyn_ssl_api_cert_info(pool, parse_state, (out->cert_info), err_ret)) {
                return true;
            }
            out->is_cert_info_set = 1;
            parse_state->current_key = saved_key;
        } else {
            LOG_ERROR_JSON_PARSE(UNKNOWN_FIELD_ERR, parse_state->current_key, CURRENT_TOKEN(parse_state).start, "Unknown field in '%s': %.*s", parse_state->current_key, CURRENT_STRING_FOR_ERROR(parse_state));
            return true;
        }
    }
    const int saved_current_token = parse_state->current_token;
    parse_state->current_token = object_start_token;
    if (!out->is_listens_set) {
        LOG_ERROR_JSON_PARSE(MISSING_REQUIRED_FIELD_ERR, parse_state->current_key, CURRENT_TOKEN(parse_state).start, "Missing required field in '%s': listens", parse_state->current_key);
        return true;
    }
    if (!out->is_serverNames_set) {
        LOG_ERROR_JSON_PARSE(MISSING_REQUIRED_FIELD_ERR, parse_state->current_key, CURRENT_TOKEN(parse_state).start, "Missing required field in '%s': serverNames", parse_state->current_key);
        return true;
    }
    if (!out->is_type_set) {
        LOG_ERROR_JSON_PARSE(MISSING_REQUIRED_FIELD_ERR, parse_state->current_key, CURRENT_TOKEN(parse_state).start, "Missing required field in '%s': type", parse_state->current_key);
        return true;
    }
    if (!out->is_cert_info_set) {
        LOG_ERROR_JSON_PARSE(MISSING_REQUIRED_FIELD_ERR, parse_state->current_key, CURRENT_TOKEN(parse_state).start, "Missing required field in '%s': cert_info", parse_state->current_key);
        return true;
    }
    parse_state->current_token = saved_current_token;
    return false;
}


static void get_json_length_dyn_ssl_api_listens_item(njt_pool_t *pool, dyn_ssl_api_listens_item_t *out, size_t *length, njt_int_t flags) {
    njt_str_t *dst = handle_escape_on_write(pool, out);
    *length += dst->len + 2; //  "str" 
}

static void get_json_length_dyn_ssl_api_listens(njt_pool_t *pool, dyn_ssl_api_listens_t *out, size_t *length, njt_int_t flags) {
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
        omit = ((flags & OMIT_NULL_STR) && (&((dyn_ssl_api_listens_item_t*)out->elts)[i])->data == NULL) ? 1 : 0;
        if (omit == 0) {
            get_json_length_dyn_ssl_api_listens_item(pool, (&((dyn_ssl_api_listens_item_t*)out->elts)[i]), length, flags);
            *length += 1; // ","
            count++; // ","
        }
    }
    if (count != 0) {
        *length -= 1; // "\b"
    }
}

static void get_json_length_dyn_ssl_api_serverNames_item(njt_pool_t *pool, dyn_ssl_api_serverNames_item_t *out, size_t *length, njt_int_t flags) {
    njt_str_t *dst = handle_escape_on_write(pool, out);
    *length += dst->len + 2; //  "str" 
}

static void get_json_length_dyn_ssl_api_serverNames(njt_pool_t *pool, dyn_ssl_api_serverNames_t *out, size_t *length, njt_int_t flags) {
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
        omit = ((flags & OMIT_NULL_STR) && (&((dyn_ssl_api_serverNames_item_t*)out->elts)[i])->data == NULL) ? 1 : 0;
        if (omit == 0) {
            get_json_length_dyn_ssl_api_serverNames_item(pool, (&((dyn_ssl_api_serverNames_item_t*)out->elts)[i]), length, flags);
            *length += 1; // ","
            count++; // ","
        }
    }
    if (count != 0) {
        *length -= 1; // "\b"
    }
}
// BEGIN GET_JSON_LENGTH ENUM

static void get_json_length_dyn_ssl_api_type(njt_pool_t *pool, dyn_ssl_api_type_t *out, size_t *length, njt_int_t flags) {
    if (*out == DYN_SSL_API_TYPE_ADD) {
        // "add"
        *length += 3 + 2;
        return;
    }
    if (*out == DYN_SSL_API_TYPE_DEL) {
        // "del"
        *length += 3 + 2;
        return;
    }
}
// BEGIN GET_JSON_LENGTH ENUM

static void get_json_length_dyn_ssl_api_cert_info_cert_type(njt_pool_t *pool, dyn_ssl_api_cert_info_cert_type_t *out, size_t *length, njt_int_t flags) {
    if (*out == DYN_SSL_API_CERT_INFO_CERT_TYPE_RSA) {
        // "rsa"
        *length += 3 + 2;
        return;
    }
    if (*out == DYN_SSL_API_CERT_INFO_CERT_TYPE_NTLS) {
        // "ntls"
        *length += 4 + 2;
        return;
    }
    if (*out == DYN_SSL_API_CERT_INFO_CERT_TYPE_ECC) {
        // "ecc"
        *length += 3 + 2;
        return;
    }
}

static void get_json_length_dyn_ssl_api_cert_info_certificate(njt_pool_t *pool, dyn_ssl_api_cert_info_certificate_t *out, size_t *length, njt_int_t flags) {
    njt_str_t *dst = handle_escape_on_write(pool, out);
    *length += dst->len + 2; //  "str" 
}

static void get_json_length_dyn_ssl_api_cert_info_certificateKey(njt_pool_t *pool, dyn_ssl_api_cert_info_certificateKey_t *out, size_t *length, njt_int_t flags) {
    njt_str_t *dst = handle_escape_on_write(pool, out);
    *length += dst->len + 2; //  "str" 
}

static void get_json_length_dyn_ssl_api_cert_info_certificateEnc(njt_pool_t *pool, dyn_ssl_api_cert_info_certificateEnc_t *out, size_t *length, njt_int_t flags) {
    njt_str_t *dst = handle_escape_on_write(pool, out);
    *length += dst->len + 2; //  "str" 
}

static void get_json_length_dyn_ssl_api_cert_info_certificateKeyEnc(njt_pool_t *pool, dyn_ssl_api_cert_info_certificateKeyEnc_t *out, size_t *length, njt_int_t flags) {
    njt_str_t *dst = handle_escape_on_write(pool, out);
    *length += dst->len + 2; //  "str" 
}

static void get_json_length_dyn_ssl_api_cert_info(njt_pool_t *pool, dyn_ssl_api_cert_info_t *out, size_t *length, njt_int_t flags) {
    if (out == NULL) {
        *length += 4; // null
        return;
    }
    *length += 1;
    njt_int_t omit;
    njt_int_t count = 0;
    omit = 0;
    omit = out->is_cert_type_set ? 0 : 1;
    if (omit == 0) {
        *length += (9 + 3); // "cert_type": 
        get_json_length_dyn_ssl_api_cert_info_cert_type(pool, (&out->cert_type), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_certificate_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->certificate.data) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (11 + 3); // "certificate": 
        get_json_length_dyn_ssl_api_cert_info_certificate(pool, (&out->certificate), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_certificateKey_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->certificateKey.data) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (14 + 3); // "certificateKey": 
        get_json_length_dyn_ssl_api_cert_info_certificateKey(pool, (&out->certificateKey), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_certificateEnc_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->certificateEnc.data) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (14 + 3); // "certificateEnc": 
        get_json_length_dyn_ssl_api_cert_info_certificateEnc(pool, (&out->certificateEnc), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_certificateKeyEnc_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->certificateKeyEnc.data) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (17 + 3); // "certificateKeyEnc": 
        get_json_length_dyn_ssl_api_cert_info_certificateKeyEnc(pool, (&out->certificateKeyEnc), length, flags);
        *length += 1; // ","
        count++;
    }
    if (count != 0) {
        *length -= 1; // "\b"
    }
    *length += 1;
}

static void get_json_length_dyn_ssl_api(njt_pool_t *pool, dyn_ssl_api_t *out, size_t *length, njt_int_t flags) {
    if (out == NULL) {
        *length += 4; // null
        return;
    }
    *length += 1;
    njt_int_t omit;
    njt_int_t count = 0;
    omit = 0;
    omit = out->is_listens_set ? 0 : 1;
    omit = (flags & OMIT_NULL_ARRAY) && (out->listens) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (7 + 3); // "listens": 
        get_json_length_dyn_ssl_api_listens(pool, (out->listens), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_serverNames_set ? 0 : 1;
    omit = (flags & OMIT_NULL_ARRAY) && (out->serverNames) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (11 + 3); // "serverNames": 
        get_json_length_dyn_ssl_api_serverNames(pool, (out->serverNames), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_type_set ? 0 : 1;
    if (omit == 0) {
        *length += (4 + 3); // "type": 
        get_json_length_dyn_ssl_api_type(pool, (&out->type), length, flags);
        *length += 1; // ","
        count++;
    }
    omit = 0;
    omit = out->is_cert_info_set ? 0 : 1;
    omit = (flags & OMIT_NULL_OBJ) && (out->cert_info) == NULL ? 1 : omit;
    if (omit == 0) {
        *length += (9 + 3); // "cert_info": 
        get_json_length_dyn_ssl_api_cert_info(pool, (out->cert_info), length, flags);
        *length += 1; // ","
        count++;
    }
    if (count != 0) {
        *length -= 1; // "\b"
    }
    *length += 1;
}
dyn_ssl_api_listens_item_t* get_dyn_ssl_api_listens_item(dyn_ssl_api_listens_t *out, size_t idx) {
    return &((dyn_ssl_api_listens_item_t*)out->elts)[idx];

}
dyn_ssl_api_serverNames_item_t* get_dyn_ssl_api_serverNames_item(dyn_ssl_api_serverNames_t *out, size_t idx) {
    return &((dyn_ssl_api_serverNames_item_t*)out->elts)[idx];

}

dyn_ssl_api_cert_info_cert_type_t get_dyn_ssl_api_cert_info_cert_type(dyn_ssl_api_cert_info_t *out) {
    return out->cert_type;
}

dyn_ssl_api_cert_info_certificate_t* get_dyn_ssl_api_cert_info_certificate(dyn_ssl_api_cert_info_t *out) {
    return &out->certificate;
}

dyn_ssl_api_cert_info_certificateKey_t* get_dyn_ssl_api_cert_info_certificateKey(dyn_ssl_api_cert_info_t *out) {
    return &out->certificateKey;
}

dyn_ssl_api_cert_info_certificateEnc_t* get_dyn_ssl_api_cert_info_certificateEnc(dyn_ssl_api_cert_info_t *out) {
    return &out->certificateEnc;
}

dyn_ssl_api_cert_info_certificateKeyEnc_t* get_dyn_ssl_api_cert_info_certificateKeyEnc(dyn_ssl_api_cert_info_t *out) {
    return &out->certificateKeyEnc;
}

dyn_ssl_api_listens_t* get_dyn_ssl_api_listens(dyn_ssl_api_t *out) {
    return out->listens;
}

dyn_ssl_api_serverNames_t* get_dyn_ssl_api_serverNames(dyn_ssl_api_t *out) {
    return out->serverNames;
}

dyn_ssl_api_type_t get_dyn_ssl_api_type(dyn_ssl_api_t *out) {
    return out->type;
}

dyn_ssl_api_cert_info_t* get_dyn_ssl_api_cert_info(dyn_ssl_api_t *out) {
    return out->cert_info;
}
int add_item_dyn_ssl_api_listens(dyn_ssl_api_listens_t *src, dyn_ssl_api_listens_item_t* item) {
    void *new = njt_array_push(src);
    if (new == NULL) {
        return NJT_ERROR;
    }
    njt_memcpy(new, item, src->size);
    return NJT_OK;
}

dyn_ssl_api_listens_t* create_dyn_ssl_api_listens(njt_pool_t *pool, size_t nelts) {
    return njt_array_create(pool, nelts, sizeof(dyn_ssl_api_listens_item_t));
}
void set_dyn_ssl_api_listens(dyn_ssl_api_t* obj, dyn_ssl_api_listens_t* field) {
    obj->listens = field;
    obj->is_listens_set = 1;
}
int add_item_dyn_ssl_api_serverNames(dyn_ssl_api_serverNames_t *src, dyn_ssl_api_serverNames_item_t* item) {
    void *new = njt_array_push(src);
    if (new == NULL) {
        return NJT_ERROR;
    }
    njt_memcpy(new, item, src->size);
    return NJT_OK;
}

dyn_ssl_api_serverNames_t* create_dyn_ssl_api_serverNames(njt_pool_t *pool, size_t nelts) {
    return njt_array_create(pool, nelts, sizeof(dyn_ssl_api_serverNames_item_t));
}
void set_dyn_ssl_api_serverNames(dyn_ssl_api_t* obj, dyn_ssl_api_serverNames_t* field) {
    obj->serverNames = field;
    obj->is_serverNames_set = 1;
}
void set_dyn_ssl_api_type(dyn_ssl_api_t* obj, dyn_ssl_api_type_t field) {
    obj->type = field;
    obj->is_type_set = 1;
}
void set_dyn_ssl_api_cert_info_cert_type(dyn_ssl_api_cert_info_t* obj, dyn_ssl_api_cert_info_cert_type_t field) {
    obj->cert_type = field;
    obj->is_cert_type_set = 1;
}
void set_dyn_ssl_api_cert_info_certificate(dyn_ssl_api_cert_info_t* obj, dyn_ssl_api_cert_info_certificate_t* field) {
    njt_memcpy(&obj->certificate, field, sizeof(njt_str_t));
    obj->is_certificate_set = 1;
}
void set_dyn_ssl_api_cert_info_certificateKey(dyn_ssl_api_cert_info_t* obj, dyn_ssl_api_cert_info_certificateKey_t* field) {
    njt_memcpy(&obj->certificateKey, field, sizeof(njt_str_t));
    obj->is_certificateKey_set = 1;
}
void set_dyn_ssl_api_cert_info_certificateEnc(dyn_ssl_api_cert_info_t* obj, dyn_ssl_api_cert_info_certificateEnc_t* field) {
    njt_memcpy(&obj->certificateEnc, field, sizeof(njt_str_t));
    obj->is_certificateEnc_set = 1;
}
void set_dyn_ssl_api_cert_info_certificateKeyEnc(dyn_ssl_api_cert_info_t* obj, dyn_ssl_api_cert_info_certificateKeyEnc_t* field) {
    njt_memcpy(&obj->certificateKeyEnc, field, sizeof(njt_str_t));
    obj->is_certificateKeyEnc_set = 1;
}
dyn_ssl_api_cert_info_t* create_dyn_ssl_api_cert_info(njt_pool_t *pool) {
    dyn_ssl_api_cert_info_t* out = njt_pcalloc(pool, sizeof(dyn_ssl_api_cert_info_t));
    return out;
}
void set_dyn_ssl_api_cert_info(dyn_ssl_api_t* obj, dyn_ssl_api_cert_info_t* field) {
    obj->cert_info = field;
    obj->is_cert_info_set = 1;
}
dyn_ssl_api_t* create_dyn_ssl_api(njt_pool_t *pool) {
    dyn_ssl_api_t* out = njt_pcalloc(pool, sizeof(dyn_ssl_api_t));
    return out;
}

static void to_oneline_json_dyn_ssl_api_listens_item(njt_pool_t *pool, dyn_ssl_api_listens_item_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    njt_str_t *dst = handle_escape_on_write(pool, out);
    cur = njt_sprintf(cur, "\"%V\"", dst);
    buf->len = cur - buf->data;
}

static void to_oneline_json_dyn_ssl_api_listens(njt_pool_t *pool, dyn_ssl_api_listens_t *out, njt_str_t* buf, njt_int_t flags) {
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
        omit = ((flags & OMIT_NULL_STR) && (&((dyn_ssl_api_listens_item_t*)out->elts)[i])->data == NULL) ? 1 : 0;
        if (omit == 0) {
            to_oneline_json_dyn_ssl_api_listens_item(pool, (&((dyn_ssl_api_listens_item_t*)out->elts)[i]), buf, flags);
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

static void to_oneline_json_dyn_ssl_api_serverNames_item(njt_pool_t *pool, dyn_ssl_api_serverNames_item_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    njt_str_t *dst = handle_escape_on_write(pool, out);
    cur = njt_sprintf(cur, "\"%V\"", dst);
    buf->len = cur - buf->data;
}

static void to_oneline_json_dyn_ssl_api_serverNames(njt_pool_t *pool, dyn_ssl_api_serverNames_t *out, njt_str_t* buf, njt_int_t flags) {
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
        omit = ((flags & OMIT_NULL_STR) && (&((dyn_ssl_api_serverNames_item_t*)out->elts)[i])->data == NULL) ? 1 : 0;
        if (omit == 0) {
            to_oneline_json_dyn_ssl_api_serverNames_item(pool, (&((dyn_ssl_api_serverNames_item_t*)out->elts)[i]), buf, flags);
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

static void to_oneline_json_dyn_ssl_api_type(njt_pool_t *pool, dyn_ssl_api_type_t *out, njt_str_t* buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    if (*out == DYN_SSL_API_TYPE_ADD) {
        cur = njt_sprintf(cur, "\"add\"");
        buf->len += 3 + 2;
        return;
    }
    if (*out == DYN_SSL_API_TYPE_DEL) {
        cur = njt_sprintf(cur, "\"del\"");
        buf->len += 3 + 2;
        return;
    }
}

static void to_oneline_json_dyn_ssl_api_cert_info_cert_type(njt_pool_t *pool, dyn_ssl_api_cert_info_cert_type_t *out, njt_str_t* buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    if (*out == DYN_SSL_API_CERT_INFO_CERT_TYPE_RSA) {
        cur = njt_sprintf(cur, "\"rsa\"");
        buf->len += 3 + 2;
        return;
    }
    if (*out == DYN_SSL_API_CERT_INFO_CERT_TYPE_NTLS) {
        cur = njt_sprintf(cur, "\"ntls\"");
        buf->len += 4 + 2;
        return;
    }
    if (*out == DYN_SSL_API_CERT_INFO_CERT_TYPE_ECC) {
        cur = njt_sprintf(cur, "\"ecc\"");
        buf->len += 3 + 2;
        return;
    }
}

static void to_oneline_json_dyn_ssl_api_cert_info_certificate(njt_pool_t *pool, dyn_ssl_api_cert_info_certificate_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    njt_str_t *dst = handle_escape_on_write(pool, out);
    cur = njt_sprintf(cur, "\"%V\"", dst);
    buf->len = cur - buf->data;
}

static void to_oneline_json_dyn_ssl_api_cert_info_certificateKey(njt_pool_t *pool, dyn_ssl_api_cert_info_certificateKey_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    njt_str_t *dst = handle_escape_on_write(pool, out);
    cur = njt_sprintf(cur, "\"%V\"", dst);
    buf->len = cur - buf->data;
}

static void to_oneline_json_dyn_ssl_api_cert_info_certificateEnc(njt_pool_t *pool, dyn_ssl_api_cert_info_certificateEnc_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    njt_str_t *dst = handle_escape_on_write(pool, out);
    cur = njt_sprintf(cur, "\"%V\"", dst);
    buf->len = cur - buf->data;
}

static void to_oneline_json_dyn_ssl_api_cert_info_certificateKeyEnc(njt_pool_t *pool, dyn_ssl_api_cert_info_certificateKeyEnc_t *out, njt_str_t *buf, njt_int_t flags) {
    u_char* cur = buf->data + buf->len;
    njt_str_t *dst = handle_escape_on_write(pool, out);
    cur = njt_sprintf(cur, "\"%V\"", dst);
    buf->len = cur - buf->data;
}

static void to_oneline_json_dyn_ssl_api_cert_info(njt_pool_t *pool, dyn_ssl_api_cert_info_t *out, njt_str_t* buf, njt_int_t flags) {
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
    omit = out->is_cert_type_set ? 0 : 1;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"cert_type\":");
        buf->len = cur - buf->data;
        to_oneline_json_dyn_ssl_api_cert_info_cert_type(pool, (&out->cert_type), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_certificate_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->certificate.data) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"certificate\":");
        buf->len = cur - buf->data;
        to_oneline_json_dyn_ssl_api_cert_info_certificate(pool, (&out->certificate), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_certificateKey_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->certificateKey.data) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"certificateKey\":");
        buf->len = cur - buf->data;
        to_oneline_json_dyn_ssl_api_cert_info_certificateKey(pool, (&out->certificateKey), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_certificateEnc_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->certificateEnc.data) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"certificateEnc\":");
        buf->len = cur - buf->data;
        to_oneline_json_dyn_ssl_api_cert_info_certificateEnc(pool, (&out->certificateEnc), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_certificateKeyEnc_set ? 0 : 1;
    omit = (flags & OMIT_NULL_STR) && (out->certificateKeyEnc.data) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"certificateKeyEnc\":");
        buf->len = cur - buf->data;
        to_oneline_json_dyn_ssl_api_cert_info_certificateKeyEnc(pool, (&out->certificateKeyEnc), buf, flags);
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

static void to_oneline_json_dyn_ssl_api(njt_pool_t *pool, dyn_ssl_api_t *out, njt_str_t* buf, njt_int_t flags) {
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
    omit = out->is_listens_set ? 0 : 1;
    omit = (flags & OMIT_NULL_ARRAY) && (out->listens) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"listens\":");
        buf->len = cur - buf->data;
        to_oneline_json_dyn_ssl_api_listens(pool, (out->listens), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_serverNames_set ? 0 : 1;
    omit = (flags & OMIT_NULL_ARRAY) && (out->serverNames) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"serverNames\":");
        buf->len = cur - buf->data;
        to_oneline_json_dyn_ssl_api_serverNames(pool, (out->serverNames), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_type_set ? 0 : 1;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"type\":");
        buf->len = cur - buf->data;
        to_oneline_json_dyn_ssl_api_type(pool, (&out->type), buf, flags);
        cur = buf->data + buf->len;
        cur = njt_sprintf(cur, ",");
        buf->len ++;
    }
    omit = 0;
    omit = out->is_cert_info_set ? 0 : 1;
    omit = (flags & OMIT_NULL_OBJ) && (out->cert_info) == NULL ? 1 : omit;
    if (omit == 0) {
        cur = njt_sprintf(cur, "\"cert_info\":");
        buf->len = cur - buf->data;
        to_oneline_json_dyn_ssl_api_cert_info(pool, (out->cert_info), buf, flags);
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
dyn_ssl_api_t* json_parse_dyn_ssl_api(njt_pool_t *pool, const njt_str_t *json_string, js2c_parse_error_t *err_ret) {
    dyn_ssl_api_t* out;
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
    out = njt_pcalloc(pool, sizeof(dyn_ssl_api_t));;
    memset(out, 0, sizeof(dyn_ssl_api_t));
    if (parse_dyn_ssl_api(pool, parse_state, out, err_ret)) {
        return NULL;
    }
    return out;
}

njt_str_t* to_json_dyn_ssl_api(njt_pool_t *pool, dyn_ssl_api_t* out, njt_int_t flags) {
    njt_str_t *json_str;
    json_str = njt_pcalloc(pool, sizeof(njt_str_t));
    size_t str_len = 0;
    get_json_length_dyn_ssl_api(pool, out, &str_len, flags);
    json_str->data = (u_char*)njt_pcalloc(pool, str_len + 1);
    json_str->len = 0;
    to_oneline_json_dyn_ssl_api(pool, out, json_str, flags);
    return json_str;
}
