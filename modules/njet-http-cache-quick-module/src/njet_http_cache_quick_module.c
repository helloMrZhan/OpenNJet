/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 * Copyright (C) 2021-2023  TMLake(Beijing) Technology Co., Ltd.
 */
#include <njt_config.h>
#include <njt_core.h>
#include <njt_http.h>
#include <njt_stream.h>
#include <njt_json_api.h>
#include <njt_str_util.h>
#include <math.h>
#include <njt_http_kv_module.h>
#include <njt_http_sendmsg_module.h>
#include <njt_rpc_result_util.h>
#include <njt_rpc_result_parser.h>
#include <njt_http_util.h>

#include "njet_http_parser_cache.h"
#include "njet_http_parser_cache_api.h"
#include "njet_http_parser_cache_dynloc.h"


#define MIN_CONFIG_BODY_LEN 2
#define MAX_CONFIG_BODY_LEN 5242880
#define HTTP_CACHE_QUICK_CONFS "cache_quick_confs"
#define DOWNLOAD_STATUS_INFO "download_ratio: %d"


#define NJT_HTTP_CACHE_QUICK_PARSE_INIT           0
#define NJT_HTTP_CACHE_QUICK_PARSE_STATUS_LINE    1
#define NJT_HTTP_CACHE_QUICK_PARSE_HEADER         2
#define NJT_HTTP_CACHE_QUICK_PARSE_BODY           4

extern njt_uint_t njt_worker;
extern njt_module_t  njt_http_rewrite_module;
extern njt_cycle_t *njet_master_cycle;


typedef enum cache_api_status_t_e{
    CACHE_API_STATUS_NONE,
    CACHE_API_STATUS_ADD_LOC_OK,
    CACHE_API_STATUS_DEL_LOC_OK,
    CACHE_API_STATUS_OK,
    CACHE_API_STATUS_ERROR
} cache_api_status_t;



static njt_int_t
njt_http_cache_quick_handler(njt_http_request_t *r);

static njt_int_t
njt_http_cache_quick_init_worker(njt_cycle_t *cycle);

static void *
njt_http_cache_quick_create_loc_conf(njt_conf_t *cf);

static njt_int_t njt_http_cache_quick_init_module(njt_cycle_t *cycle);

static njt_int_t
njt_http_cache_quick_init(njt_conf_t *cf);

static char *
njt_http_cache_quick(njt_conf_t *cf, njt_command_t *cmd, void *conf);

njt_int_t
njt_http_cache_quick_lvlhsh_test(njt_lvlhsh_query_t *lhq, void *data);


#if (NJT_OPENSSL)
typedef struct njt_http_cache_quick_ssl_conf_s {
    njt_flag_t ssl_enable;
    njt_flag_t ntls_enable;
    njt_flag_t ssl_session_reuse;
    njt_uint_t ssl_protocols;
    njt_str_t ssl_protocol_str;
    njt_str_t ssl_ciphers;
    njt_str_t ssl_name;
    njt_flag_t ssl_server_name;
    njt_flag_t ssl_verify;
    njt_int_t ssl_verify_depth;
    njt_str_t ssl_trusted_certificate;
    njt_str_t ssl_crl;
    njt_str_t ssl_certificate;
    njt_str_t ssl_certificate_key;
    njt_str_t ssl_enc_certificate;
    njt_str_t ssl_enc_certificate_key;
    njt_array_t *ssl_passwords;
    njt_array_t *ssl_conf_commands;
    njt_ssl_t *ssl;
} njt_http_cache_quick_ssl_conf_t;
#endif


typedef struct njt_http_cache_resouce_metainfo_s{
    njt_str_t       addr_port;
    njt_str_t       server_name;
    njt_str_t       location_rule;
    njt_str_t       location_name;
    njt_str_t       location_body;
    njt_str_t       proxy_pass;
    
    njt_int_t       ssl_enable;
#if (NJT_OPENSSL)
    njt_http_cache_quick_ssl_conf_t ssl;
#endif

    uint32_t        crc32;
    njt_str_t       crc32_str;

    ssize_t         resource_size;
    ssize_t         current_size;

    cache_api_status_t  status;
    njt_str_t       status_str;
    njt_event_t     download_timer;
    njt_pool_t      *item_pool;

    njt_queue_t     cache_item;
} njt_http_cache_resouce_metainfo_t;


typedef struct njt_http_cache_quick_loc_conf_s{
    njt_flag_t      cache_quick_enable;
} njt_http_cache_quick_loc_conf_t;


typedef struct njt_http_cache_quick_main_conf_s{
    njt_lvlhsh_t    resource_to_metainfo; //key: addr(server_name, location)   value:metainfo
    njt_queue_t     caches;
    njt_pool_t      *cache_pool;            //used for map
} njt_http_cache_quick_main_conf_t;



/*Main structure of the download information of a peer*/
typedef struct njt_http_cache_quick_download_peer_s {
    njt_pool_t                      *pool;
    // njt_str_t                       server;
    njt_str_t                       location_name;
    njt_peer_connection_t           *peer;
    njt_buf_t                       *send_buf;
    njt_buf_t                       *recv_buf;
    njt_chain_t                     *recv_chain;
    njt_chain_t                     *last_chain_node;
    void                            *parser;

    uint32_t                        crc32;
    njt_str_t                       crc32_str;
    njt_str_t                       host;
#if (NJT_HTTP_SSL)
    njt_str_t                       ssl_name;
#endif
}njt_http_cache_quick_download_peer_t;


/*Structure used for holding http parser internal info*/
typedef struct njt_cache_quick_http_parse_s {
    njt_uint_t state;
    njt_uint_t code;
    njt_flag_t done;
    njt_flag_t body_used;
    njt_uint_t stage;
    u_char *status_text;
    u_char *status_text_end;
    njt_uint_t count;
    njt_flag_t chunked;
    off_t content_length_n;
    njt_array_t headers;
    u_char *header_name_start;
    u_char *header_name_end;
    u_char *header_start;
    u_char *header_end;
    u_char *body_start;

    njt_int_t (*process)(njt_http_cache_quick_download_peer_t *cq_peer);

    njt_msec_t start;
}njt_cache_quick_http_parse_t;


const njt_lvlhsh_proto_t  njt_http_cache_quick_lvlhsh_proto = {
    NJT_LVLHSH_LARGE_MEMALIGN,
    njt_http_cache_quick_lvlhsh_test,
    njt_lvlhsh_pool_alloc,
    njt_lvlhsh_pool_free,
};


njt_int_t
njt_http_cache_quick_lvlhsh_test(njt_lvlhsh_query_t *lhq, void *data)
{
    //ignore value compare, just return ok
    return NJT_OK;
}


static njt_command_t njt_http_cache_quick_commands[] = {
        {
                njt_string("cache_quick_api"),
                NJT_HTTP_LOC_CONF|NJT_CONF_NOARGS,
                njt_http_cache_quick,
                NJT_HTTP_LOC_CONF_OFFSET,
                0,
                NULL
        },
        njt_null_command
};


static njt_http_module_t njt_http_cache_quick_module_ctx = {
        NULL,                              /* preconfiguration */
        njt_http_cache_quick_init,                              /* postconfiguration */

        NULL,                              /* create main configuration */
        NULL,                              /* init main configuration */

        NULL,                              /* create server configuration */
        NULL,                              /* merge server configuration */

        njt_http_cache_quick_create_loc_conf, /* create location configuration */
        NULL   /* merge location configuration */
};

njt_module_t njt_http_cache_quick_module = {
        NJT_MODULE_V1,
        &njt_http_cache_quick_module_ctx, /* module context */
        njt_http_cache_quick_commands,    /* module directives */
        NJT_HTTP_MODULE,                    /* module type */
        NULL,                               /* init master */
        njt_http_cache_quick_init_module,   /* init module */
        njt_http_cache_quick_init_worker, /* init process */
        NULL,                               /* init thread */
        NULL,                               /* exit thread */
        NULL,                               /* exit process */
        NULL,                               /* exit master */
        NJT_MODULE_V1_PADDING
};


static char *
njt_http_cache_quick(njt_conf_t *cf, njt_command_t *cmd, void *conf) {
    
	njt_http_cache_quick_loc_conf_t   *cqmf = conf;

    cqmf->cache_quick_enable = 1;

    return NJT_CONF_OK;
}


static njt_int_t njt_http_cache_quick_init_module(njt_cycle_t *cycle) {
    njt_http_cache_quick_main_conf_t *cqmf;

    cqmf = njt_pcalloc(cycle->pool, sizeof(njt_http_cache_quick_main_conf_t));
    if (cqmf == NULL) {
        njt_log_error(NJT_LOG_ERR, cycle->log, 0, "malloc njt_http_cache_quick_main_conf_t eror");
        return NJT_ERROR;
    }

    njt_queue_init(&cqmf->caches);
    njt_lvlhsh_init(&cqmf->resource_to_metainfo);

    if(cqmf->cache_pool == NULL){
        cqmf->cache_pool = njt_create_dynamic_pool(NJT_MIN_POOL_SIZE, cycle->log);
        if (cqmf->cache_pool == NULL) {
            njt_log_error(NJT_LOG_EMERG, cycle->log, 0,
                    " cache quick create dynamic pool error");

            return NJT_ERROR;
        }

        njt_sub_pool(cycle->pool, cqmf->cache_pool);
    }

    cycle->conf_ctx[njt_http_cache_quick_module.index] = (void *) cqmf;

    return NJT_OK;
}


