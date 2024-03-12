

/* This file was generated by JSON Schema to C.
 * Any changes made to it will be lost on regeneration. 

 * Copyright (C) 2021-2023  TMLake(Beijing) Technology Co., Ltd.
 */

#ifndef PARSER_H
#define PARSER_H
#include <stdint.h>
#include <stdbool.h>
#include "njt_core.h"
#include "js2c_njet_builtins.h"
/* ===================== Generated type declarations ===================== */
typedef njt_str_t httpmap_maps_item_keyFrom_t;

typedef njt_str_t httpmap_maps_item_keyTo_t;

typedef njt_str_t httpmap_maps_item_type_t;

typedef njt_str_t httpmap_maps_item_values_item_valueFrom_t;

typedef njt_str_t httpmap_maps_item_values_item_valueTo_t;

typedef struct httpmap_maps_item_values_item_t_s {
    httpmap_maps_item_values_item_valueFrom_t valueFrom;
    httpmap_maps_item_values_item_valueTo_t valueTo;
    unsigned int is_valueFrom_set:1;
    unsigned int is_valueTo_set:1;
} httpmap_maps_item_values_item_t;

typedef njt_array_t  httpmap_maps_item_values_t;
typedef bool httpmap_maps_item_isVolatile_t;
typedef bool httpmap_maps_item_hostnames_t;
typedef struct httpmap_maps_item_t_s {
    httpmap_maps_item_keyFrom_t keyFrom;
    httpmap_maps_item_keyTo_t keyTo;
    httpmap_maps_item_type_t type;
    httpmap_maps_item_values_t *values;
    httpmap_maps_item_isVolatile_t isVolatile;
    httpmap_maps_item_hostnames_t hostnames;
    unsigned int is_keyFrom_set:1;
    unsigned int is_keyTo_set:1;
    unsigned int is_type_set:1;
    unsigned int is_values_set:1;
    unsigned int is_isVolatile_set:1;
    unsigned int is_hostnames_set:1;
} httpmap_maps_item_t;

typedef njt_array_t  httpmap_maps_t;
typedef struct httpmap_t_s {
    httpmap_maps_t *maps;
    unsigned int is_maps_set:1;
} httpmap_t;

httpmap_maps_item_values_item_valueFrom_t* get_httpmap_maps_item_values_item_valueFrom(httpmap_maps_item_values_item_t *out);
httpmap_maps_item_values_item_valueTo_t* get_httpmap_maps_item_values_item_valueTo(httpmap_maps_item_values_item_t *out);
httpmap_maps_item_values_item_t* get_httpmap_maps_item_values_item(httpmap_maps_item_values_t *out, size_t idx);
httpmap_maps_item_keyFrom_t* get_httpmap_maps_item_keyFrom(httpmap_maps_item_t *out);
httpmap_maps_item_keyTo_t* get_httpmap_maps_item_keyTo(httpmap_maps_item_t *out);
httpmap_maps_item_type_t* get_httpmap_maps_item_type(httpmap_maps_item_t *out);
// CHECK ARRAY not exceeding bounds before calling this func
httpmap_maps_item_values_t* get_httpmap_maps_item_values(httpmap_maps_item_t *out);
httpmap_maps_item_isVolatile_t get_httpmap_maps_item_isVolatile(httpmap_maps_item_t *out);
httpmap_maps_item_hostnames_t get_httpmap_maps_item_hostnames(httpmap_maps_item_t *out);
httpmap_maps_item_t* get_httpmap_maps_item(httpmap_maps_t *out, size_t idx);
// CHECK ARRAY not exceeding bounds before calling this func
httpmap_maps_t* get_httpmap_maps(httpmap_t *out);
void set_httpmap_maps_item_keyFrom(httpmap_maps_item_t* obj, httpmap_maps_item_keyFrom_t* field);
void set_httpmap_maps_item_keyTo(httpmap_maps_item_t* obj, httpmap_maps_item_keyTo_t* field);
void set_httpmap_maps_item_type(httpmap_maps_item_t* obj, httpmap_maps_item_type_t* field);
void set_httpmap_maps_item_values_item_valueFrom(httpmap_maps_item_values_item_t* obj, httpmap_maps_item_values_item_valueFrom_t* field);
void set_httpmap_maps_item_values_item_valueTo(httpmap_maps_item_values_item_t* obj, httpmap_maps_item_values_item_valueTo_t* field);
httpmap_maps_item_values_item_t* create_httpmap_maps_item_values_item(njt_pool_t *pool);
int add_item_httpmap_maps_item_values(httpmap_maps_item_values_t *src, httpmap_maps_item_values_item_t* items);
httpmap_maps_item_values_t* create_httpmap_maps_item_values(njt_pool_t *pool, size_t nelts);
void set_httpmap_maps_item_values(httpmap_maps_item_t* obj, httpmap_maps_item_values_t* field);
void set_httpmap_maps_item_isVolatile(httpmap_maps_item_t* obj, httpmap_maps_item_isVolatile_t field);
void set_httpmap_maps_item_hostnames(httpmap_maps_item_t* obj, httpmap_maps_item_hostnames_t field);
httpmap_maps_item_t* create_httpmap_maps_item(njt_pool_t *pool);
int add_item_httpmap_maps(httpmap_maps_t *src, httpmap_maps_item_t* items);
httpmap_maps_t* create_httpmap_maps(njt_pool_t *pool, size_t nelts);
void set_httpmap_maps(httpmap_t* obj, httpmap_maps_t* field);
httpmap_t* create_httpmap(njt_pool_t *pool);
httpmap_t* json_parse_httpmap(njt_pool_t *pool, const njt_str_t *json_string, js2c_parse_error_t *err_ret);
njt_str_t* to_json_httpmap(njt_pool_t *pool, httpmap_t *out, njt_int_t flags);
#endif /* PARSER_H */