static njt_int_t
njt_http_cache_quick_init(njt_conf_t *cf) {
    njt_http_core_main_conf_t  *cmcf;
    njt_http_handler_pt        *h;

    cmcf = njt_http_conf_get_module_main_conf(cf, njt_http_core_module);
    h = njt_array_push(&cmcf->phases[NJT_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL) {
        return NJT_ERROR;
    }

    *h = njt_http_cache_quick_handler;

    return NJT_OK;
}


static void *
njt_http_cache_quick_create_loc_conf(njt_conf_t *cf) {
    njt_http_cache_quick_loc_conf_t *cqmf;

    cqmf = njt_pcalloc(cf->pool, sizeof(njt_http_cache_quick_loc_conf_t));
    if (cqmf == NULL) {
        njt_log_error(NJT_LOG_ERR, cf->log, 0, "malloc cqmf eror");
        return NULL;
    }

    cqmf->cache_quick_enable = 0;

    return cqmf;
}



static njt_str_t *njt_http_caches_to_json(njt_pool_t *pool, njt_http_cache_quick_main_conf_t *cqmf) {
    njt_http_cache_resouce_metainfo_t       *cache_info;
    njt_queue_t                             *q;
    cache_t                                 dynjson_obj;
    cache_caches_item_t                     *cache_item;
    cache_caches_item_download_ratio_t      ratio;           
    // njt_str_t                               tmp_str;


    njt_memzero(&dynjson_obj, sizeof(cache_t));

    set_cache_caches(&dynjson_obj, create_cache_caches(pool, 4));
    if(dynjson_obj.caches == NULL){
        goto err;
    }

    q = njt_queue_head(&cqmf->caches);
    for (; q != njt_queue_sentinel(&cqmf->caches); q = njt_queue_next(q)) {
        cache_info = njt_queue_data(q, njt_http_cache_resouce_metainfo_t, cache_item);
        cache_item = create_cache_caches_item(pool);
        if(cache_item == NULL ){
            goto err;
        }

        set_cache_caches_item_addr_port(cache_item, &cache_info->addr_port);
        set_cache_caches_item_server_name(cache_item, &cache_info->server_name);
        set_cache_caches_item_location_rule(cache_item, &cache_info->location_rule);
        set_cache_caches_item_location_name(cache_item, &cache_info->location_name);
        set_cache_caches_item_location_body(cache_item, &cache_info->location_body);
        set_cache_caches_item_proxy_pass(cache_item, &cache_info->proxy_pass);

        set_cache_caches_item_status(cache_item, &cache_info->status_str);
        // switch (cache_info->status)
        // {
        // case CACHE_API_STATUS_NONE:
        //     njt_str_set(&tmp_str, "init status");
        //     set_cache_caches_item_status(cache_item, &tmp_str);
        //     break;
        // case CACHE_API_STATUS_ADD_LOC_OK:
        //     njt_str_set(&tmp_str, "add dyn location ok");
        //     set_cache_caches_item_status(cache_item, &tmp_str);
        //     break;
        // case CACHE_API_STATUS_OK:
        //     njt_str_set(&tmp_str, "download ok");
        //     set_cache_caches_item_status(cache_item, &tmp_str);
        //     break;
        // case CACHE_API_STATUS_ERROR:
        //     njt_str_set(&tmp_str, "has error");
        //     set_cache_caches_item_status(cache_item, &tmp_str);
        //     break;
        // default:
        //     njt_str_set(&tmp_str, "init status");
        //     set_cache_caches_item_status(cache_item, &tmp_str);
        //     break;
        // }

        if(cache_info->current_size < 1){
            set_cache_caches_item_download_ratio(cache_item, 0);
        }else{
            ratio = (cache_info->resource_size * 1.0) / (cache_info->current_size * 1.0) * 100;
            set_cache_caches_item_download_ratio(cache_item, ratio);
        }

        add_item_cache_caches(dynjson_obj.caches, cache_item);
    }

    return to_json_cache(pool, &dynjson_obj, OMIT_NULL_ARRAY | OMIT_NULL_OBJ | OMIT_NULL_STR);

    err:
    return NULL;
}


static njt_int_t njt_http_get_caches(njt_http_request_t *r, njt_http_cache_quick_main_conf_t *cqmf,
            njt_rpc_result_t *rpc_result) {
    njt_int_t                           rc;
    njt_buf_t                           *buf;
    njt_chain_t                         out;
    njt_str_t                           *json;

    rc = njt_http_discard_request_body(r);
    if (rc == NJT_ERROR || rc >= NJT_HTTP_SPECIAL_RESPONSE) {
        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)" cache quick discard request body error");
        
        return NJT_ERROR;
    }

    json = njt_http_caches_to_json(r->pool, cqmf);
    if (json == NULL || json->len == 0) {
        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)" cache quick caches to json error");
        
        return NJT_ERROR;
    }

    buf = njt_create_temp_buf(r->pool, json->len);
    if(buf == NULL){
        njt_log_error(NJT_LOG_ERR, r->connection->log, 0, "njt_create_temp_buf error , size :%ui" ,json->len);
        
        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)" cache quick create tmp buffer error");
        
        return NJT_ERROR;
    }

    buf->last = buf->pos + json->len;
    njt_memcpy(buf->pos, json->data, json->len);
    r->headers_out.status = NJT_HTTP_OK;
    njt_str_t type = njt_string("application/json");
    r->headers_out.content_type = type;
    r->headers_out.content_length_n = buf->last - buf->pos;
    if (r->headers_out.content_length) {
        r->headers_out.content_length->hash = 0;
        r->headers_out.content_length = NULL;
    }

    rc = njt_http_send_header(r);
    if (rc == NJT_ERROR || rc > NJT_OK || r->header_only) {
        return rc;
    }

    buf->last_buf = 1;
    out.buf = buf;
    out.next = NULL;

    return njt_http_output_filter(r, &out);
}


static uint32_t njt_cache_quick_item_crc32(cache_api_t *api_data){
    uint32_t                                 crc32;

    njt_crc32_init(crc32);
    if(api_data->is_addr_port_set){
        njt_crc32_update(&crc32, api_data->addr_port.data, api_data->addr_port.len);
    }
    if(api_data->is_server_name_set){
        njt_crc32_update(&crc32, api_data->server_name.data, api_data->server_name.len);
    }
    if(api_data->is_location_rule_set){
        njt_crc32_update(&crc32, api_data->location_rule.data, api_data->location_rule.len);
    }
    if(api_data->is_location_name_set){
        njt_crc32_update(&crc32, api_data->location_name.data, api_data->location_name.len);
    }
    njt_crc32_final(crc32);

    return crc32;
}


static njt_int_t njt_cache_quick_item_exist(uint32_t crc32,
            njt_http_cache_quick_main_conf_t *cqmf,
            njt_http_cache_resouce_metainfo_t **cache_info){
    njt_lvlhsh_query_t                      lhq;
    u_char                                  buff[50];
    u_char                                  *end;
    
    end = njt_snprintf(buff, 50, "%d", crc32);
    lhq.key.data = buff;
    lhq.key.len = end - buff;
    lhq.key_hash = njt_murmur_hash2(lhq.key.data, lhq.key.len);
    lhq.proto = &njt_http_cache_quick_lvlhsh_proto;
    lhq.pool = cqmf->cache_pool;

    if(NJT_OK == njt_lvlhsh_find(&cqmf->resource_to_metainfo, &lhq)){
        *cache_info = lhq.value;
        return NJT_OK;
    }

    return NJT_ERROR;
}


static njt_int_t njt_http_del_cache_item_from_lvlhash(uint32_t crc32,
        njt_http_cache_quick_main_conf_t *cqmf){
    njt_lvlhsh_query_t                      lhq;
    u_char                                  buff[50];
    u_char                                  *end;
    
    end = njt_snprintf(buff, 50, "%d", crc32);
    lhq.key.data = buff;
    lhq.key.len = end - buff;
    lhq.key_hash = njt_murmur_hash2(lhq.key.data, lhq.key.len);
    lhq.proto = &njt_http_cache_quick_lvlhsh_proto;
    lhq.pool = cqmf->cache_pool;

    return njt_lvlhsh_delete(&cqmf->resource_to_metainfo, &lhq);
}


static int njt_http_cache_quicky_add_dynloc_rpc_msg_handler(njt_dyn_rpc_res_t* res, njt_str_t *msg){
    njt_http_cache_resouce_metainfo_t   *cache_info;
    njt_int_t                           rc = NJT_OK;
    js2c_parse_error_t                  err_info;
    njt_pool_t                          *tmp_pool;
    njt_str_t                           tmp_str;
    rpc_result_t                        *rpc_res;

    cache_info = res->data;

    if(res->rc == RPC_RC_OK){
        tmp_pool = njt_create_pool(NJT_MIN_POOL_SIZE, njt_cycle->log);
        if(tmp_pool == NULL){
            njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
                    " cache quick dyn loc result msg create pool error");

            cache_info->status = CACHE_API_STATUS_ERROR;
            njt_str_set(&tmp_str, "cache quick dyn loc result msg create pool error");
            cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &tmp_str);
            cache_info->status_str.len = tmp_str.len;

            return NJT_ERROR;
        }

        rpc_res = json_parse_rpc_result(tmp_pool, msg, &err_info);
        if(rpc_res == NULL){
            njt_destroy_pool(tmp_pool);

            njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
                    " cache quick dyn loc result msg parse error");

            cache_info->status = CACHE_API_STATUS_ERROR;
            njt_str_set(&tmp_str, "cache quick dyn loc result msg parse error");
            cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &tmp_str);
            cache_info->status_str.len = tmp_str.len;

            return NJT_ERROR;
        }

        if(rpc_res->code != 0){
            njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
                    " cache quick add dyn loc error:%V", &rpc_res->msg);

            cache_info->status = CACHE_API_STATUS_ERROR;
            cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &rpc_res->msg);
            cache_info->status_str.len = rpc_res->msg.len;

            njt_destroy_pool(tmp_pool);

            return NJT_ERROR;
        }else{
            njt_destroy_pool(tmp_pool);
            cache_info->status = CACHE_API_STATUS_ADD_LOC_OK;

            njt_log_error(NJT_LOG_INFO, njt_cycle->log, 0, 
                    " cache quick add dyn loc ok, location:%V", &cache_info->location_name);

            return NJT_OK;
        }
    }

    if(res->rc == RPC_RC_TIMEOUT){
        cache_info->status = CACHE_API_STATUS_ERROR;

        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
                    " cache quick add dyn loc rpc timeout, location:%V", &cache_info->location_name);

        njt_str_set(&tmp_str, "cache quick add dyn loc rpc timeout");
        cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &tmp_str);
        cache_info->status_str.len = tmp_str.len;

        return NJT_ERROR;
    }

    return rc;
}


static int njt_http_cache_quicky_del_dynloc_rpc_msg_handler(njt_dyn_rpc_res_t* res, njt_str_t *msg){
    njt_int_t                           rc = NJT_OK;
    js2c_parse_error_t                  err_info;
    njt_pool_t                          *tmp_pool;
    rpc_result_t                        *rpc_res;


    if(res->rc == RPC_RC_OK){
        tmp_pool = njt_create_pool(NJT_MIN_POOL_SIZE, njt_cycle->log);
        if(tmp_pool == NULL){
            njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
                " cache quick del dyn loc create pool error");

            return NJT_ERROR;
        }

        rpc_res = json_parse_rpc_result(tmp_pool, msg, &err_info);
        if(rpc_res == NULL){
            njt_destroy_pool(tmp_pool);
            njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
                " cache quick dyn loc result msg parse error");

            return NJT_ERROR;
        }

        if(rpc_res->code != 0){
            njt_destroy_pool(tmp_pool);
            njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
                " cache quick del dyn loc error");

            return NJT_ERROR;
        }else{
            njt_destroy_pool(tmp_pool);
            njt_log_error(NJT_LOG_INFO, njt_cycle->log, 0, 
                    " cache quick del dyn loc success");

            return NJT_OK;
        }
    }

    if(res->rc == RPC_RC_TIMEOUT){
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
                " cache quick del dyn loc rpc timeout");

        return NJT_ERROR;
    }

    return rc;
}



static njt_int_t njt_http_cache_item_add_dyn_location(
        njt_http_cache_resouce_metainfo_t *cache_info, njt_http_cache_quick_main_conf_t *cqmf){
    njt_int_t                       rc = NJT_OK;
    cache_dyn_location_t            dyn_location;
    njt_pool_t                      *pool = NULL;
    njt_str_t                       tmp_str;
    njt_str_t                       *msg;
    uint32_t                        crc32;
    u_char                          buf[100];
    u_char                          *p;
    njt_str_t                       topic_name;
    cache_dyn_location_locations_item_t *dyn_loc_item;


    pool = njt_create_pool(njt_pagesize, njt_cycle->log);
    if (pool == NULL)
    {
        njt_log_error(NJT_LOG_EMERG, njt_cycle->log, 0, " cache quick create dyn loc pool error");
        
        cache_info->status = CACHE_API_STATUS_ERROR;
        njt_str_set(&tmp_str, "cache quick create dyn loc pool error");
        cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &tmp_str);
        cache_info->status_str.len = tmp_str.len;
        
        return NJT_ERROR;
    }

    //create dyn location structure and get json str
    set_cache_dyn_location_type(&dyn_location, CACHE_DYN_LOCATION_TYPE_ADD);
    set_cache_dyn_location_addr_port(&dyn_location, &cache_info->addr_port);
    set_cache_dyn_location_server_name(&dyn_location, &cache_info->server_name);
    set_cache_dyn_location_locations(&dyn_location, create_cache_dyn_location_locations(pool, 1));
    if(dyn_location.locations == NULL){
        njt_log_error(NJT_LOG_EMERG, njt_cycle->log, 0, " cache quick create dyn loc locations error");
        
        cache_info->status = CACHE_API_STATUS_ERROR;
        njt_str_set(&tmp_str, "cache quick create dyn loc locations error");
        cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &tmp_str);
        cache_info->status_str.len = tmp_str.len;
        
        rc = NJT_ERROR;
        goto dyn_loc_out;
    }

    dyn_loc_item = create_cache_dyn_location_locations_item(pool);
    if(dyn_loc_item == NULL){
        njt_log_error(NJT_LOG_EMERG, njt_cycle->log, 0, " cache quick create dyn loc item error");
        
        cache_info->status = CACHE_API_STATUS_ERROR;
        njt_str_set(&tmp_str, "cache quick create dyn loc item error");
        cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &tmp_str);
        cache_info->status_str.len = tmp_str.len;
        
        rc = NJT_ERROR;
        goto dyn_loc_out;
    }

    set_cache_dyn_location_locations_item_location_rule(dyn_loc_item, &cache_info->location_rule);
    set_cache_dyn_location_locations_item_location_name(dyn_loc_item, &cache_info->location_name);
    set_cache_dyn_location_locations_item_location_body(dyn_loc_item, &cache_info->location_body);
    set_cache_dyn_location_locations_item_proxy_pass(dyn_loc_item, &cache_info->proxy_pass);

    add_item_cache_dyn_location_locations(dyn_location.locations, dyn_loc_item);

    msg = to_json_cache_dyn_location(pool, &dyn_location, OMIT_NULL_ARRAY | OMIT_NULL_OBJ | OMIT_NULL_STR);
    if(msg == NULL){
        njt_log_error(NJT_LOG_EMERG, njt_cycle->log, 0, " cache quick location json parse error");
        
        cache_info->status = CACHE_API_STATUS_ERROR;
        njt_str_set(&tmp_str, "cache quick location json parse error");
        cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &tmp_str);
        cache_info->status_str.len = tmp_str.len;
        
        rc = NJT_ERROR;
        goto dyn_loc_out;
    }

	njt_crc32_init(crc32);
	njt_crc32_update(&crc32, cache_info->addr_port.data, cache_info->addr_port.len);
	if (cache_info->server_name.len > 0) {
		njt_crc32_update(&crc32, cache_info->server_name.data, cache_info->server_name.len);
	}
	if (cache_info->location_rule.len > 0) {
		njt_crc32_update(&crc32, cache_info->location_rule.data, cache_info->location_rule.len);
	}

	njt_crc32_update(&crc32, cache_info->location_name.data, cache_info->location_name.len);
	njt_crc32_final(crc32);

    topic_name.data = buf;
    p = njt_snprintf(buf, 100, "/worker_0/ins/loc/l_%ui", crc32);
    topic_name.len = p - buf;

    njt_log_error(NJT_LOG_INFO, njt_cycle->log, 0, 
        " cache quick add dyn location, topic:%V  msg:%V", &topic_name, msg);
    
    //call njt_rpc_send to add dyn location
    njt_dyn_rpc(&topic_name, msg, 0, 0, njt_http_cache_quicky_add_dynloc_rpc_msg_handler, cache_info);

dyn_loc_out:
    if (pool != NULL)
    {
        njt_destroy_pool(pool);
    }

    return rc;
}


static njt_int_t njt_http_cache_item_del_dyn_location(njt_http_cache_resouce_metainfo_t *cache_info,
            njt_str_t *json_str, njt_http_cache_quick_main_conf_t *cqmf){
    uint32_t                        crc32;
    u_char                          buf[100];
    u_char                          *p;
    njt_str_t                       topic_name;


	njt_crc32_init(crc32);
	njt_crc32_update(&crc32, cache_info->addr_port.data, cache_info->addr_port.len);
	if (cache_info->server_name.len > 0) {
		njt_crc32_update(&crc32, cache_info->server_name.data, cache_info->server_name.len);
	}
	if (cache_info->location_rule.len > 0) {
		njt_crc32_update(&crc32, cache_info->location_rule.data, cache_info->location_rule.len);
	}

	njt_crc32_update(&crc32, cache_info->location_name.data, cache_info->location_name.len);
	njt_crc32_final(crc32);

    topic_name.data = buf;
    p = njt_snprintf(buf, 100, "/ins/loc/l_%ui", crc32);
    topic_name.len = p - buf;

    njt_log_error(NJT_LOG_INFO, njt_cycle->log, 0, 
        " cache quick del dyn location, topic:%V  json:%V", &topic_name, json_str);

    //call njt_rpc_send to add dyn location
    njt_dyn_rpc(&topic_name, json_str, 0, 0, njt_http_cache_quicky_del_dynloc_rpc_msg_handler, NULL);

    return NJT_OK;
}



static njt_int_t njt_http_add_cache_item_to_lvlhash(uint32_t crc32,
        njt_http_cache_resouce_metainfo_t *cache_info, njt_http_cache_quick_main_conf_t *cqmf){
    njt_lvlhsh_query_t                      lhq;
    u_char                                  buff[50];
    u_char                                  *end;
    
    end = njt_snprintf(buff, 50, "%d", crc32);
    lhq.key.data = buff;
    lhq.key.len = end - buff;

    //update cache item crc32 info
    cache_info->crc32 = crc32;
    cache_info->crc32_str.data = njt_pstrdup(cache_info->item_pool, &lhq.key);
    cache_info->crc32_str.len = lhq.key.len;

    lhq.key_hash = njt_murmur_hash2(lhq.key.data, lhq.key.len);
    lhq.proto = &njt_http_cache_quick_lvlhsh_proto;
    lhq.pool = cqmf->cache_pool;
    lhq.value = cache_info;

    return njt_lvlhsh_insert(&cqmf->resource_to_metainfo, &lhq);
}


static njt_int_t njt_http_del_cache_item_from_queue(
            njt_http_cache_quick_main_conf_t *cqmf, cache_api_t *api_data){
    njt_queue_t             *q;

    njt_http_cache_resouce_metainfo_t       *cache_info = NULL;
    njt_uint_t                              found = 0;

    //find item
    q = njt_queue_head(&cqmf->caches);
    for (; q != njt_queue_sentinel(&cqmf->caches); q = njt_queue_next(q)) {
        cache_info = njt_queue_data(q, njt_http_cache_resouce_metainfo_t, cache_item);
        if(cache_info->addr_port.len != api_data->addr_port.len){
            continue;
        }

        if(cache_info->addr_port.len > 0
            && njt_strncmp(cache_info->addr_port.data, api_data->addr_port.data, cache_info->addr_port.len) != 0){
                continue;
        }

        if(cache_info->server_name.len != api_data->server_name.len){
            continue;
        }

        if(cache_info->server_name.len > 0
            && njt_strncmp(cache_info->server_name.data, api_data->server_name.data, cache_info->server_name.len) != 0){
                continue;
        }

        if(cache_info->location_rule.len != api_data->location_rule.len){
            continue;
        }

        if(cache_info->location_rule.len > 0
            && njt_strncmp(cache_info->location_rule.data, api_data->location_rule.data, cache_info->location_rule.len) != 0){
                continue;
        }

        if(cache_info->location_name.len != api_data->location_name.len){
            continue;
        }

        if(cache_info->location_name.len > 0
            && njt_strncmp(cache_info->location_name.data, api_data->location_name.data, cache_info->location_name.len) != 0){
                continue;
        }

        found = 1;
        break;
    }

    if(found){
        njt_queue_remove(&cache_info->cache_item);
        //free memory
        if(cache_info->item_pool != NULL){
            if(cache_info->download_timer.timer_set){
                njt_del_timer(&cache_info->download_timer);
            }
 
            njt_destroy_pool(cache_info->item_pool);
        }

        return NJT_OK;
    }


    return NJT_ERROR;
}


static njt_http_cache_resouce_metainfo_t *njt_http_add_cache_item_to_queue(
            njt_http_cache_quick_main_conf_t *cqmf, cache_api_t *api_data){
    njt_http_cache_resouce_metainfo_t       *cache_info;
    njt_pool_t                              *item_pool;
    njt_pool_cleanup_t                      *cln;
    njt_conf_t                              cf;

    item_pool = njt_create_pool(NJT_MIN_POOL_SIZE, njt_cycle->log);
    if (item_pool == NULL || NJT_OK != njt_sub_pool(njt_cycle->pool, item_pool)) {
        njt_log_error(NJT_LOG_EMERG, njt_cycle->log, 0, "create pool error in function %s", __func__);
        return NULL;
    }

    cache_info = njt_pcalloc(item_pool, sizeof(njt_http_cache_resouce_metainfo_t));
    if(cache_info == NULL){
        return NULL;
    }

    cache_info->ssl_enable = 1;
#if (NJT_OPENSSL)
    njt_str_set(&cache_info->ssl.ssl_ciphers, "DEFAULT");
    cache_info->ssl.ssl_protocols = (NJT_CONF_BITMASK_SET | NJT_SSL_TLSv1 | NJT_SSL_TLSv1_1 | NJT_SSL_TLSv1_2);
    cache_info->ssl.ssl = njt_pcalloc(item_pool, sizeof(njt_ssl_t));
    if(cache_info->ssl.ssl == NULL){
        return NULL;
    }
    cache_info->ssl.ssl->log = njt_cycle->log;
    if (njt_ssl_create(cache_info->ssl.ssl, cache_info->ssl.ssl_protocols, NULL)
        != NJT_OK)
    {
        return NULL;
    }
#if (NJT_HAVE_NTLS)
    if (1 == cache_info->ssl.ntls_enable) {
        SSL_CTX_set_ssl_version(cache_info->ssl.ssl->ctx, NTLS_method());
        SSL_CTX_set_cipher_list(cache_info->ssl.ssl->ctx,(const char *)cache_info->ssl.ssl_ciphers.data);
        SSL_CTX_enable_ntls(cache_info->ssl.ssl->ctx);
    }
#endif

    cf.pool = item_pool;
    cf.log = njt_cycle->log;
    cf.cycle = (njt_cycle_t *)njt_cycle;


    cln = njt_pool_cleanup_add(cf.pool, 0);
    if (cln == NULL) {
        njt_ssl_cleanup_ctx(cache_info->ssl.ssl);
        return NULL;
    }

    cln->handler = njt_ssl_cleanup_ctx;
    cln->data = cache_info->ssl.ssl;
    if (njt_ssl_ciphers(&cf, cache_info->ssl.ssl, &cache_info->ssl.ssl_ciphers, 0)
        != NJT_OK)
    {
        return NULL;
    }
#endif

    cache_info->item_pool = item_pool;

    if(api_data->is_addr_port_set){
        cache_info->addr_port.data = njt_pstrdup(item_pool, &api_data->addr_port);
        cache_info->addr_port.len = api_data->addr_port.len;
    }
    if(api_data->is_server_name_set){
        cache_info->server_name.data = njt_pstrdup(item_pool, &api_data->server_name);
        cache_info->server_name.len = api_data->server_name.len;
    }
    if(api_data->is_location_rule_set){
        cache_info->location_rule.data = njt_pstrdup(item_pool, &api_data->location_rule);
        cache_info->location_rule.len = api_data->location_rule.len;
    }
    if(api_data->is_location_name_set){
        cache_info->location_name.data = njt_pstrdup(item_pool, &api_data->location_name);
        cache_info->location_name.len = api_data->location_name.len;
    }

    if(api_data->is_location_body_set){
        cache_info->location_body.data = njt_pstrdup(item_pool, &api_data->location_body);
        cache_info->location_body.len = api_data->location_body.len;
    }

    if(api_data->is_proxy_pass_set){
        cache_info->proxy_pass.data = njt_pstrdup(item_pool, &api_data->proxy_pass);
        cache_info->proxy_pass.len = api_data->proxy_pass.len;
    }

    njt_queue_insert_tail(&cqmf->caches, &cache_info->cache_item);

    return cache_info;
}


static void njt_http_cache_quick_flush_confs(njt_http_cache_quick_main_conf_t *cqmf) {
    njt_pool_t  *pool;
    njt_str_t   *msg;
    njt_str_t   key = njt_string(HTTP_CACHE_QUICK_CONFS);

    pool = njt_create_pool(NJT_MIN_POOL_SIZE, njt_cycle->log);
    if (pool == NULL) {
        njt_log_error(NJT_LOG_EMERG, njt_cycle->log, 0, 
            " cache quick create pool error in function %s", __func__);
        return;
    }

    msg = njt_http_caches_to_json(pool, cqmf);
    if (msg == NULL || msg->len == 0) {
        goto end;
    }

    njt_dyn_kv_set(&key, msg);

    end:
    njt_destroy_pool(pool);
}

static njt_peer_connection_t *njt_http_cache_quick_create_download_peer(
        njt_pool_t *pool, njt_http_cache_resouce_metainfo_t *cache_info){
    njt_url_t                   u;
    u_char                      *p;
    njt_peer_connection_t       *peer;
    njt_str_t                   host, port;

    p = (u_char *)njt_strstr(cache_info->addr_port.data,":");
    if(p == NULL) {
        njt_log_error(NJT_LOG_EMERG, njt_cycle->log, 0,
                "cache quick addr_port format error, url:%V",
                &cache_info->addr_port);

        return NULL;
    }

    host.data = cache_info->addr_port.data;
    host.len = p - cache_info->addr_port.data;

    port.data = p + 1;
    port.len = cache_info->addr_port.len - host.len - 1;

    njt_memzero(&u, sizeof(njt_url_t));
    u.url.len = cache_info->addr_port.len;
    u.url.data = njt_pstrdup(pool, &cache_info->addr_port);
    
    // njt_str_set(&tmp_str, "192.168.40.136:80");
    // u.url.len = tmp_str.len;
    // u.url.data = njt_pstrdup(pool, &tmp_str); 

    // u.no_resolve = 1;
    if (njt_parse_url(pool, &u) != NJT_OK) {
        if (u.err) {
            njt_log_error(NJT_LOG_EMERG, njt_cycle->log, 0,
                    "cache quick, parse url err:%s  url:\"%V\"", u.err, &u.url);
        }

        return NULL;
    }

    // if (njt_inet_resolve_host(pool, &u) != NJT_OK) {
    //     if (u.err) {
    //         njt_log_error(NJT_LOG_EMERG, njt_cycle->log, 0,
    //                 "cache quick resolve url error:%s url:%V",
    //                 u.err, &cache_info->addr_port);
    //     }

    //     return NULL;
    // }

    // n = u.naddrs;
    peer = njt_pcalloc(pool, sizeof(njt_peer_connection_t));
    if (peer == NULL) {
        njt_log_error(NJT_LOG_EMERG, njt_cycle->log, 0,
                "cache quick malloc peer error, url:%V",
                &cache_info->addr_port);

        return NULL;
    }

    peer->sockaddr = u.addrs[0].sockaddr;
    peer->socklen = u.addrs[0].socklen;
    peer->name = &u.addrs[0].name;
    njt_inet_set_port(peer->sockaddr, njt_atoi(port.data, port.len));

    return peer;
}



static void
njt_http_cache_quick_update_download_status_str(njt_http_cache_resouce_metainfo_t  *cache_info,
        cache_api_status_t status){
    njt_str_t           tmp_str;

    cache_info->status = status;
    switch(cache_info->status)
    {
    case CACHE_API_STATUS_NONE:
        njt_str_set(&tmp_str, "init status");
        break;
    case CACHE_API_STATUS_ADD_LOC_OK:
        njt_str_set(&tmp_str, "add dyn location ok");
        break;
    case CACHE_API_STATUS_OK:
        njt_str_set(&tmp_str, "download ok");
        break;
    case CACHE_API_STATUS_ERROR:
        njt_str_set(&tmp_str, "has error");
        break;
    default:
        njt_str_set(&tmp_str, "init status");
        break;
    }

    cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &tmp_str);
    cache_info->status_str.len = tmp_str.len;  
}

static njt_int_t
njt_http_cache_quick_update_download_status(njt_http_cache_quick_download_peer_t *cq_peer,
        cache_api_status_t status) {
    njt_int_t                           rc;
    njt_lvlhsh_query_t                  lhq;
    njt_http_cache_quick_main_conf_t   *cqmf;
    njt_http_cache_resouce_metainfo_t  *cache_info;

    cqmf = (njt_http_cache_quick_main_conf_t *)njt_get_conf(njt_cycle->conf_ctx, njt_http_cache_quick_module);   
    if(cqmf == NULL){
        if (cq_peer->peer->connection) {
            njt_http_close_connection(cq_peer->peer->connection);
        }
        return NJT_OK;
    }

    lhq.key = cq_peer->crc32_str;
    lhq.key_hash = njt_murmur_hash2(lhq.key.data, lhq.key.len);
    lhq.proto = &njt_http_cache_quick_lvlhsh_proto;
    lhq.pool = cqmf->cache_pool;
    if(NJT_OK == njt_lvlhsh_find(&cqmf->resource_to_metainfo, &lhq)){
        cache_info = lhq.value;
        njt_http_cache_quick_update_download_status_str(cache_info, status);
        rc = NJT_OK;
    }else{
        njt_log_error(NJT_LOG_DEBUG, njt_cycle->log, 0,
            "update download status, not find cache info, crc32:%V", &cq_peer->crc32_str);
        
        rc = NJT_ERROR;
    }


    if (cq_peer->peer->connection) {
        njt_http_close_connection(cq_peer->peer->connection);
    }

    return rc;
}


static void
njt_http_cache_quicky_dummy_handler(njt_event_t *ev) {
    njt_log_debug0(NJT_LOG_DEBUG_EVENT, ev->log, 0,
                   "http cache_quick dummy handler");
}


static void njt_http_cache_quick_update_download_process(njt_uint_t total_flag,
        njt_http_cache_quick_download_peer_t *cq_peer, ssize_t n){
    njt_lvlhsh_query_t                  lhq;
    njt_http_cache_quick_main_conf_t   *cqmf;
    njt_http_cache_resouce_metainfo_t  *cache_info;

    cqmf = (njt_http_cache_quick_main_conf_t *)njt_get_conf(njt_cycle->conf_ctx, njt_http_cache_quick_module);   
    if(cqmf == NULL){
        return;
    }

    lhq.key = cq_peer->crc32_str;
    lhq.key_hash = njt_murmur_hash2(lhq.key.data, lhq.key.len);
    lhq.proto = &njt_http_cache_quick_lvlhsh_proto;
    lhq.pool = cqmf->cache_pool;
    if(NJT_OK == njt_lvlhsh_find(&cqmf->resource_to_metainfo, &lhq)){
        cache_info = lhq.value;
        if(total_flag){
            cache_info->resource_size = n;
            njt_log_error(NJT_LOG_INFO, njt_cycle->log, 0,
                "update download process, file size:%d", n);
        }else{
            cache_info->current_size += n;
            // njt_log_error(NJT_LOG_INFO, njt_cycle->log, 0,
            //     "update download process, read_size:%d  current_size:%d  file_size:%d",
            //     n, cache_info->current_size, cache_info->resource_size);
        }

        return;
    }else{
        njt_log_error(NJT_LOG_DEBUG, njt_cycle->log, 0,
            "update download process, not find cache info, crc32:%V", &cq_peer->crc32_str);
        
        return;
    }

    return;    
}


static njt_int_t
njt_cache_quick_http_process_body(njt_http_cache_quick_download_peer_t *cq_peer) {
    njt_cache_quick_http_parse_t    *hp;
    njt_buf_t                       *b;

    hp = cq_peer->parser;
    b = cq_peer->recv_buf;

    njt_http_cache_quick_update_download_process(0, cq_peer, b->last - b->pos);
    b->pos = b->last;

    if (hp->done) {
        return NJT_DONE;
    }
    return NJT_OK;
}



static njt_int_t
njt_cache_quick_http_parse_header_line(njt_http_cache_quick_download_peer_t *cq_peer) {
    u_char                           c, ch, *p;
    njt_cache_quick_http_parse_t    *hp;
    njt_buf_t                       *b;

    enum {
        sw_start = 0,
        sw_name,
        sw_space_before_value,
        sw_value,
        sw_space_after_value,
        sw_almost_done,
        sw_header_almost_done
    } state;

    b = cq_peer->recv_buf;
    hp = cq_peer->parser;
    state = hp->state;

    for (p = b->pos; p < b->last; p++) {
        ch = *p;

        switch (state) {

            /* first char */
            case sw_start:

                switch (ch) {
                    case CR:
                        hp->header_end = p;
                        state = sw_header_almost_done;
                        break;
                    case LF:
                        hp->header_end = p;
                        goto header_done;
                    default:
                        state = sw_name;
                        hp->header_name_start = p;

                        c = (u_char) (ch | 0x20);
                        if (c >= 'a' && c <= 'z') {
                            break;
                        }

                        if (ch >= '0' && ch <= '9') {
                            break;
                        }

                        return NJT_ERROR;
                }
                break;

                /* header name */
            case sw_name:
                c = (u_char) (ch | 0x20);
                if (c >= 'a' && c <= 'z') {
                    break;
                }

                if (ch == ':') {
                    hp->header_name_end = p;
                    state = sw_space_before_value;
                    break;
                }

                if (ch == '-') {
                    break;
                }

                if (ch >= '0' && ch <= '9') {
                    break;
                }

                if (ch == CR) {
                    hp->header_name_end = p;
                    hp->header_start = p;
                    hp->header_end = p;
                    state = sw_almost_done;
                    break;
                }

                if (ch == LF) {
                    hp->header_name_end = p;
                    hp->header_start = p;
                    hp->header_end = p;
                    goto done;
                }

                return NJT_ERROR;

                /* space* before header value */
            case sw_space_before_value:
                switch (ch) {
                    case ' ':
                        break;
                    case CR:
                        hp->header_start = p;
                        hp->header_end = p;
                        state = sw_almost_done;
                        break;
                    case LF:
                        hp->header_start = p;
                        hp->header_end = p;
                        goto done;
                    default:
                        hp->header_start = p;
                        state = sw_value;
                        break;
                }
                break;

                /* header value */
            case sw_value:
                switch (ch) {
                    case ' ':
                        hp->header_end = p;
                        state = sw_space_after_value;
                        break;
                    case CR:
                        hp->header_end = p;
                        state = sw_almost_done;
                        break;
                    case LF:
                        hp->header_end = p;
                        goto done;
                }
                break;

                /* space* before end of header line */
            case sw_space_after_value:
                switch (ch) {
                    case ' ':
                        break;
                    case CR:
                        state = sw_almost_done;
                        break;
                    case LF:
                        goto done;
                    default:
                        state = sw_value;
                        break;
                }
                break;

                /* end of header line */
            case sw_almost_done:
                switch (ch) {
                    case LF:
                        goto done;
                    default:
                        return NJT_ERROR;
                }

                /* end of header */
            case sw_header_almost_done:
                switch (ch) {
                    case LF:
                        goto header_done;
                    default:
                        return NJT_ERROR;
                }
        }
    }

    b->pos = p;
    hp->state = state;

    return NJT_AGAIN;

    done:

    b->pos = p + 1;
    hp->state = sw_start;

    return NJT_OK;

    header_done:

    b->pos = p + 1;
    hp->state = sw_start;
    hp->body_start = b->pos;

    return NJT_DONE;
}



static njt_int_t
njt_cache_quick_http_process_headers(njt_http_cache_quick_download_peer_t *cq_peer) {
    njt_int_t                       rc;
    njt_table_elt_t                 *h;
    njt_cache_quick_http_parse_t    *hp;

    njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0, "http process header.");

    hp = cq_peer->parser;

    if (hp->headers.size == 0) {
        rc = njt_array_init(&hp->headers, cq_peer->pool, 4,
                            sizeof(njt_table_elt_t));
        if (rc != NJT_OK) {
            return NJT_ERROR;
        }
    }

    for (;;) {
        rc = njt_cache_quick_http_parse_header_line(cq_peer);
        if (rc == NJT_OK) {
            h = njt_array_push(&hp->headers);
            if (h == NULL) {
                return NJT_ERROR;
            }

            njt_memzero(h, sizeof(njt_table_elt_t));
            h->hash = 1;
            h->key.data = hp->header_name_start;
            h->key.len = hp->header_name_end - hp->header_name_start;

            h->value.data = hp->header_start;
            h->value.len = hp->header_end - hp->header_start;

            njt_log_debug4(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                           "http header \"%*s: %*s\"",
                           h->key.len, h->key.data, h->value.len,
                           h->value.data);

            if (h->key.len == njt_strlen("Transfer-Encoding")
                && h->value.len == njt_strlen("chunked")
                && njt_strncasecmp(h->key.data, (u_char *) "Transfer-Encoding",
                                   h->key.len) == 0
                && njt_strncasecmp(h->value.data, (u_char *) "chunked",
                                   h->value.len) == 0) {
                hp->chunked = 1;
            }

            if (h->key.len == njt_strlen("Content-Length")
                && njt_strncasecmp(h->key.data, (u_char *) "Content-Length",
                                   h->key.len) == 0) {
                hp->content_length_n = njt_atoof(h->value.data, h->value.len);

                if (hp->content_length_n == NJT_ERROR) {

                    njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                                   "invalid fetch content length");
                    return NJT_ERROR;
                }

                //update file size
                njt_http_cache_quick_update_download_process(1, cq_peer, hp->content_length_n);
            }

            continue;
        }

        if (rc == NJT_DONE) {
            break;
        }

        if (rc == NJT_AGAIN) {
            return NJT_AGAIN;
        }

        /*http header parse error*/
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0,
                       "http process header error.");
        return NJT_ERROR;
    }

    /*TODO check if the first buffer is used out*/
    hp->stage = NJT_HTTP_CACHE_QUICK_PARSE_BODY;
    hp->process = njt_cache_quick_http_process_body;

    return hp->process(cq_peer);
}


/*We assume the status line and headers are located in one buffer*/
static njt_int_t
njt_cache_quick_http_parse_status_line(njt_http_cache_quick_download_peer_t *cq_peer) {
    u_char ch;
    u_char *p;
    njt_cache_quick_http_parse_t *hp;
    njt_buf_t *b;

    enum {
        sw_start = 0,
        sw_H,
        sw_HT,
        sw_HTT,
        sw_HTTP,
        sw_first_major_digit,
        sw_major_digit,
        sw_first_minor_digit,
        sw_minor_digit,
        sw_status,
        sw_space_after_status,
        sw_status_text,
        sw_almost_done
    } state;

    hp = cq_peer->parser;
    b = cq_peer->recv_buf;
    state = hp->state;

    for (p = b->pos; p < b->last; p++) {
        ch = *p;

        switch (state) {

            /* "HTTP/" */
            case sw_start:
                switch (ch) {
                    case 'H':
                        state = sw_H;
                        break;
                    default:
                        return NJT_ERROR;
                }
                break;

            case sw_H:
                switch (ch) {
                    case 'T':
                        state = sw_HT;
                        break;
                    default:
                        return NJT_ERROR;
                }
                break;

            case sw_HT:
                switch (ch) {
                    case 'T':
                        state = sw_HTT;
                        break;
                    default:
                        return NJT_ERROR;
                }
                break;

            case sw_HTT:
                switch (ch) {
                    case 'P':
                        state = sw_HTTP;
                        break;
                    default:
                        return NJT_ERROR;
                }
                break;

            case sw_HTTP:
                switch (ch) {
                    case '/':
                        state = sw_first_major_digit;
                        break;
                    default:
                        return NJT_ERROR;
                }
                break;

                /* the first digit of major HTTP version */
            case sw_first_major_digit:
                if (ch < '1' || ch > '9') {
                    return NJT_ERROR;
                }

                state = sw_major_digit;
                break;

                /* the major HTTP version or dot */
            case sw_major_digit:
                if (ch == '.') {
                    state = sw_first_minor_digit;
                    break;
                }

                if (ch < '0' || ch > '9') {
                    return NJT_ERROR;
                }

                break;

                /* the first digit of minor HTTP version */
            case sw_first_minor_digit:
                if (ch < '0' || ch > '9') {
                    return NJT_ERROR;
                }

                state = sw_minor_digit;
                break;

                /* the minor HTTP version or the end of the request line */
            case sw_minor_digit:
                if (ch == ' ') {
                    state = sw_status;
                    break;
                }

                if (ch < '0' || ch > '9') {
                    return NJT_ERROR;
                }

                break;

                /* HTTP status code */
            case sw_status:
                if (ch == ' ') {
                    break;
                }

                if (ch < '0' || ch > '9') {
                    return NJT_ERROR;
                }

                hp->code = hp->code * 10 + (ch - '0');

                if (++hp->count == 3) {
                    state = sw_space_after_status;
                }

                break;

                /* space or end of line */
            case sw_space_after_status:
                switch (ch) {
                    case ' ':
                        state = sw_status_text;
                        break;
                    case '.':                    /* IIS may send 403.1, 403.2, etc */
                        state = sw_status_text;
                        break;
                    case CR:
                        break;
                    case LF:
                        goto done;
                    default:
                        return NJT_ERROR;
                }
                break;

                /* any text until end of line */
            case sw_status_text:
                switch (ch) {
                    case CR:
                        hp->status_text_end = p;
                        state = sw_almost_done;
                        break;
                    case LF:
                        hp->status_text_end = p;
                        goto done;
                }

                if (hp->status_text == NULL) {
                    hp->status_text = p;
                }

                break;

                /* end of status line */
            case sw_almost_done:
                switch (ch) {
                    case LF:
                        goto done;
                    default:
                        return NJT_ERROR;
                }
        }
    }

    b->pos = p;
    hp->state = state;

    return NJT_AGAIN;

    done:

    b->pos = p + 1;
    hp->state = sw_start;

    /*begin to process headers*/

    hp->stage = NJT_HTTP_CACHE_QUICK_PARSE_HEADER;
    hp->process = njt_cache_quick_http_process_headers;

    hp->process(cq_peer);

    return NJT_OK;
}




static njt_int_t
njt_http_cache_quick_http_read_handler(njt_event_t *rev) {
    njt_connection_t                        *c;
    njt_http_cache_quick_download_peer_t    *cq_peer;
    ssize_t                                 n, size;
    njt_buf_t                               *b;
    njt_int_t                               rc;
    njt_chain_t                             *chain, *node;
    njt_cache_quick_http_parse_t            *hp;

    c = rev->data;
    cq_peer = c->data;

    njt_log_debug0(NJT_LOG_DEBUG_HTTP, c->log, 0, "cache quick recv.");

    /*Init the internal parser*/
    if (cq_peer->parser == NULL) {
        hp = njt_pcalloc(cq_peer->pool, sizeof(njt_cache_quick_http_parse_t));
        if (hp == NULL) {
            /*log*/
            njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                           "memory allocation error for cache_quick.");
            return NJT_ERROR;
        }

        hp->stage = NJT_HTTP_CACHE_QUICK_PARSE_STATUS_LINE;
        hp->process = njt_cache_quick_http_parse_status_line;

        cq_peer->parser = hp;
    }

    for (;;) {
        if (cq_peer->recv_buf == NULL) {
            b = njt_create_temp_buf(cq_peer->pool, njt_pagesize);
            if (b == NULL) {
                /*log*/
                njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                               "recv buffer memory allocation error for cache_quick.");
                return NJT_ERROR;
            }
            cq_peer->recv_buf = b;
        }

        b = cq_peer->recv_buf;
        size = b->end - b->last;

        n = c->recv(c, b->last, size);

        if (n > 0) {
            b->last += n;
            hp = cq_peer->parser;
            // if(NJT_HTTP_CACHE_QUICK_PARSE_BODY == hp->stage){
            //     njt_http_cache_quick_update_download_process(0, cq_peer, b->last - b->pos);
            //     b->pos = b->last;
            // }
            rc = hp->process(cq_peer);
            if (rc == NJT_ERROR) {
                return NJT_ERROR;
            }

            /*link chain buffer*/
            if (b->last == b->end) {
                hp = cq_peer->parser;
                if (hp->stage != NJT_HTTP_CACHE_QUICK_PARSE_BODY) {
                    /*log. The status and headers are too large to be hold in one buffer*/
                    njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                                   "status and headers exceed one page size");
                    return NJT_ERROR;
                }

                chain = njt_alloc_chain_link(cq_peer->pool);
                if (chain == NULL) {
                    /*log and process the error*/
                    njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                                   "memory allocation of the chain buf failed.");
                    return NJT_ERROR;
                }

                chain->buf = b;
                chain->next = NULL;

                node = cq_peer->recv_chain;
                if (node == NULL) {
                    cq_peer->recv_chain = chain;
                } else {
                    cq_peer->last_chain_node->next = chain;
                }
                cq_peer->last_chain_node = chain;

                /*Reset the recv buffer*/
                cq_peer->recv_buf = NULL;
            }

            continue;
        }

        if (n == NJT_AGAIN) {
            if (njt_handle_read_event(rev, 0) != NJT_OK) {
                /*log*/
                njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                               "read event handle error for cache_quick");
                return NJT_ERROR;
            }
            return NJT_AGAIN;
        }

        if (n == NJT_ERROR) {
            /*log*/
            njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                           "read error for cache_quick");
            return NJT_ERROR;
        }

        break;
    }


    hp = cq_peer->parser;
    hp->done = 1;
    rc = hp->process(cq_peer);

    if (rc == NJT_DONE) {
        return NJT_DONE;
    }

    if (rc == NJT_AGAIN) {
        /* the connection is shutdown*/
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                       "connection is shutdown");
        return NJT_ERROR;
    }

    return NJT_OK;
}


static njt_int_t
njt_http_cache_quick_http_write_handler(njt_event_t *wev) {
    njt_connection_t                        *c;
    njt_http_cache_quick_download_peer_t    *cq_peer;
    ssize_t                                 n, size;

    c = wev->data;
    cq_peer = c->data;

    njt_log_error(NJT_LOG_INFO, c->log, 0, "http check send.");

    if (cq_peer->send_buf == NULL) {
        cq_peer->send_buf = njt_create_temp_buf(cq_peer->pool, njt_pagesize);
        if (cq_peer->send_buf == NULL) {
            /*log the send buf allocation failure*/
            njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                           "malloc failure of the send buffer for cache quicky");
            return NJT_ERROR;
        }
        /*Fill in the buff*/
        cq_peer->send_buf->last = njt_snprintf(cq_peer->send_buf->last,
                                               cq_peer->send_buf->end - cq_peer->send_buf->last, "GET %V HTTP/1.1" CRLF,
                                               &cq_peer->location_name);
        cq_peer->send_buf->last = njt_snprintf(cq_peer->send_buf->last,
                                               cq_peer->send_buf->end - cq_peer->send_buf->last,
                                               "Connection: close" CRLF);
        cq_peer->send_buf->last = njt_snprintf(cq_peer->send_buf->last,
                                               cq_peer->send_buf->end - cq_peer->send_buf->last, "Host: %V" CRLF,
                                               &cq_peer->host);
        // cq_peer->send_buf->last = njt_snprintf(cq_peer->send_buf->last,
        //                                        cq_peer->send_buf->end - cq_peer->send_buf->last, "Host: 192.168.40.136" CRLF);
        cq_peer->send_buf->last = njt_snprintf(cq_peer->send_buf->last,
                                               cq_peer->send_buf->end - cq_peer->send_buf->last,
                                               "User-Agent: njet (cache-quick)" CRLF);
        cq_peer->send_buf->last = njt_snprintf(cq_peer->send_buf->last,
                                               cq_peer->send_buf->end - cq_peer->send_buf->last, CRLF);
    }

    size = cq_peer->send_buf->last - cq_peer->send_buf->pos;

    n = c->send(c, cq_peer->send_buf->pos,
                cq_peer->send_buf->last - cq_peer->send_buf->pos);
    if (n == NJT_ERROR) {
        return NJT_ERROR;
    }

    if (n > 0) {
        cq_peer->send_buf->pos += n;
        if (n == size) {
            wev->handler = njt_http_cache_quicky_dummy_handler;

            if (njt_handle_write_event(wev, 0) != NJT_OK) {
                /*LOG the failure*/
                njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                               "write event handle error for cache quicky");
                return NJT_ERROR;
            }
            return NJT_DONE;
        }
    }

    return NJT_AGAIN;
}

static void njt_http_cache_quick_download_write_handler(njt_event_t *wev) {
    njt_connection_t                        *c;
    njt_http_cache_quick_download_peer_t    *cq_peer;
    njt_int_t                               rc;
    
    c = wev->data;
    cq_peer = c->data;

    if (wev->timedout) {
        // njt_del_timer(wev);
        if (wev->timer_set) {
            njt_del_timer(wev);
        }
        /*log the case and update the peer status.*/
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                       "write action for cache_quick timeout");
        njt_http_cache_quick_update_download_status(cq_peer, CACHE_API_STATUS_ERROR);
        return;
    }

    if (wev->timer_set) {
        njt_del_timer(wev);
    }

    //handler write data
    rc = njt_http_cache_quick_http_write_handler(wev);
    if (rc == NJT_ERROR) {

        /*log the case and update the peer status.*/
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0,
                       "write action error for cache quick");
        njt_http_cache_quick_update_download_status(cq_peer, CACHE_API_STATUS_ERROR);
        return;
    } else if (rc == NJT_DONE || rc == NJT_OK) {
        return;
    } else {
        /*AGAIN*/
    }


    if (!wev->timer_set) {
        njt_add_timer(wev, 5000);
    }

    return;
}


static void njt_http_cache_quick_download_read_handler(njt_event_t *rev) {
    njt_connection_t                        *c;
    njt_http_cache_quick_download_peer_t    *cq_peer;
    njt_int_t                               rc;


    c = rev->data;
    cq_peer = c->data;

    if (rev->timedout) {
        if (rev->timer_set) {
            njt_del_timer(rev);
        }
        /*log the case and update the peer status.*/
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                       "read action for cache_quick timeout");
        njt_http_cache_quick_update_download_status(cq_peer, CACHE_API_STATUS_ERROR);
        return;
    }

    if (rev->timer_set) {
        njt_del_timer(rev);
    }

    rc = njt_http_cache_quick_http_read_handler(rev);
    if (rc == NJT_ERROR) {
        /*log the case and update the peer status.*/
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0,
                       "read action error for cache_quick");
        njt_http_cache_quick_update_download_status(cq_peer, CACHE_API_STATUS_ERROR);
        return;
    } else if (rc == NJT_DONE) {
        njt_http_cache_quick_update_download_status(cq_peer, CACHE_API_STATUS_OK);
        return;
    } else {
        /*AGAIN*/
    }
    if (!rev->timer_set) {
        njt_add_timer(rev, 5000);
    }

    return;
}

#if (NJT_HTTP_SSL)
static njt_int_t
njt_http_cache_quick_ssl_handshake(njt_connection_t *c,
            njt_http_cache_quick_download_peer_t *cq_peer,
            njt_http_cache_resouce_metainfo_t *cache_info
            ) {
    if (c->ssl->handshaked) {
        cq_peer->peer->connection->write->handler = njt_http_cache_quick_download_write_handler;
        cq_peer->peer->connection->read->handler = njt_http_cache_quick_download_read_handler;

        /*NJT_AGAIN or NJT_OK*/
        njt_http_cache_quick_download_write_handler(cq_peer->peer->connection->write);
        return NJT_OK;
    }

    if (c->write->timedout) {
        return NJT_ERROR;
    }

    return NJT_ERROR;
}

static void njt_http_cache_quick_ssl_handshake_handler(njt_connection_t *c){
    njt_http_cache_quick_download_peer_t    *cq_peer;
    njt_http_cache_resouce_metainfo_t       *cache_info;
    njt_int_t                               rc;
    njt_http_cache_quick_main_conf_t        *cqmf;

    cqmf = (njt_http_cache_quick_main_conf_t *)njt_get_conf(njt_cycle->conf_ctx, njt_http_cache_quick_module);   
    if(cqmf == NULL){
        return;
    }

    cq_peer = c->data;

    if(NJT_OK == njt_cache_quick_item_exist(cq_peer->crc32, cqmf, &cache_info)){
        rc = njt_http_cache_quick_ssl_handshake(c, cq_peer, cache_info);
        if (rc != NJT_OK) {
            /*log the case and update the peer status.*/
            njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0,
                        "read action error for cache quick ssl handshake");
            njt_http_cache_quick_update_download_status(cq_peer, CACHE_API_STATUS_ERROR);
        }
    }
}

static njt_int_t njt_http_cache_quick_ssl_init_connection(njt_connection_t *c,
        njt_http_cache_quick_download_peer_t *cq_peer,
        njt_http_cache_resouce_metainfo_t *cache_info) {
    njt_int_t rc;

    // if (njt_http_cache_quick_test_connect(c) != NJT_OK) {
    //     return NJT_ERROR;
    // }
    if (njt_ssl_create_connection(cache_info->ssl.ssl, c,
                                  NJT_SSL_BUFFER | NJT_SSL_CLIENT) != NJT_OK) {
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0, "ssl init create connection for cache_quick error ");
        return NJT_ERROR;
    }

    c->sendfile = 0;
    c->log->action = "SSL handshaking to hc";

    rc = njt_ssl_handshake(c);
    if (rc == NJT_AGAIN) {

        if (!c->write->timer_set) {
            njt_add_timer(c->write, 1000);
        }

        c->ssl->handler = njt_http_cache_quick_ssl_handshake_handler;
        return NJT_OK;
    }

    return njt_http_cache_quick_ssl_handshake(c, cq_peer, cache_info);
//    return NJT_OK;
}

#endif


static void njt_http_cache_quick_download_timer_handler(njt_event_t *ev)
{
    njt_pool_t                          *pool;
    njt_http_cache_resouce_metainfo_t   *cache_info;
    njt_str_t                           tmp_str;
    u_char                              *p;
    njt_http_cache_quick_download_peer_t *cq_peer;
    njt_int_t                           rc;

    cache_info = ev->data;

    if(ev->timer_set){
        njt_del_timer(ev);
        njt_log_error(NJT_LOG_INFO, njt_cycle->log, 0,
                      " cache quick del timer ");
    }

    if (njt_quit || njt_terminate || njt_exiting) {
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
            " cache quick download for quiting, url:%V", &cache_info->addr_port);

        return;
    }

    cache_info = ev->data;
    //connect peer
    pool = njt_create_pool(njt_pagesize, njt_cycle->log);
    if (pool == NULL) {
        cache_info->status = CACHE_API_STATUS_ERROR;
        njt_str_set(&tmp_str, "create pool error when downloading");
        cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &tmp_str);
        cache_info->status_str.len = tmp_str.len;

        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0,
            "create pool error when downloading, url:%V", &cache_info->addr_port);
        return;
    }

    cq_peer = njt_pcalloc(pool, sizeof(njt_http_cache_quick_download_peer_t));
    if (cq_peer == NULL) {
        cache_info->status = CACHE_API_STATUS_ERROR;
        njt_str_set(&tmp_str, "cache quick download pool malloc error");
        cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &tmp_str);
        cache_info->status_str.len = tmp_str.len;

        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0,
                        " cache quick download pool malloc error, url:%V", &cache_info->addr_port);
        njt_destroy_pool(pool);
        return;
    }

    cq_peer->pool = pool;
    cq_peer->peer = njt_http_cache_quick_create_download_peer(pool, cache_info);
    if(cq_peer->peer == NULL){
        cache_info->status = CACHE_API_STATUS_ERROR;
        njt_str_set(&tmp_str, "cache quick create download peer error");
        cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &tmp_str);
        cache_info->status_str.len = tmp_str.len;

        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0,
                " cache quick create download peer:%V error", &cache_info->addr_port);
        njt_destroy_pool(pool);
        return;
    }

    p = (u_char *)njt_strstr(cache_info->addr_port.data,":");
    if(p == NULL) {
        cache_info->status = CACHE_API_STATUS_ERROR;
        njt_str_set(&tmp_str, "cache quick addr_port format error");
        cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &tmp_str);
        cache_info->status_str.len = tmp_str.len;

        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0,
                " cache quick addr_port format, peer:%V", &cache_info->addr_port);
        njt_destroy_pool(pool);
        return;
    }

    cq_peer->host.len = p - cache_info->addr_port.data;
    cq_peer->host.data = njt_pstrdup(pool, &cache_info->addr_port);

    cq_peer->crc32 = cache_info->crc32;
    cq_peer->crc32_str.len = cache_info->crc32_str.len;
    cq_peer->crc32_str.data = njt_pstrdup(pool, &cache_info->crc32_str);

    cq_peer->location_name.len = cache_info->location_name.len;
    cq_peer->location_name.data = njt_pstrdup(pool, &cache_info->location_name);

    // cq_peer->peer->type = SOCK_STREAM;
    cq_peer->peer->get = njt_event_get_peer;
    cq_peer->peer->log = njt_cycle->log;
    cq_peer->peer->log_error = NJT_ERROR_ERR;

    rc = njt_event_connect_peer(cq_peer->peer);
    if (rc == NJT_ERROR || rc == NJT_DECLINED || rc == NJT_BUSY) {
        cache_info->status = CACHE_API_STATUS_ERROR;
        njt_str_set(&tmp_str, "cache quick connect to peer error");
        cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &tmp_str);
        cache_info->status_str.len = tmp_str.len;

        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0,
                " cache quick connect to peer:%V error", &cache_info->addr_port);
        njt_destroy_pool(pool);
        return;
    }

    njt_log_error(NJT_LOG_INFO, njt_cycle->log, 0,
            " cache quick connected to peer of %V, rc:%d", &cache_info->addr_port, rc);

    cq_peer->peer->connection->data = cq_peer;
    cq_peer->peer->connection->pool = cq_peer->pool;
#if (NJT_HTTP_SSL)
    if (cache_info->ssl_enable && cache_info->ssl.ssl->ctx &&
        cq_peer->peer->connection->ssl == NULL) {
        rc = njt_http_cache_quick_ssl_init_connection(cq_peer->peer->connection, cq_peer, cache_info);
        if (rc == NJT_ERROR) {
            cache_info->status = CACHE_API_STATUS_ERROR;
            njt_str_set(&tmp_str, "cache quick ssl connect init error");
            cache_info->status_str.data = njt_pstrdup(cache_info->item_pool, &tmp_str);
            cache_info->status_str.len = tmp_str.len;

            njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0,
                    " cache quick ssl connect to peer:%V error", &cache_info->addr_port);
            njt_destroy_pool(pool);
            return;
        }
        return;
    }
#endif
            njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0,
                    " =============80 connect");
    cq_peer->peer->connection->write->handler = njt_http_cache_quick_download_write_handler;
    cq_peer->peer->connection->read->handler = njt_http_cache_quick_download_read_handler;

    //send request
    njt_http_cache_quick_download_write_handler(cq_peer->peer->connection->write);

    return;
}



static njt_int_t njt_http_cache_quick_download(njt_http_cache_resouce_metainfo_t *cache_info) {
    njt_event_t                     *download_timer;

    download_timer = &cache_info->download_timer;
    download_timer->handler = njt_http_cache_quick_download_timer_handler;
    download_timer->log = njt_cycle->log;
    download_timer->data = cache_info;
    download_timer->cancelable = 1;

    njt_add_timer(download_timer, 1000);

    return NJT_OK;
}


static void njt_http_cache_quick_save(njt_http_cache_quick_main_conf_t *cqmf) {
    njt_http_cache_quick_flush_confs(cqmf);

    return;
}


static njt_int_t njt_cache_quick_download_status(njt_http_cache_quick_main_conf_t *cqmf,
        cache_api_t *api_data, njt_rpc_result_t*rpc_result) {
    njt_http_cache_resouce_metainfo_t       *cache_info;
    uint32_t                                 crc32;
    njt_lvlhsh_query_t                      lhq;
    u_char                                  buff[50];
    u_char                                  *end;
    njt_str_t                               tmp_str;
    njt_int_t                               ratio;


    //del item from local, queue and lvlhash
    crc32 = njt_cache_quick_item_crc32(api_data);
    
    end = njt_snprintf(buff, 50, "%d", crc32);
    lhq.key.data = buff;
    lhq.key.len = end - buff;
    lhq.key_hash = njt_murmur_hash2(lhq.key.data, lhq.key.len);
    lhq.proto = &njt_http_cache_quick_lvlhsh_proto;
    lhq.pool = cqmf->cache_pool;

    if(NJT_OK != njt_lvlhsh_find(&cqmf->resource_to_metainfo, &lhq)){
        if(api_data->is_location_name_set){
            njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
                " cache quick item:%V is not exist", &api_data->location_name);
        }

        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)"cache quick item is not exist");

        return NJT_ERROR;  
    }

    cache_info = lhq.value;
    if(cache_info->status != CACHE_API_STATUS_ERROR){
        if(cache_info->current_size > 0){
            ratio = (cache_info->resource_size * 1.0) / (cache_info->current_size * 1.0) * 100;
            end = njt_snprintf(buff, 50, DOWNLOAD_STATUS_INFO, ratio);
        }else{
            end = njt_snprintf(buff, 50, DOWNLOAD_STATUS_INFO, 0);
        }
        
        tmp_str.data = buff;
        tmp_str.len = end - buff;
        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_SUCCESS);
        njt_rpc_result_set_msg2(rpc_result, &tmp_str);
    }else{
        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)" downlodd error");

        return NJT_ERROR; 
    }
    
    return NJT_OK;
}



static njt_int_t njt_cache_quick_del_item(njt_http_cache_quick_main_conf_t *cqmf,
        cache_api_t *api_data, njt_str_t *json_str, njt_rpc_result_t *rpc_result) {
    uint32_t                                 crc32;
    njt_http_cache_resouce_metainfo_t       *cache_info;

    //del item from local, queue and lvlhash
    crc32 = njt_cache_quick_item_crc32(api_data);

    //find from lvlhash
    if(NJT_OK != njt_cache_quick_item_exist(crc32, cqmf, &cache_info)){
        if(api_data->is_location_name_set){
            njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
                " cache quick item:%V is not exist", &api_data->location_name);
        }

        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR_INPUT_PARAM);
        njt_rpc_result_set_msg(rpc_result, (u_char *)"cache quick item is not exist");

        return NJT_ERROR;
    }

    //del dyn locaton
    if(NJT_OK != njt_http_cache_item_del_dyn_location(cache_info, json_str, cqmf)){
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
            " cache quick del dyn location error");

        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)"cache quick del dyn location error");

        // return NJT_ERROR;
    }

    //first delete from lvlhash
    if(NJT_OK != njt_http_del_cache_item_from_lvlhash(crc32, cqmf)){
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
            " del cache info from lvlhash error");

        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)"del cache info from lvlhash error");

        return NJT_ERROR; 
    }

    //delete from queue
    if(NJT_OK != njt_http_del_cache_item_from_queue(cqmf, api_data)){
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
            " del cache info from queue error");

        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)"del cache info from queue error");

        return NJT_ERROR;  
    }

    //kv_set refresh
    njt_http_cache_quick_save(cqmf);

    //todo purge cache info
    
    return NJT_OK;
}


static njt_int_t njt_cache_quick_add_item(njt_http_cache_quick_main_conf_t *cqmf,
            cache_api_t *api_data, njt_rpc_result_t*rpc_result) {
    njt_http_cache_resouce_metainfo_t       *cache_info;
    uint32_t                                 crc32;

    //check param, location body must has proxy_cache directive
    if(!api_data->is_location_body_set){
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
            " cache quick item:%V is already exist", &api_data->location_name);

        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR_INPUT_PARAM);
        njt_rpc_result_set_msg(rpc_result, (u_char *)"cache quick item location_body must set");

        return NJT_ERROR;
    }

    if(njt_strstr(api_data->location_body.data,"proxy_cache ") == NULL) {
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
            " cache quick item:%V is already exist", &api_data->location_name);

        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR_INPUT_PARAM);
        njt_rpc_result_set_msg(rpc_result, (u_char *)"cache quick item location_body must has proxy_cache");

        return NJT_ERROR;
    }

    //add item to local, queue and lvlhash
    crc32 = njt_cache_quick_item_crc32(api_data);

    //find from lvlhash
    if(NJT_OK == njt_cache_quick_item_exist(crc32, cqmf, &cache_info)){
        if(api_data->is_location_name_set){
            njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
                " cache quick item:%V is already exist", &api_data->location_name);
        }

        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR_INPUT_PARAM);
        njt_rpc_result_set_msg(rpc_result, (u_char *)"cache quick item is already exist");

        return NJT_ERROR;
    }

    //insert queue
    cache_info = njt_http_add_cache_item_to_queue(cqmf, api_data);
    if(cache_info == NULL){
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
            " malloc cache info error");

        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR_MEM_ALLOC);
        njt_rpc_result_set_msg(rpc_result, (u_char *)"malloc cache info error");

        return NJT_ERROR;
    }


    //insert lvlhash
    if(NJT_OK != njt_http_add_cache_item_to_lvlhash(crc32, cache_info, cqmf)){
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
            " add cache info to lvlhash error");

        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)"add cache info to lvlhash error");

        return NJT_ERROR;  
    }

    //add dyn locaton
    if(NJT_OK != njt_http_cache_item_add_dyn_location(cache_info, cqmf)){
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
            " cache quick add dyn location error");

        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)"cache quick add dyn location error");

        return NJT_ERROR;
    }

    //kv_set save and create download event
    njt_http_cache_quick_save(cqmf);

    njt_http_cache_quick_download(cache_info);
    
    return NJT_OK;
}


static njt_int_t njt_http_cache_quick_conf_out_handler(cache_api_type_t cache_api_type,
            njt_http_request_t *r, njt_rpc_result_t *rpc_result) {
    njt_buf_t       *buf;
    njt_chain_t     out;
    njt_int_t       rc;
    njt_str_t       tmp_str;


    switch (rpc_result->code) {
        case NJT_RPC_RSP_SUCCESS:
            r->headers_out.status = NJT_HTTP_OK;
            break;
        case NJT_RPC_RSP_ERR_MEM_ALLOC:
        case NJT_RPC_RSP_ERR:
            r->headers_out.status = NJT_HTTP_INTERNAL_SERVER_ERROR;
            break;
        case NJT_RPC_RSP_ERR_JSON:
            r->headers_out.status = NJT_HTTP_BAD_REQUEST;
            break;
        default:
            r->headers_out.status = NJT_HTTP_INTERNAL_SERVER_ERROR;
            break;
    }

    njt_rpc_result_to_json_str(rpc_result, &tmp_str);
    buf = njt_create_temp_buf(r->pool, tmp_str.len);
    if (buf == NULL) {
        njt_log_debug1(NJT_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "could not alloc buffer in function %s", __func__);
        return NJT_ERROR;
    }

    njt_memcpy(buf->last, tmp_str.data, tmp_str.len);
    buf->last += tmp_str.len;

    njt_str_t type = njt_string("application/json");
    r->headers_out.content_type = type;
    r->headers_out.content_length_n = buf->last - buf->pos;
    if (r->headers_out.content_length) {
        r->headers_out.content_length->hash = 0;
        r->headers_out.content_length = NULL;
    }
    rc = njt_http_send_header(r);

    // r->header_only  when method is HEAD ,header_only is set.
    if (rc == NJT_ERROR || rc > NJT_OK) {
        return rc;
    }

    buf->last_buf = 1;
    out.buf = buf;
    out.next = NULL;
    return njt_http_output_filter(r, &out);
}


static void njt_http_cache_quick_api_read_data(njt_http_request_t *r){
    njt_str_t                   json_str;
    njt_chain_t                 *body_chain, *tmp_chain;
    njt_int_t                   rc = NJT_OK;
    cache_api_t                 *api_data = NULL;
    njt_uint_t                  len, size;
    js2c_parse_error_t          err_info;
    njt_str_t                   *del_json_str;
    njt_rpc_result_t            *rpc_result = NULL;
    njt_http_cache_quick_main_conf_t *cqmf;


    rpc_result = njt_rpc_result_create();
    if(!rpc_result){
        njt_log_error(NJT_LOG_ERR, r->connection->log, 0, 
                " cache quick create rpc_result error");
        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)" cache quick create rpc_result error");

        rc = NJT_ERROR;
        goto end;
    }

    njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_SUCCESS);

    body_chain = r->request_body->bufs;
    /*check the sanity of the json body*/
    if(NULL == body_chain){
        njt_log_error(NJT_LOG_ERR, r->connection->log, 0, 
                " cache quick input body error");
        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)" input body error");

        rc = NJT_ERROR;
        goto out;
    }

    len = 0;
    tmp_chain = body_chain;
    while (tmp_chain != NULL) {
        len += tmp_chain->buf->last - tmp_chain->buf->pos;
        tmp_chain = tmp_chain->next;
    }

    json_str.len = len;
    json_str.data = njt_pcalloc(r->pool, len);
    if (json_str.data == NULL) {
        njt_log_error(NJT_LOG_ERR, r->connection->log, 0, 
                " cache quick malloc error");
        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR_MEM_ALLOC);
        njt_rpc_result_set_msg(rpc_result, (u_char *)" malloc error");

        rc = NJT_ERROR;
        goto out;
    }

    len = 0;
    tmp_chain = r->request_body->bufs;
    while (tmp_chain != NULL) {
        size = tmp_chain->buf->last - tmp_chain->buf->pos;
        njt_memcpy(json_str.data + len, tmp_chain->buf->pos, size);
        tmp_chain = tmp_chain->next;
        len += size;
    }

    api_data = json_parse_cache_api(r->pool, &json_str, &err_info);
    if(api_data == NULL){
        njt_log_error(NJT_LOG_ERR, r->connection->log, 0, 
            " cache quick json parse error:%V", &err_info.err_str);

        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR_JSON);
        njt_rpc_result_set_msg2(rpc_result, &err_info.err_str);

        rc = NJT_ERROR;
        goto out;
    }

    njt_http_set_ctx(r, api_data, njt_http_cache_quick_module);
    cqmf = (njt_http_cache_quick_main_conf_t *)njt_get_conf(njt_cycle->conf_ctx, njt_http_cache_quick_module);   
    if(cqmf == NULL){
        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)" module main config is null");

        rc = NJT_ERROR;
        goto out;
    }

    switch (api_data->type)
    {
    case CACHE_API_TYPE_ADD:
        rc = njt_cache_quick_add_item(cqmf, api_data, rpc_result);
        if (rc == NJT_OK) {
            goto out;
        }
        break;
    
    case CACHE_API_TYPE_DEL:
        api_data->is_location_body_set = 0;
        api_data->is_proxy_pass_set = 0;
        del_json_str = to_json_cache_api(r->pool, api_data, OMIT_NULL_ARRAY | OMIT_NULL_OBJ | OMIT_NULL_STR);
        rc = njt_cache_quick_del_item(cqmf, api_data, del_json_str, rpc_result);
        if (rc == NJT_OK) {
            goto out;
        }
        break;
    
    case CACHE_API_TYPE_DOWNLOAD_STATUS:
        rc = njt_cache_quick_download_status(cqmf, api_data, rpc_result);
        if (rc == NJT_OK) {
            goto out;
        }
        break;

    default:
        njt_log_error(NJT_LOG_ERR, r->connection->log, 0,
            " cache put type error:%d", api_data->type);

        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR_JSON);
        njt_rpc_result_set_msg(rpc_result, (u_char *)" cache put type error");

        rc = NJT_ERROR;
        goto out;
    }

out:
    rc = njt_http_cache_quick_conf_out_handler(api_data->type, r, rpc_result);

end:
    if(rpc_result){
        njt_rpc_result_destroy(rpc_result);
        rpc_result = NULL;
    }
    njt_http_finalize_request(r, rc);
}


static njt_int_t
njt_http_cache_quick_handler(njt_http_request_t *r) {
    njt_int_t                       rc = NJT_OK;
    njt_http_cache_quick_loc_conf_t *cqlcf;
    njt_http_cache_quick_main_conf_t *cqmf;
    njt_rpc_result_t                *rpc_result = NULL;

    cqmf = (njt_http_cache_quick_main_conf_t *)njt_get_conf(njt_cycle->conf_ctx, njt_http_cache_quick_module);   
    if(cqmf == NULL){
        return NJT_DECLINED;
    }

    cqlcf = njt_http_get_module_loc_conf(r, njt_http_cache_quick_module);
    if(cqlcf == NULL  || cqlcf->cache_quick_enable == NJT_CONF_UNSET || cqlcf->cache_quick_enable == 0){
        return NJT_DECLINED;
    }

    rpc_result = njt_rpc_result_create();
    if(!rpc_result){
        njt_log_error(NJT_LOG_ERR, r->connection->log, 0, 
                " cache quick create rpc_result error");
        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)" cache quick create rpc_result error");

        rc = NJT_ERROR;
        goto out_handler;
    }
    njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_SUCCESS);

    if (r->method == NJT_HTTP_GET) {
        njt_http_get_caches(r, cqmf, rpc_result);
    } else if (r->method == NJT_HTTP_PUT) {
        rc = njt_http_read_client_request_body(r, njt_http_cache_quick_api_read_data);

        if (rc == NJT_ERROR || rc >= NJT_HTTP_SPECIAL_RESPONSE) {
            if(rpc_result != NULL){
                njt_rpc_result_destroy(rpc_result);
                rpc_result = NULL;
            }
            return rc;
        }

        if(rpc_result != NULL){
            njt_rpc_result_destroy(rpc_result);
            rpc_result = NULL;
        }
        return NJT_DONE;
    } else {
        njt_rpc_result_set_code(rpc_result, NJT_RPC_RSP_ERR);
        njt_rpc_result_set_msg(rpc_result, (u_char *)" method not allowed");
    }

out_handler:
    rc = njt_http_cache_quick_conf_out_handler(0, r, rpc_result);

    if(rpc_result != NULL){
        njt_rpc_result_destroy(rpc_result);
    }

    return rc;
}


static void njt_cache_quick_item_transter(cache_caches_item_t *item, cache_api_t *api_data){
    if(item->is_addr_port_set){
        set_cache_api_addr_port(api_data, get_cache_caches_item_addr_port(item));
    }

    if(item->is_server_name_set){
        set_cache_api_server_name(api_data, get_cache_caches_item_server_name(item));
    }

    if(item->is_location_rule_set){
        set_cache_api_location_rule(api_data, get_cache_caches_item_location_rule(item));
    }

    if(item->is_location_name_set){
        set_cache_api_location_name(api_data, get_cache_caches_item_location_name(item));
    }

    if(item->is_location_body_set){
        set_cache_api_location_body(api_data, get_cache_caches_item_location_body(item));
    }

    if(item->is_proxy_pass_set){
        set_cache_api_proxy_pass(api_data, get_cache_caches_item_proxy_pass(item));
    }
}

static void njt_http_cache_quick_recovery_confs(njt_http_cache_quick_main_conf_t *cqmf, njt_str_t *msg){
    njt_pool_t                     *pool;
    njt_uint_t                      i;
    cache_t                        *caches;    
    cache_caches_item_t            *item;
    uint32_t                        crc32;
    cache_api_t                     api_data, *p_api_data;
    js2c_parse_error_t              err_info;
    njt_http_cache_resouce_metainfo_t       *cache_info;


    pool = njt_create_pool(NJT_MIN_POOL_SIZE, njt_cycle->log);
    if (pool == NULL) {
        njt_log_error(NJT_LOG_EMERG, njt_cycle->log, 0, "create pool error in function %s", __func__);
        goto end;
    }

    njt_log_error(NJT_LOG_INFO, pool->log, 0, 
                " cache quick recover confs msg: %V",  msg);

    caches = json_parse_cache(pool, msg, &err_info);
    if (caches == NULL || !caches->is_caches_set)
    {
        njt_log_error(NJT_LOG_ERR, pool->log, 0, 
                " cache quick recover confs json parse err: %V",  &err_info.err_str);

        goto end;
    }

    if (!caches->is_caches_set)
    {
        njt_log_error(NJT_LOG_INFO, pool->log, 0, 
                " cache quick recover confs json size 0");

        goto end;
    }

    for (i = 0; i < caches->caches->nelts; ++i) {
        item = get_cache_caches_item(caches->caches, i);
        if(item == NULL){
            continue;
        }

        njt_memzero(&api_data, sizeof(cache_api_t));
        p_api_data = &api_data;
        njt_cache_quick_item_transter(item, p_api_data);
       
        //add item to local, queue and lvlhash
        crc32 = njt_cache_quick_item_crc32(p_api_data);

        //find from lvlhash
        if(NJT_OK == njt_cache_quick_item_exist(crc32, cqmf, &cache_info)){
            if(p_api_data->is_location_name_set){
                njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
                    " cache quick item:%V is already exist", &p_api_data->location_name);
            }

            continue;
        }

        //insert queue
        cache_info = njt_http_add_cache_item_to_queue(cqmf, p_api_data);
        if(cache_info == NULL){
            njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
                " malloc cache info error");

            continue;
        }

        //insert lvlhash
        if(NJT_OK != njt_http_add_cache_item_to_lvlhash(crc32, cache_info, cqmf)){
            njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0, 
                " add cache info to lvlhash error");

            continue;  
        }

        //create download event
        // njt_http_cache_quick_download(cache_info);
    }

end:
    if(pool != NULL){
        njt_destroy_pool(pool);
    }
}


static njt_int_t
njt_http_cache_quick_init_worker(njt_cycle_t *cycle) {
    njt_http_cache_quick_main_conf_t   *cqmf;
    njt_str_t                          key = njt_string(HTTP_CACHE_QUICK_CONFS);
    njt_str_t                          msg;

    cqmf = (njt_http_cache_quick_main_conf_t *)njt_get_conf(cycle->conf_ctx, njt_http_cache_quick_module);   
    if(cqmf == NULL){
        return NJT_OK;
    }

    //recover all config
    njt_memzero(&msg, sizeof(njt_str_t));
    njt_dyn_kv_get(&key, &msg);
    if (msg.len > 2) {
        njt_http_cache_quick_recovery_confs(cqmf, &msg);
    }

    return NJT_OK;
}

