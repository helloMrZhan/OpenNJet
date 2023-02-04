/*************************************************************************************
 Copyright (C), 2021-2023, TMLake(Beijing) Technology Ltd.,
 File name    : njt_http_health_check_helper.c
 Version      : 1.0
 Author       : ChengXu
 Date         : 2023/2/1/001 
 Description  : 
 Other        :
 History      :
 <author>       <time>          <version >      <desc>
 ChengXu        2023/2/1/001       1.1             
***********************************************************************************/
//
// Created by Administrator on 2023/2/1/001.
//
#include <njt_config.h>
#include <njt_core.h>
#include <njt_http.h>
#include <njet.h>
#include <njt_stream.h>

#include "njt_http_match_module.h"
#include "njt_common_health_check.h"

#include <njt_http_upstream.h>


#define NJT_HTTP_SERVER_PORT        5688
#define NJT_HTTP_HC_INTERVAL        5000
#define NJT_HTTP_HC_CONNECT_TIMEOUT 5000


#define NJT_HTTP_HC_TCP                   0x0001
#define NJT_HTTP_HC_HTTP                  0x0002
#define NJT_HTTP_HC_GRPC                  0x0003


/**
 *This module provided directive: health_check url interval=100 timeout=300 port=8080 type=TCP.
 **/

extern njt_module_t njt_http_proxy_module;

typedef struct njt_http_health_check_peer_s njt_http_health_check_peer_t;
typedef struct njt_health_check_http_parse_s njt_health_check_http_parse_t;


/*Type of callbacks of the checker*/
typedef njt_int_t (*njt_http_health_check_init_pt)
        (njt_http_upstream_rr_peer_t *peer);

typedef njt_int_t (*njt_http_health_check_process_pt)
        (njt_http_health_check_peer_t *peer);

typedef njt_int_t (*njt_http_health_check_event_handler)
        (njt_event_t *ev);

typedef njt_int_t (*njt_http_health_check_update_pt)
        (njt_http_health_check_peer_t *hc_peer, njt_int_t status);

typedef struct {
    njt_uint_t type;
    njt_uint_t protocol;
    njt_str_t name;
    njt_flag_t one_side;
    /* HTTP */
    njt_http_health_check_event_handler write_handler;
    njt_http_health_check_event_handler read_handler;

    njt_http_health_check_init_pt init;
    njt_http_health_check_process_pt process;
    njt_http_health_check_update_pt update;

} njt_health_checker_t;


typedef struct njt_http_health_check_main_conf_s {
    njt_array_t health_checks;
} njt_http_health_check_main_conf_t;


typedef struct njt_http_health_check_loc_conf_s {
    njt_str_t *upstream;
    njt_msec_t interval;
    njt_msec_t jitter;
    njt_msec_t timeout;
    njt_uint_t protocol;
    njt_health_checker_t *checker;
    njt_uint_t port;
    njt_str_t uri;
    njt_http_match_t *match;
    njt_uint_t passes;
    njt_uint_t fails;
    njt_event_t hc_timer;
    //njt_conf_t                            *cf;
//    njt_http_proxy_loc_conf_t *plcf;
    void *pglcf;//njt_http_grpc_loc_conf_t
    njt_str_t gsvc;
    njt_uint_t gstatus;
    unsigned persistent: 1;
    unsigned mandatory: 1;
    njt_uint_t type;
    njt_uint_t curr_delay;
    njt_uint_t curr_frame;
    njt_uint_t test_val;
} njt_http_health_check_loc_conf_t;


#define NJT_HTTP_PARSE_INIT           0
#define NJT_HTTP_PARSE_STATUS_LINE    1
#define NJT_HTTP_PARSE_HEADER         2
#define NJT_HTTP_PARSE_BODY           4

#define GRPC_STATUS_CODE_OK                  0
#define GRPC_STATUS_CODE_UNIMPLEMENTED       12
#define GRPC_STATUS_CODE_INVALID             ((njt_uint_t)-1)

/*Structure used for holding http parser internal info*/
struct njt_health_check_http_parse_s {
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

    njt_int_t (*process)(njt_http_health_check_peer_t *hc_peer);

    njt_msec_t start;
};

/*Main structure of the hc information of a peer*/
struct njt_http_health_check_peer_s {
    njt_uint_t peer_id;
    njt_helper_health_check_conf_t *hhccf;
    njt_pool_t *pool;
    njt_peer_connection_t peer;
    njt_buf_t *send_buf;
    njt_buf_t *recv_buf;
    njt_chain_t *recv_chain;
    njt_chain_t *last_chain_node;
    void *parser;
#if (NJT_HTTP_SSL)
    njt_str_t ssl_name;
#endif
};


typedef enum {
    njt_http_grpc_hc_st_start = 0,
    njt_http_grpc_hc_st_sent,
    njt_http_grpc_hc_st_end,
} njt_http_grpc_hc_state_e;

typedef struct njt_http_health_check_conf_ctx_s {
//    njt_http_grpc_loc_conf_t *pglcf;//njt_http_grpc_loc_conf_t
    njt_health_checker_t *checker;
    njt_http_match_t *match;
    njt_str_t uri;
    njt_str_t gsvc;
    njt_uint_t gstatus;
    njt_http_upstream_srv_conf_t* upstream;
} njt_http_health_check_conf_ctx_t;


static char *njt_http_health_check_conf(njt_conf_t *cf, njt_command_t *cmd, void *conf);

static njt_int_t njt_http_health_check_conf_handler(njt_http_request_t *r);

static njt_int_t njt_http_health_check_add(njt_helper_health_check_conf_t *hhccf);

static void njt_http_health_check_timer_handler(njt_event_t *ev);

static njt_int_t
njt_http_health_check_common_update(njt_http_health_check_peer_t *hc_peer,
                                    njt_int_t status);


/*Common framework functions of all types of checkers*/
static njt_int_t njt_http_health_check_update_status(
        njt_http_health_check_peer_t *hc_peer, njt_int_t status);

/*update the peer's status without lock*/
static njt_int_t
njt_http_health_check_update_wo_lock(njt_helper_health_check_conf_t *hhccf,
                                     njt_http_health_check_peer_t *hc_peer,
                                     njt_http_upstream_rr_peer_t *peer,
                                     njt_int_t status);

static void njt_http_health_check_write_handler(njt_event_t *event);

static void njt_http_health_check_read_handler(njt_event_t *event);

static njt_health_checker_t *njt_http_get_health_check_type(njt_str_t *str);


/*TCP type of checker related functions.*/
static njt_int_t njt_http_health_check_peek_one_byte(njt_connection_t *c);

static njt_int_t njt_http_health_check_tcp_handler(njt_event_t *wev);


/*HTTP type of checker related functions*/
static njt_int_t njt_http_health_check_http_write_handler(njt_event_t *event);

static njt_int_t njt_http_health_check_http_read_handler(njt_event_t *event);

static njt_int_t
njt_http_health_check_http_update_status(njt_http_health_check_peer_t *hc_peer,
                                         njt_int_t status);

static njt_int_t njt_http_health_check_http_process(
        njt_http_health_check_peer_t *hc_peer);

static njt_int_t njt_health_check_http_parse_status_line(
        njt_http_health_check_peer_t *hc_peer);

static njt_int_t njt_health_check_http_parse_header_line(
        njt_http_health_check_peer_t *hc_peer);

static njt_int_t njt_health_check_http_process_headers(
        njt_http_health_check_peer_t *hc_peer);

static njt_int_t njt_health_check_http_process_body(
        njt_http_health_check_peer_t *hc_peer);

static void njt_http_hc_grpc_loop_peer(njt_helper_health_check_conf_t *hhccf, njt_http_upstream_rr_peer_t *peer);

void *njt_http_grpc_hc_get_up_uptream(void *pglcf);

njt_array_t *njt_http_grpc_hc_get_lengths(void *pglcf);

njt_shm_zone_t *njt_http_grpc_hc_get_shm_zone(void *pglcf);

void njt_http_upstream_handler(njt_event_t *ev);

void njt_http_upstream_send_request_handler(njt_http_request_t *r, njt_http_upstream_t *u);

void njt_http_upstream_process_header(njt_http_request_t *r, njt_http_upstream_t *u);

void njt_http_grpc_hc_set_upstream(njt_http_upstream_t *u);

njt_int_t njt_http_grpc_body_output_filter(void *data, njt_chain_t *in);

njt_int_t njt_http_grpc_create_request(njt_http_request_t *r);

static void njt_http_grpc_hc_handler(njt_event_t *wev);

void *njt_http_grpc_hc_create_in_filter_ctx(njt_pool_t *pool, void *r, void *grpc_con);

void *njt_http_grpc_hc_create_grpc_on(njt_pool_t *pool);

void *njt_http_grpc_hc_get_uptream(void *pglcf);

static njt_int_t njt_hc_helper_module_init(njt_cycle_t *cycle);
static njt_http_match_t *njt_helper_http_match_create(njt_helper_health_check_conf_t *hhccf);
extern njt_module_t njt_http_grpc_module;

typedef struct njt_http_match_ctx_s {
    njt_http_match_t *match;
} njt_http_match_ctx_t;

typedef njt_uint_t (*njt_http_match_check_handler)
        (njt_http_match_t *match, njt_uint_t code, njt_array_t *headers,
         njt_buf_t *body);

typedef struct njt_http_match_main_conf_s {
    /*array of njt_http_match_t*/
    njt_array_t matches;
} njt_http_match_main_conf_t;


static njt_int_t njt_http_match_block(njt_helper_hc_api_data_t *api_data,njt_helper_health_check_conf_t *hhccf);

static njt_int_t njt_http_match(njt_helper_hc_api_data_t *api_data,njt_helper_health_check_conf_t *hhccf);



//static njt_str_t njt_http_grpc_hc_svc = njt_string("/grpc.health.v1.Health/Check");

#if (NJT_OPENSSL)
static njt_json_define_t njt_helper_hc_api_data_ssl_json_dt[] ={
        {
                njt_string("enable"),
                offsetof(njt_helper_hc_ssl_add_data_t, ssl_enable),
                NJT_JSON_BOOL,
                NULL,
                NULL,
        },
        {
                njt_string("sessionReuse"),
                offsetof(njt_helper_hc_ssl_add_data_t, ssl_session_reuse),
                NJT_JSON_BOOL,
                NULL,
                NULL,
        },
        {
                njt_string("protocols"),
                offsetof(njt_helper_hc_ssl_add_data_t, ssl_protocols),
                NJT_JSON_STR,
                NULL,
                njt_json_parse_ssl_protocols,
        },
        {
                njt_string("ciphers"),
                offsetof(njt_helper_hc_ssl_add_data_t, ssl_ciphers),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("name"),
                offsetof(njt_helper_hc_ssl_add_data_t, ssl_name),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("server_name"),
                offsetof(njt_helper_hc_ssl_add_data_t, ssl_name),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("verify"),
                offsetof(njt_helper_hc_ssl_add_data_t, ssl_verify),
                NJT_JSON_BOOL,
                NULL,
                NULL,
        },
        {
                njt_string("verify_depth"),
                offsetof(njt_helper_hc_ssl_add_data_t, ssl_verify_depth),
                NJT_JSON_INT,
                NULL,
                NULL,
        },
        {
                njt_string("trusted_certificate"),
                offsetof(njt_helper_hc_ssl_add_data_t, ssl_trusted_certificate),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("crl"),
                offsetof(njt_helper_hc_ssl_add_data_t, ssl_crl),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("certificate"),
                offsetof(njt_helper_hc_ssl_add_data_t, ssl_certificate),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("certificate_key"),
                offsetof(njt_helper_hc_ssl_add_data_t, ssl_certificate_key),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("passwords"),
                offsetof(njt_helper_hc_ssl_add_data_t, ssl_passwords),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("conf_commands"),
                offsetof(njt_helper_hc_ssl_add_data_t, ssl_conf_commands),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        njt_json_define_null,
};
#endif
static njt_json_define_t njt_helper_hc_api_data_http_json_dt[] ={
        {
                njt_string("uri"),
                offsetof(njt_helper_hc_http_add_data_t, uri),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("header"),
                offsetof(njt_helper_hc_http_add_data_t, headers),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("body"),
                offsetof(njt_helper_hc_http_add_data_t, body),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("status"),
                offsetof(njt_helper_hc_http_add_data_t, status),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("grpc_service"),
                offsetof(njt_helper_hc_http_add_data_t, grpc_service),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("grpc_status"),
                offsetof(njt_helper_hc_http_add_data_t, grpc_status),
                NJT_JSON_INT,
                NULL,
                NULL,
        },
        njt_json_define_null,
};
static njt_json_define_t njt_helper_hc_api_data_stream_json_dt[] ={
        {
                njt_string("send"),
                offsetof(njt_helper_hc_stream_add_data_t, send),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("expect"),
                offsetof(njt_helper_hc_stream_add_data_t, expect),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        njt_json_define_null,
};

static njt_json_define_t njt_helper_hc_api_data_json_dt[] = {
        {
                njt_string("upstream"),
                offsetof(njt_helper_hc_api_data_t, upstream_name),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("type"),
                offsetof(njt_helper_hc_api_data_t, hc_type),
                NJT_JSON_STR,
                NULL,
                NULL,
        },
        {
                njt_string("interval"),
                offsetof(njt_helper_hc_api_data_t, interval),
                NJT_JSON_STR,
                NULL,
                njt_json_parse_msec,
        },
        {
                njt_string("jitter"),
                offsetof(njt_helper_hc_api_data_t, jitter),
                NJT_JSON_STR,
                NULL,
                njt_json_parse_msec,
        },
        {
                njt_string("timeout"),
                offsetof(njt_helper_hc_api_data_t, timeout),
                NJT_JSON_STR,
                NULL,
                njt_json_parse_msec,
        },
        {
                njt_string("port"),
                offsetof(njt_helper_hc_api_data_t, port),
                NJT_JSON_INT,
                NULL,
                NULL,
        },
        {
                njt_string("passes"),
                offsetof(njt_helper_hc_api_data_t, passes),
                NJT_JSON_INT,
                NULL,
                NULL,
        },
        {
                njt_string("fails"),
                offsetof(njt_helper_hc_api_data_t, fails),
                NJT_JSON_INT,
                NULL,
                NULL,
        },
        {
                njt_string("persistent"),
                offsetof(njt_helper_hc_api_data_t, persistent),
                NJT_JSON_BOOL,
                NULL,
                NULL,
        },
        {
                njt_string("mandatory"),
                offsetof(njt_helper_hc_api_data_t, mandatory),
                NJT_JSON_BOOL,
                NULL,
                NULL,
        },
        {
                njt_string("http"),
                offsetof(njt_helper_hc_api_data_t, http),
                NJT_JSON_OBJ,
                njt_helper_hc_api_data_http_json_dt,
                NULL,
        },
        {
                njt_string("stream"),
                offsetof(njt_helper_hc_api_data_t, stream),
                NJT_JSON_OBJ,
                njt_helper_hc_api_data_stream_json_dt,
                NULL,
        },
        {
                njt_string("ssl"),
                offsetof(njt_helper_hc_api_data_t, ssl),
                NJT_JSON_OBJ,
                njt_helper_hc_api_data_ssl_json_dt,
                NULL,
        },
        njt_json_define_null,
};



static njt_command_t njt_http_health_check_helper_commands[] = {
        {
                njt_string("health_check_api"),
                NJT_HTTP_LOC_CONF | NJT_CONF_NOARGS,
                njt_http_health_check_conf,
                0,
                0,
                NULL
        },
        njt_null_command
};

static njt_http_module_t njt_http_health_check_helper_ctx = {
        NULL,                                   /* preconfiguration */
        NULL,                                   /* postconfiguration */

        NULL,                                   /* create main configuration */
        NULL,                                  /* init main configuration */

        NULL,                                  /* create server configuration */
        NULL,                                  /* merge server configuration */

        NULL,                                   /* create location configuration */
        NULL                                    /* merge location configuration */
};

njt_module_t njt_http_health_check_helper = {
        NJT_MODULE_V1,
        &njt_http_health_check_helper_ctx,      /* module context */
        njt_http_health_check_helper_commands,  /* module directives */
        NJT_HTTP_MODULE,                        /* module type */
        NULL,                                   /* init master */
        njt_hc_helper_module_init,              /* init module */
        NULL,                                   /* init process */
        NULL,                                   /* init thread */
        NULL,                                   /* exit thread */
        NULL,                                   /* exit process */
        NULL,                                   /* exit master */
        NJT_MODULE_V1_PADDING
};

static njt_int_t njt_hc_helper_module_init(njt_cycle_t *cycle) {
    njt_helper_main_conf_t *hmcf;
    hmcf = njt_pcalloc(cycle->pool, sizeof(njt_helper_main_conf_t));
    if (hmcf == NULL) {
        njt_log_error(NJT_LOG_EMERG, cycle->log, 0, "health check helper alloc main conf error ");
        return NJT_ERROR;
    }
    njt_queue_init(&hmcf->hc_queue);
    cycle->conf_ctx[njt_http_health_check_helper.index] = (void*)hmcf;
    return NJT_OK;
}

static char *njt_http_health_check_conf(njt_conf_t *cf, njt_command_t *cmd, void *conf) {
    njt_http_core_loc_conf_t *clcf;
    clcf = njt_http_conf_get_module_loc_conf(cf, njt_http_core_module);
    clcf->handler = njt_http_health_check_conf_handler;
    return NJT_CONF_OK;
}


/**
 * 它读取请求体并将其解析为 njt_helper_hc_api_data_t 结构
 *
 * @param r http 请求对象
 */
static void njt_http_hc_api_read_data(njt_http_request_t *r) {
    njt_str_t json_str;
    njt_chain_t *body_chain;
    njt_int_t rc;
    njt_helper_hc_api_data_t *api_data = NULL;

    body_chain = r->request_body->bufs;
    if (body_chain && body_chain->next) {
        api_data->success = 0;
        api_data->rc= NJT_OK;
        return;
    }
    /*check the sanity of the json body*/
    json_str.data = body_chain->buf->pos;
    json_str.len = body_chain->buf->last - body_chain->buf->pos;

    api_data = njt_pcalloc(r->pool,sizeof (njt_helper_hc_api_data_t));
    if(api_data == NULL){
        njt_log_debug1(NJT_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "could not alloc buffer in function %s", __func__);
        return;
    }

    rc =njt_json_parse_data(r->pool,&json_str,njt_helper_hc_api_data_json_dt,api_data);
    api_data->rc = rc;
    if(rc != NJT_OK ){
        api_data->success = 0;
    }else {
        api_data->success = 1;
    }
    njt_http_set_ctx(r, api_data, njt_http_health_check_helper);
}

/**
 * > 按名称查找upstream配置
 *
 * @param cycle 当前cycle。
 * @param name upstream的名称。
 *
 * @return njt_http_upstream_srv_conf_t
 */
static njt_http_upstream_srv_conf_t* njt_http_find_upstream_by_name(njt_cycle_t *cycle,njt_str_t *name){
    njt_http_upstream_main_conf_t  *umcf;
    njt_http_upstream_srv_conf_t   **uscfp;
    njt_uint_t i;

    umcf = njt_http_cycle_get_module_main_conf(cycle, njt_http_upstream_module);
    uscfp = umcf->upstreams.elts;

    for (i = 0; i < umcf->upstreams.nelts; i++) {
        if (uscfp[i]->host.len != name->len
            || njt_strncasecmp(uscfp[i]->host.data, name->data, name->len) != 0) {
            continue;
        }
        return uscfp[i];
    }
    return NULL;
}



static njt_int_t njy_hc_api_data2_ssl_cf(njt_helper_hc_api_data_t *api_data,njt_helper_health_check_conf_t *hhccf){

    hhccf->ssl.ssl_enable = api_data->ssl.ssl_enable?1:0;
    hhccf->ssl.ssl_session_reuse = api_data->ssl.ssl_session_reuse?1:0;
    hhccf->ssl.ssl_protocols = api_data->ssl.ssl_protocols==0?
            (NJT_CONF_BITMASK_SET|NJT_SSL_TLSv1|NJT_SSL_TLSv1_1|NJT_SSL_TLSv1_2):api_data->ssl.ssl_protocols;
    if(api_data->ssl.ssl_ciphers.len <= 0){
        njt_str_set(&hhccf->ssl.ssl_ciphers,"DEFAULT");
    }else{
        njt_str_copy_pool(hhccf->pool,hhccf->ssl.ssl_ciphers,api_data->ssl.ssl_ciphers,return NJT_ERROR);
    }
    njt_str_copy_pool(hhccf->pool,hhccf->ssl.ssl_name,api_data->ssl.ssl_name,return NJT_ERROR);
    hhccf->ssl.ssl_server_name = api_data->ssl.ssl_server_name?1:0;
    hhccf->ssl.ssl_verify = api_data->ssl.ssl_verify?1:0;
    hhccf->ssl.ssl_verify_depth = api_data->ssl.ssl_verify_depth;
    njt_str_copy_pool(hhccf->pool,hhccf->ssl.ssl_trusted_certificate,api_data->ssl.ssl_trusted_certificate,return NJT_ERROR);
    njt_str_copy_pool(hhccf->pool,hhccf->ssl.ssl_crl,api_data->ssl.ssl_crl,return NJT_ERROR);
    njt_str_copy_pool(hhccf->pool,hhccf->ssl.ssl_certificate,api_data->ssl.ssl_certificate,return NJT_ERROR);
    njt_str_copy_pool(hhccf->pool,hhccf->ssl.ssl_certificate_key,api_data->ssl.ssl_certificate_key,return NJT_ERROR);
    //todo
//    njt_array_t *ssl_passwords;
//    njt_array_t *ssl_conf_commands;

    hhccf->ssl.ssl = njt_pcalloc(hhccf->pool, sizeof(njt_ssl_t));
    if ( hhccf->ssl.ssl == NULL) {
        return NJT_ERROR;
    }
    hhccf->ssl.ssl->log = hhccf->log;
    njt_helper_hc_set_ssl(hhccf,&hhccf->ssl);
    return NJT_OK;
}


static njt_int_t njt_hc_api_data2_common_cf(njt_helper_hc_api_data_t *api_data,njt_helper_health_check_conf_t *hhccf){
    njt_http_health_check_conf_ctx_t *hhccc;
    njt_int_t rc;

    hhccf->upstream_name.data = njt_pcalloc(hhccf->pool,api_data->upstream_name.len);
    if(hhccf == NULL){
        njt_log_error(NJT_LOG_EMERG, hhccf->log, 0, "health check helper create dynamic pool error ");
        return NJT_ERROR;
    }
    njt_memcpy(hhccf->upstream_name.data,api_data->upstream_name.data,api_data->upstream_name.len);
    hhccf->upstream_name.len = api_data->upstream_name.len;

    hhccf->interval = api_data->interval==0?NJT_HTTP_HC_INTERVAL:api_data->interval ;
    hhccf->jitter = api_data->jitter==0?0:api_data->jitter ;
    hhccf->timeout = api_data->timeout==0?NJT_HTTP_HC_CONNECT_TIMEOUT:api_data->timeout;
    hhccf->port = api_data->port;
    hhccf->passes = api_data->passes==0?1:api_data->passes;
    hhccf->fails = api_data->fails==0?1:api_data->fails;
    if(hhccf->type == NJT_HTTP_MODULE){
        hhccc = hhccf->ctx;
        njt_str_copy_pool(hhccf->pool, hhccc->uri,api_data->http.uri,return NJT_ERROR);
        rc = njt_http_match_block(api_data,hhccf);
        if(rc != NJT_OK){
            return rc;
        }
        //todo grpc  void *pglcf;//njt_http_grpc_loc_conf_t gsvc gstatus
    }
    if(hhccf->type == NJT_STREAM_MODULE){
        //todo stream
    }
    njy_hc_api_data2_ssl_cf(api_data,hhccf);
    //todo ssl
    return NJT_OK;
}


static njt_int_t njt_http_api_add_conf(njt_http_request_t *r,njt_helper_hc_api_data_t *api_data){
    njt_health_checker_t *checker;
    njt_http_upstream_srv_conf_t* uscf;
    njt_helper_health_check_conf_t *hhccf;
    njt_cycle_t *cycle = (njt_cycle_t *)njt_cycle;
    njt_pool_t *hc_pool;
    njt_http_health_check_conf_ctx_t *hhccc;
    njt_int_t rc;

    if(api_data->hc_type.len == 0 || api_data->upstream_name.len == 0 ){
        njt_log_error(NJT_LOG_ERR, r->connection->log, 0," type and upstream must be set !!");
        return NJT_ERROR;
    }

    hc_pool = njt_create_dynamic_pool(NJT_MIN_POOL_SIZE, cycle->log);
    if(hc_pool == NULL) {
        njt_log_error(NJT_LOG_EMERG, cycle->log, 0, "health check helper create dynamic pool error ");
        goto err;
    }
    njt_sub_pool(cycle->pool,hc_pool);
    hhccf = njt_pcalloc(hc_pool, sizeof(njt_helper_health_check_conf_t));
    if( hhccf == NULL){
        njt_log_error(NJT_LOG_EMERG, cycle->log, 0, "health check helper alloc hhccf mem error");
        goto err;
    }
    njt_queue_init(&hhccf->queue);
    hhccf->pool = hc_pool;
    hhccf->log = cycle->log;
    checker = njt_http_get_health_check_type(&api_data->hc_type);
    hhccf->type = checker->type;
    hhccf->protocol = checker->protocol;
    if(checker->type == NJT_HTTP_MODULE){
        uscf = njt_http_find_upstream_by_name(cycle,&api_data->upstream_name);
        if(uscf == NULL ){
            njt_log_error(NJT_LOG_ERR, r->connection->log, 0,"not find http upstream: %V",&api_data->upstream_name);
            goto err;
        }
        if (uscf->shm_zone == NULL) {
            njt_log_error(NJT_LOG_EMERG,  r->connection->log, 0,
                               "helper health check requires upstream \"%V\" to be in shared memory.",
                               &uscf->host);
            return NJT_ERROR;
        }
        hhccc =  njt_pcalloc(hc_pool, sizeof(njt_http_health_check_conf_ctx_t));
        if( hhccf == NULL){
            njt_log_error(NJT_LOG_EMERG, cycle->log, 0, "health check helper alloc hhccc error ");
            goto err;
        }
        hhccc->upstream = uscf;
        hhccc->checker = checker;
        hhccf->ctx = hhccc;
        njt_http_health_check_add(hhccf);
    }
    rc = njt_hc_api_data2_common_cf(api_data,hhccf);
    if( rc != NJT_OK ){
        goto err;
    }
    return NJT_OK;

err:
    if(hc_pool){
        njt_destroy_pool(hc_pool);
    }
    return NJT_ERROR;
}

static njt_int_t njt_http_health_check_conf_handler(njt_http_request_t *r) {
    njt_int_t rc = NJT_OK;
    njt_chain_t out;
    njt_str_t insert;
    njt_buf_t *buf;
    njt_helper_hc_api_data_t *api_data = NULL;

    out.next = NULL;
    out.buf = NULL;

    rc = njt_http_read_client_request_body(r, njt_http_hc_api_read_data);
    if (rc == NJT_OK) {
        njt_http_finalize_request(r, NJT_DONE);
    }
    api_data = njt_http_get_module_ctx(r, njt_http_health_check_helper);
    if (api_data == NULL || !api_data->success) {
        rc = NJT_ERROR;
        goto out;
    }
    //put (delete location)
    if (r->method == NJT_HTTP_PUT) {
        // todo 删除
    } else if (r->method == NJT_HTTP_POST) {
        rc = njt_http_api_add_conf(r,api_data);
    } else {
        rc = NJT_ERROR;
        goto out;
    }

    out:

    if (rc == NJT_OK) {
        njt_str_set(&insert, "ok");
    } else {
        njt_str_set(&insert, "error");
    }

    r->headers_out.content_type_len = sizeof("text/plain") - 1;
    njt_str_set(&r->headers_out.content_type, "text/plain");
    r->headers_out.content_type_lowcase = NULL;
    r->headers_out.status = NJT_HTTP_OK;
    buf = njt_create_temp_buf(r->pool,insert.len);
    if(buf == NULL){
        njt_log_debug1(NJT_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "could not alloc buffer in function %s", __func__);
        return NJT_ERROR;
    }
    njt_memcpy(buf->pos,insert.data, insert.len);
    buf->last = buf->pos + insert.len;
    out.buf = buf;
    out.next = NULL;
    r->headers_out.content_length_n = insert.len;
    if (r->headers_out.content_length) {
        r->headers_out.content_length->hash = 0;
        r->headers_out.content_length = NULL;
    }
    rc = njt_http_send_header(r);
    return njt_http_output_filter(r, &out);
}

static njt_int_t njt_http_match_block(njt_helper_hc_api_data_t *api_data,njt_helper_health_check_conf_t *hhccf) {
    njt_http_match_t *match;

    njt_http_health_check_conf_ctx_t *hhccc = hhccf->ctx;

    match = njt_helper_http_match_create( hhccf);
    if (match == NULL) {
        njt_log_error(NJT_LOG_EMERG, hhccf->log, 0, "match create error");
        return NJT_ERROR;
    }
    hhccc->match = match;
    match->defined = 1;
    if (njt_array_init(&match->status.codes, hhccf->pool, 4,
                       sizeof(njt_http_match_code_t)) != NJT_OK) {
        njt_log_error(NJT_LOG_EMERG, hhccf->log, 0, "status code array init error");
        return NJT_ERROR;
    }
    if (njt_array_init(&match->headers, hhccf->pool, 4,
                       sizeof(njt_http_match_header_t)) != NJT_OK) {
        njt_log_error(NJT_LOG_EMERG, hhccf->log, 0, "match header array init error");
        return NJT_ERROR;
    }
    njt_http_match(api_data,hhccf);
    return NJT_OK;
}

static njt_int_t njt_http_match_parse_code(njt_str_t *code,njt_http_match_code_t *match_code) {
    u_char *dash, *last;
    njt_uint_t status;

    last = code->data + code->len;
    dash = njt_strlchr(code->data, last, '-');

    if (dash) {

        status = njt_atoi(code->data, dash - code->data);
        if (status < 100 || status >= 600) {
            return NJT_ERROR;
        }
        match_code->code = status;

        status = njt_atoi(dash + 1, last - dash - 1);
        if (status < 100 || status >= 600) {
            return NJT_ERROR;
        }
        match_code->last_code = status;

        if (match_code->last_code < match_code->code) {
            return NJT_ERROR;
        }

        match_code->single = 0;

    } else {

        status = njt_atoi(code->data, code->len);
        if (status < 100 || status >= 600) {
            return NJT_ERROR;
        }
        match_code->code = status;
        match_code->single = 1;
    }
    return NJT_OK;

}

static njt_regex_t *
njt_http_match_regex_value(njt_conf_t *cf, njt_str_t *regex) {
#if (NJT_PCRE)
    njt_regex_compile_t rc;
    u_char errstr[NJT_MAX_CONF_ERRSTR];

    njt_memzero(&rc, sizeof(njt_regex_compile_t));

    rc.pattern = *regex;
    rc.err.len = NJT_MAX_CONF_ERRSTR;
    rc.err.data = errstr;
    rc.pool = cf->pool;
    rc.options = NJT_REGEX_CASELESS;


    if (njt_regex_compile(&rc) != NJT_OK) {
        return NULL;
    }

    return rc.regex;

#else

    njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                       "using regex \"%V\" requires PCRE library",
                       regex);
    return NULL;

#endif
}

//todo 未完成 解析期望header
static njt_int_t njt_http_match_parse_dheaders(njt_str_t *args,njt_http_match_t *match,njt_helper_health_check_conf_t *hhccf){
    njt_uint_t nelts;
    njt_http_match_header_t *header;
    njt_uint_t i;
    njt_conf_t cf;

    i=0;
    nelts = args->len;
    header = njt_array_push(&match->headers);
    if (header == NULL) {
        njt_log_error(NJT_LOG_EMERG, hhccf->log, 0, "header array push error.");
        return NJT_ERROR;
    }
    njt_memzero(header, sizeof(njt_http_match_header_t));

    i++;
    /*header ! abc;*/
    if (njt_strncmp(args[i].data, "!", 1) == 0) {
        header->operation = NJT_HTTP_MATCH_NOT_CONTAIN;
        i++;
        if (nelts != 3) {
            njt_log_error(NJT_LOG_EMERG,  hhccf->log, 0,"parameter number %u of header ! error.", nelts);
            return NJT_ERROR;
        }
        header->key = args[2];
        return NJT_OK;
    }
    header->key = args[i];
    i++;

    /*header abc;*/
    if (nelts == 2) {
        header->operation = NJT_HTTP_MATCH_CONTAIN;
        return NJT_OK;
    }

    if (nelts != 4) {
        njt_log_error(NJT_LOG_EMERG, hhccf->log, 0, "header parse error.");
        return NJT_ERROR;
    }

    if (args[i].len == 1) {
        if (njt_strncmp(args[i].data, "=", 1) == 0) {
            header->operation = NJT_HTTP_MATCH_EQUAL;

        } else if (njt_strncmp(args[i].data, "~", 1) == 0) {

            header->operation = NJT_HTTP_MATCH_REG_MATCH;
            header->regex = njt_http_match_regex_value(&cf, &args[3]);

            if (header->regex == NULL) {
                njt_log_error(NJT_LOG_EMERG, hhccf->log, 0, "header regex %V parse error.",
                              &args[3]);
                return NJT_ERROR;
            }

        } else {
            njt_log_error(NJT_LOG_EMERG, hhccf->log, 0, "header operation parse error.");
            return NJT_ERROR;
        }

        header->value = args[3];

        return NJT_OK;

    } else if (args[i].len == 2) {

        if (njt_strncmp(args[i].data, "!=", 2) == 0) {
            header->operation = NJT_HTTP_MATCH_NOT_EQUAL;

        } else if (njt_strncmp(args[i].data, "!~", 2) == 0) {

            header->operation = NJT_HTTP_MATCH_NOT_REG_MATCH;
            header->regex = njt_http_match_regex_value(&cf, &args[3]);

            if (header->regex == NULL) {
                njt_log_error(NJT_LOG_EMERG, hhccf->log, 0, "header regex %V parse error.",
                              &args[3]);
                return NJT_ERROR;
            }

        } else {
            return NJT_ERROR;
        }

        header->value = args[3];
        return NJT_OK;
    } else {
        njt_log_error(NJT_LOG_EMERG, hhccf->log, 0,
                      "header operation %V isn't supported.", &args[i]);
        return NJT_ERROR;
    }
}


static njt_int_t njt_http_match(njt_helper_hc_api_data_t *api_data,njt_helper_health_check_conf_t *hhccf) {
    njt_str_t args,tmp;
    njt_http_match_code_t *code,tmp_code;
    njt_uint_t i,diff;
    njt_int_t rc;
    njt_http_match_t *match;
    u_char* end;
    njt_conf_t cf;

    njt_http_health_check_conf_ctx_t *hhccc = hhccf->ctx;
    match = hhccc->match;

    match->conditions = 1;

    if (api_data->http.status.len > 0) {
        i=0;
        args = api_data->http.status;
        if (njt_strncmp(args.data, "!", 1) == 0) {
            match->status.not_operation = 1;
            i++;
        }
        if (i >= args.len) {
            njt_log_error(NJT_LOG_EMERG, hhccf->log, 0,"Too many parameters  for status.");
            return NJT_ERROR;
        }

        for (; i < args.len;) {
            // todo 测试分割是否正确
            tmp.data = args.data+i;
            end = njt_strlchr(args.data+i,args.data+args.len-i,' ');
            if(end == NULL){
                tmp.len = (size_t)args.len-i;
            }else{
                diff = end - tmp.data;
                tmp.len = (size_t)diff;
            }
            i += tmp.len;
            njt_memzero(&tmp_code, sizeof(njt_http_match_code_t));
            rc = njt_http_match_parse_code(&tmp, &tmp_code);
            if (rc == NJT_ERROR) {
                continue;
            }
            code = njt_array_push(&match->status.codes);
            if (code == NULL) {
                njt_log_error(NJT_LOG_EMERG, hhccf->log, 0, "code array push error.");
                return NJT_ERROR;
            }
            *code = tmp_code;
        }

    }
    if (api_data->http.headers.len > 0 ) {
        args = api_data->http.headers;
        njt_http_match_parse_dheaders(&args,match,hhccf);
    }
    if (api_data->http.body.len > 0) {
        args = api_data->http.body;
        if (args.len != 3) {
            njt_log_error(NJT_LOG_EMERG, hhccf->log, 0, "body parameter number error.");
            return NJT_ERROR;
        }

        if (njt_strncmp(args.data, "!~", 2) == 0) {
            diff = 2;
            match->body.operation = NJT_HTTP_MATCH_NOT_REG_MATCH;
        } else if (njt_strncmp(args.data, "~", 1) == 0) {
            diff = 1;
            match->body.operation = NJT_HTTP_MATCH_REG_MATCH;
        } else {
            /*log the case*/
            njt_log_error(NJT_LOG_EMERG, hhccf->log, 0, "body operation %V isn't supported error.", &args);
            return NJT_ERROR;
        }
        tmp.data = args.data + diff;
        tmp.len = args.len - diff;
        cf.pool = hhccf->pool;
        match->body.regex = njt_http_match_regex_value(&cf, &tmp);
        if (match->body.regex == NULL) {
            njt_log_error(NJT_LOG_EMERG, hhccf->log, 0, "body regex %V parse error.", &args);
            return NJT_ERROR;
        }
        match->body.value = tmp;
    }

    return NJT_OK;
}


static njt_http_match_t* njt_helper_http_match_create(njt_helper_health_check_conf_t *hhccf) {
    njt_http_match_t *match;

    match = njt_pcalloc(hhccf->pool ,sizeof(njt_http_match_t));
    if (match == NULL) {
        return NULL;
    }
    njt_memzero(match, sizeof(njt_http_match_t));

    return match;
}


static njt_http_v2_header_t njt_http_grpc_hc_headers[] = {
        {njt_string("content-length"), njt_string("5")},
        {njt_string("te"),             njt_string("trailers")},
        {njt_string("content-type"),   njt_string("application/grpc")},
        {njt_string("user-agent"),     njt_string("njet (health check grpc)")},
};


static njt_health_checker_t njt_health_checks[] = {

        {
                NJT_STREAM_MODULE,
                0,
                njt_string("tcp"),
                1,
                njt_http_health_check_tcp_handler,
                njt_http_health_check_tcp_handler,
                NULL,
                NULL,
                NULL
        },

        {
                NJT_HTTP_MODULE,
                0,
                njt_string("http"),
                0,
                njt_http_health_check_http_write_handler,
                njt_http_health_check_http_read_handler,
                NULL,
                njt_http_health_check_http_process,
                njt_http_health_check_http_update_status
        },

        {
                NJT_HTTP_MODULE,
                0,
                njt_string("grpc"),
                0,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
        },

        {
                0,
                0,
                njt_null_string,
                0,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL
        }
};


static void njt_free_peer_resource(njt_http_health_check_peer_t *hc_peer) {
    njt_pool_t *pool;

    pool = hc_peer->pool;
    if (pool) {
        njt_destroy_pool(pool);
    }

    if (hc_peer->peer.connection) {
        njt_close_connection(hc_peer->peer.connection);
    }

    njt_free(hc_peer);
    return;
}

static void njt_update_peer(njt_helper_health_check_conf_t *hhccf,
                            njt_http_upstream_rr_peer_t *peer,
                            njt_int_t status, njt_uint_t passes, njt_uint_t fails) {
    njt_http_health_check_conf_ctx_t *cf_ctx = hhccf->ctx;

    peer->hc_check_in_process = 0;
    peer->hc_checks++;
    if (status == NJT_OK || status == NJT_DONE) {

        peer->hc_consecutive_fails = 0;
        peer->hc_last_passed = 1; //zyg

        if (peer->hc_last_passed) {
            peer->hc_consecutive_passes++;
        }

        if (peer->hc_consecutive_passes >= passes) {
            peer->hc_down = (peer->hc_down / 100 * 100);

            if (peer->hc_downstart != 0) {
                peer->hc_upstart = njt_time();
                peer->hc_downtime = peer->hc_downtime + (((njt_uint_t) ((njt_timeofday())->sec) * 1000 +
                                                          (njt_uint_t) ((njt_timeofday())->msec)) - peer->hc_downstart);
            }
            peer->hc_downstart = 0;//(njt_timeofday())->sec;

        }
        if (hhccf->mandatory == 1 && cf_ctx->upstream->reload != 1 &&
            peer->hc_checks == 1) {  //hclcf->plcf->upstream.upstream->reload
            if (peer->down == 0) {
                peer->hc_down = (peer->hc_down / 100 * 100);
            }
        }


    } else {

        peer->hc_fails++;
        peer->hc_consecutive_passes = 0;
        peer->hc_consecutive_fails++;


        /*Only change the status at the first time when fails number mets*/
        if (peer->hc_consecutive_fails == fails) {
            peer->hc_unhealthy++;

            peer->hc_down = (peer->hc_down / 100 * 100) + 1;

            peer->hc_downstart = (njt_uint_t) ((njt_timeofday())->sec) * 1000 +
                                 (njt_uint_t) ((njt_timeofday())->msec); //(peer->hc_downstart == 0 ?(njt_current_msec):(peer->hc_downstart));
        }
        peer->hc_last_passed = 0;
        if (hhccf->mandatory == 1 && cf_ctx->upstream->reload != 1 && peer->hc_checks == 1) {
            if (peer->down == 0) {
                peer->hc_down = (peer->hc_down / 100 * 100) + 1;
                peer->hc_downstart = (njt_uint_t) ((njt_timeofday())->sec) * 1000 +
                                     (njt_uint_t) ((njt_timeofday())->msec); //(peer->hc_downstart == 0 ?(njt_current_msec):(peer->hc_downstart));
            }
        }

    }

    return;
}

static njt_int_t
njt_http_health_check_update_wo_lock(njt_helper_health_check_conf_t *hhccf,
                                     njt_http_health_check_peer_t *hc_peer,
                                     njt_http_upstream_rr_peer_t *peer,
                                     njt_int_t status) {
    njt_update_peer(hhccf, peer, status, hhccf->passes, hhccf->fails);
    njt_free_peer_resource(hc_peer);
    return NJT_OK;
}


static njt_int_t
njt_http_health_check_common_update(njt_http_health_check_peer_t *hc_peer,
                                    njt_int_t status) {
    njt_uint_t peer_id;
    njt_http_upstream_srv_conf_t *uscf;
    njt_http_upstream_rr_peers_t *peers;
    njt_http_upstream_rr_peer_t *peer;
    njt_http_health_check_conf_ctx_t *cf_ctx;
    njt_helper_health_check_conf_t *hhccf;

    /*Find the right peer and update it's status*/
    peer_id = hc_peer->peer_id;
    hhccf = hc_peer->hhccf;
    cf_ctx = hhccf->ctx;
    uscf = cf_ctx->upstream;
    peers = (njt_http_upstream_rr_peers_t *) uscf->peer.data;
    njt_http_upstream_rr_peers_wlock(peers);

    for (peer = peers->peer; peer != NULL; peer = peer->next) {
        if (peer->id == peer_id) {
            break;
        }
    }
    if (peer == NULL && peers->next) {
        for (peer = peers->next->peer; peer != NULL; peer = peer->next) {
            if (peer->id == peer_id) {
                break;
            }
        }
    }
    if (peer) {
        njt_update_peer(hhccf, peer, status, hhccf->passes, hhccf->fails);
    } else {
        /*LOG peer not found*/
        njt_log_debug1(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0, "peer %u isn't found", peer_id);
    }

    njt_http_upstream_rr_peers_unlock(peers);
    /*Free the resource*/
    njt_free_peer_resource(hc_peer);
    return NJT_OK;
}

static njt_int_t njt_http_health_check_tcp_handler(njt_event_t *ev) {
    njt_connection_t *c;
    njt_int_t rc;

    c = ev->data;

    rc = njt_http_health_check_peek_one_byte(c);

    return rc;
}

static void njt_http_health_check_write_handler(njt_event_t *wev) {
    njt_connection_t *c;
    njt_http_health_check_peer_t *hc_peer;
    njt_int_t rc;
    njt_http_health_check_conf_ctx_t *cf_ctx;
    njt_helper_health_check_conf_t *hhccf;

    c = wev->data;
    hc_peer = c->data;
    hhccf = hc_peer->hhccf;
    cf_ctx = hhccf->ctx;

    if (wev->timedout) {
        /*log the case and update the peer status.*/
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                       "write action for health check timeout");
        njt_http_health_check_update_status(hc_peer, NJT_ERROR);
        return;
    }

    if (wev->timer_set) {
        njt_del_timer(wev);
    }

    rc = cf_ctx->checker->write_handler(wev);
    if (rc == NJT_ERROR) {

        /*log the case and update the peer status.*/
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                       "write action error for health check");
        njt_http_health_check_update_status(hc_peer, NJT_ERROR);
        return;
    } else if (rc == NJT_DONE || rc == NJT_OK) {
        if (cf_ctx->checker->one_side) {
            njt_http_health_check_update_status(hc_peer, rc);
            return;
        }
    } else {
        /*AGAIN*/
    }

    if (!wev->timer_set) {
        njt_add_timer(wev, hhccf->timeout);
    }

    return;
}


static void njt_http_health_check_read_handler(njt_event_t *rev) {
    njt_connection_t *c;
    njt_http_health_check_peer_t *hc_peer;
    njt_int_t rc;
    njt_http_health_check_conf_ctx_t *cf_ctx;
    njt_helper_health_check_conf_t *hhccf;

    c = rev->data;
    hc_peer = c->data;
    hhccf = hc_peer->hhccf;
    cf_ctx = hhccf->ctx;

    if (rev->timedout) {

        /*log the case and update the peer status.*/
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                       "read action for health check timeout");
        njt_http_health_check_update_status(hc_peer, NJT_ERROR);
        return;
    }

    if (rev->timer_set) {
        njt_del_timer(rev);
    }

    rc = cf_ctx->checker->read_handler(rev);
    if (rc == NJT_ERROR) {
        /*log the case and update the peer status.*/
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                       "read action error for health check");
        njt_http_health_check_update_status(hc_peer, NJT_ERROR);
        return;
    } else if (rc == NJT_DONE) {
        njt_http_health_check_update_status(hc_peer, rc);
        return;
    } else {
        /*AGAIN*/
    }

    if (!rev->timer_set) {
        njt_add_timer(rev, hhccf->timeout);
    }

    return;
}

static njt_int_t njt_http_hc_test_connect(njt_connection_t *c) {
    int err;
    socklen_t len;

#if (NJT_HAVE_KQUEUE)
    if (njt_event_flags & NJT_USE_KQUEUE_EVENT)  {
        if (c->write->pending_eof || c->read->pending_eof) {
            if (c->write->pending_eof) {
                err = c->write->kq_errno;

            } else {
                err = c->read->kq_errno;
            }

            c->log->action = "connecting to upstream";
            (void) njt_connection_error(c, err,
                                    "kevent() reported that connect() failed");
            return NJT_ERROR;
        }

    } else
#endif
    {
        err = 0;
        len = sizeof(int);

        /*
         * BSDs and Linux return 0 and set a pending error in err
         * Solaris returns -1 and sets errno
         */

        if (getsockopt(c->fd, SOL_SOCKET, SO_ERROR, (void *) &err, &len)
            == -1) {
            err = njt_socket_errno;
        }

        if (err) {
            c->log->action = "connecting to hc";
            (void) njt_connection_error(c, err, "connect() failed");
            return NJT_ERROR;
        }
    }

    return NJT_OK;
}

#if (NJT_HTTP_SSL)


static njt_int_t
njt_http_hc_ssl_name(njt_connection_t *c, njt_http_health_check_peer_t *hc_peer) {
    u_char *p, *last;
    njt_str_t name;
    njt_http_health_check_conf_ctx_t *cf_ctx;
    njt_helper_health_check_conf_t *hhccf;

    hhccf = hc_peer->hhccf;
    cf_ctx = hhccf->ctx;

    if (hhccf->ssl.ssl_name.len) {
        name = hhccf->ssl.ssl_name;
    } else {
        name = cf_ctx->upstream->host;
    }
    if (name.len == 0) {
        goto done;
    }

    /*
     * ssl name here may contain port, notably if derived from $proxy_host
     * or $http_host; we have to strip it
     */

    p = name.data;
    last = name.data + name.len;

    if (*p == '[') {
        p = njt_strlchr(p, last, ']');

        if (p == NULL) {
            p = name.data;
        }
    }

    p = njt_strlchr(p, last, ':');

    if (p != NULL) {
        name.len = p - name.data;
    }

    if (!hhccf->ssl.ssl_server_name) {
        goto done;
    }

#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME

    /* as per RFC 6066, literal IPv4 and IPv6 addresses are not permitted */

    if (name.len == 0 || *name.data == '[') {
        goto done;
    }

    if (njt_inet_addr(name.data, name.len) != INADDR_NONE) {
        goto done;
    }

    /*
     * SSL_set_tlsext_host_name() needs a null-terminated string,
     * hence we explicitly null-terminate name here
     */

    p = njt_pnalloc(c->pool, name.len + 1);
    if (p == NULL) {
        return NJT_ERROR;
    }

    (void) njt_cpystrn(p, name.data, name.len + 1);

    name.data = p;

    njt_log_debug1(NJT_LOG_DEBUG_HTTP, c->log, 0, "upstream SSL server name: \"%s\"", name.data);

    if (SSL_set_tlsext_host_name(c->ssl->connection,
                                 (char *) name.data)
        == 0) {
        njt_ssl_error(NJT_LOG_ERR, c->log, 0,
                      "SSL_set_tlsext_host_name(\"%s\") failed", name.data);
        return NJT_ERROR;
    }

#endif

    done:

    hc_peer->ssl_name = name;

    return NJT_OK;
}

static njt_int_t
njt_http_hc_ssl_handshake(njt_connection_t *c, njt_http_health_check_peer_t *hc_peer) {
    long rc;
    njt_helper_health_check_conf_t *hhccf;

    hhccf = hc_peer->hhccf;

    if (c->ssl->handshaked) {

        if (hhccf->ssl.ssl_verify) {
            rc = SSL_get_verify_result(c->ssl->connection);
            if (rc != X509_V_OK) {
                njt_log_error(NJT_LOG_ERR, c->log, 0,
                              "upstream SSL certificate verify error: (%l:%s)",
                              rc, X509_verify_cert_error_string(rc));
                goto failed;
            }

            if (njt_ssl_check_host(c, &hc_peer->ssl_name) != NJT_OK) {
                njt_log_error(NJT_LOG_ERR, c->log, 0,
                              "hc SSL certificate does not match \"%V\"",
                              &hc_peer->ssl_name);
                goto failed;
            }
        }

        hc_peer->peer.connection->write->handler = njt_http_health_check_write_handler;
        hc_peer->peer.connection->read->handler = njt_http_health_check_read_handler;

        /*NJT_AGAIN or NJT_OK*/
        if (hhccf->timeout) {
            njt_add_timer(hc_peer->peer.connection->write, hhccf->timeout);
            njt_add_timer(hc_peer->peer.connection->read, hhccf->timeout);
        }
        return NJT_OK;
    }

    if (c->write->timedout) {
//        njt_http_health_check_update_status(hc_peer, NJT_ERROR);
        return NJT_ERROR;
    }

    failed:

//    njt_http_health_check_update_status(hc_peer, NJT_ERROR);
    return NJT_ERROR;
}

static void
njt_http_hc_ssl_handshake_handler(njt_connection_t *c) {
    njt_http_health_check_peer_t *hc_peer;
    njt_int_t rc;

    hc_peer = c->data;

    rc = njt_http_hc_ssl_handshake(c, hc_peer);
    if (rc != NJT_OK) {
        njt_http_health_check_update_status(hc_peer, NJT_ERROR);
    }
}

static njt_int_t
njt_http_hc_ssl_init_connection(njt_connection_t *c, njt_http_health_check_peer_t *hc_peer) {
    njt_int_t rc;
    njt_helper_health_check_conf_t *hhccf;

    hhccf = hc_peer->hhccf;

    if (njt_http_hc_test_connect(c) != NJT_OK) {
        return NJT_ERROR;
    }

    if (njt_ssl_create_connection(hhccf->ssl.ssl, c,
                                  NJT_SSL_BUFFER | NJT_SSL_CLIENT) != NJT_OK) {
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0, "ssl init create connection for health check error ");
        return NJT_ERROR;
    }

    c->sendfile = 0;

    if (hhccf->ssl.ssl_server_name || hhccf->ssl.ssl_verify) {
        if (njt_http_hc_ssl_name(c, hc_peer) != NJT_OK) {
            njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0, "ssl init check ssl name for health check error ");
            return NJT_ERROR;
        }
    }

    c->log->action = "SSL handshaking to hc";

    rc = njt_ssl_handshake(c);

    if (rc == NJT_AGAIN) {

        if (!c->write->timer_set) {
            njt_add_timer(c->write, hhccf->timeout);
        }

        c->ssl->handler = njt_http_hc_ssl_handshake_handler;
        return NJT_OK;
    }

    return njt_http_hc_ssl_handshake(c, hc_peer);
//    return NJT_OK;
}

#endif


static void
njt_http_health_loop_peer(njt_helper_health_check_conf_t *hhccf, njt_http_upstream_rr_peers_t *peers, njt_flag_t backup,
                          njt_flag_t op) {
    njt_int_t rc;
    njt_http_health_check_peer_t *hc_peer;
    njt_http_upstream_rr_peer_t *peer;
    njt_http_upstream_rr_peers_wlock(peers);

    peer = peers->peer;
    if (backup == 1) {
        peer = peers->next->peer;
    }
    for (; peer != NULL; peer = peer->next) {

        if (peer->down == 1) //zyg
        {
            continue;
        }
        if ((peer->hc_down == 2) || (op == 1)) {  //checking

            if (peer->hc_check_in_process && peer->hc_checks) {
                njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                               "peer's health check is in process.");
                continue;
            }

            peer->hc_check_in_process = 1;

            if (hhccf->type == NJT_HTTP_HC_GRPC) {
                njt_http_hc_grpc_loop_peer(hhccf, peer);
            } else {
                hc_peer = njt_calloc(sizeof(njt_http_health_check_peer_t), njt_cycle->log);
                if (hc_peer == NULL) {
                    /*log the malloc failure*/
                    njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                                   "memory allocate failure for health check.");
                    continue;
                }

                hc_peer->peer_id = peer->id;
                hc_peer->hhccf = hhccf;
                hc_peer->peer.sockaddr = peer->sockaddr;

                /*customized the peer's port*/
                if (hhccf->port) {
                    njt_inet_set_port(hc_peer->peer.sockaddr, hhccf->port);
                }

                hc_peer->peer.socklen = peer->socklen;
                hc_peer->peer.name = &peer->name;
                hc_peer->peer.get = njt_event_get_peer;
                hc_peer->peer.log = njt_cycle->log;
                hc_peer->peer.log_error = NJT_ERROR_ERR;
                hc_peer->pool = njt_create_pool(njt_pagesize, njt_cycle->log);


                njt_log_debug1(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                               "health check connect to peer of %V.", &peer->name);

                rc = njt_event_connect_peer(&hc_peer->peer);

                if (rc == NJT_ERROR || rc == NJT_DECLINED || rc == NJT_BUSY) {
                    njt_log_debug1(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                                   "health check connect to peer of %V errror.", &peer->name);
                    /*release the memory and update the statistics*/
                    njt_http_health_check_update_wo_lock(hhccf, hc_peer, peer, NJT_ERROR);
                    continue;
                }

                hc_peer->peer.connection->data = hc_peer;
                hc_peer->peer.connection->pool = hc_peer->pool;
#if (NJT_HTTP_SSL)

                if (hhccf->ssl.ssl_enable && hhccf->ssl.ssl->ctx &&
                    hc_peer->peer.connection->ssl == NULL) { //zyg
                    rc = njt_http_hc_ssl_init_connection(hc_peer->peer.connection, hc_peer);
                    if (rc == NJT_ERROR) {
                        njt_http_health_check_update_wo_lock(hhccf, hc_peer, peer, NJT_ERROR);
                    }
                    continue;
                }

#endif

                hc_peer->peer.connection->write->handler = njt_http_health_check_write_handler;
                hc_peer->peer.connection->read->handler = njt_http_health_check_read_handler;

                /*NJT_AGAIN or NJT_OK*/
                if (hhccf->timeout) {
                    njt_add_timer(hc_peer->peer.connection->write, hhccf->timeout);
                    njt_add_timer(hc_peer->peer.connection->read, hhccf->timeout);
                }
            }
        }
    }

    njt_http_upstream_rr_peers_unlock(peers);
}


/*define the status machine stage*/
static void njt_http_health_check_timer_handler(njt_event_t *ev) {
    njt_helper_health_check_conf_t *hhccf;
    njt_http_upstream_srv_conf_t *uscf;
    njt_http_upstream_rr_peers_t *peers;
    njt_uint_t jitter;
    njt_flag_t op = 0;
    njt_http_health_check_conf_ctx_t *cf_ctx;

    hhccf = ev->data;
    cf_ctx = hhccf->ctx;
    if (hhccf == NULL) {
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0, "no valid data");
        return;
    }

    uscf = cf_ctx->upstream;
    if (uscf == NULL) {
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0, "no upstream data");
        return;
    }

    if (njt_quit || njt_terminate || njt_exiting) {
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0,"active probe clearup for quiting");
        return;
    }

    njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,"Health check timer is triggered.");

    if (hhccf->mandatory == 1 && hhccf->persistent == 0 && hhccf->curr_delay != 0) {
        hhccf->curr_frame += 1000;
    }

    if (hhccf->curr_delay != 0 && hhccf->curr_delay <= hhccf->curr_frame) {
        hhccf->curr_frame = 0;
        op = 1;
    } else if (hhccf->curr_delay == 0) {
        op = 1;
    }

    peers = (njt_http_upstream_rr_peers_t *) uscf->peer.data;
    if (peers) {
        njt_http_health_loop_peer(hhccf, peers, 0, op);
    }
    if (peers->next) {
        njt_http_health_loop_peer(hhccf, peers, 1, op);
    }
    jitter = 0;
    if (hhccf->jitter) {
        jitter = njt_random() % hhccf->jitter;

        njt_log_debug1(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                       "delay %u for the health check timer.", jitter);
    }
    if (hhccf->mandatory == 1 && hhccf->persistent == 0) {
        hhccf->curr_delay = hhccf->interval + jitter;
        njt_add_timer(&hhccf->hc_timer, 1000);
    } else {
        njt_add_timer(&hhccf->hc_timer, hhccf->interval + jitter);
    }
    return;
}


static njt_int_t njt_http_health_check_add(njt_helper_health_check_conf_t *hhccf) {
    njt_event_t *hc_timer;
    njt_uint_t refresh_in;
    njt_helper_main_conf_t *hmcf;
    njt_http_health_check_conf_ctx_t *cf_ctx;
    njt_http_upstream_rr_peers_t *peers;
    njt_http_upstream_rr_peer_t *peer;

    njt_cycle_t *cycle = (njt_cycle_t *)njt_cycle;

    hmcf = (njt_helper_main_conf_t *)njt_get_conf(cycle->conf_ctx,njt_http_health_check_helper);

    hc_timer = &hhccf->hc_timer;
    hc_timer->handler = njt_http_health_check_timer_handler;
    hc_timer->log = hhccf->log;
    hc_timer->data = hhccf;
    hc_timer->cancelable = 1;
    refresh_in = njt_random() % 1000 ;

    cf_ctx= hhccf->ctx;
    if (hhccf->persistent == 1 && cf_ctx->upstream) {
        cf_ctx->upstream->hc_type = 2;  //peers = olduscf->peer.data;
    } else if (hhccf->mandatory == 1 && cf_ctx->upstream) {
        cf_ctx->upstream->hc_type = 1;
        peers = cf_ctx->upstream->peer.data;
        if (peers) {
            for (peer = peers->peer; peer; peer = peer->next) {
                peer->hc_down = 2; //checking
            }
        }
    }
    njt_queue_insert_tail(&hmcf->hc_queue,&hhccf->queue);
    if(!hc_timer->timer_set){
        njt_add_timer(hc_timer, refresh_in);
    }
    return NJT_OK;
}


static njt_int_t
njt_http_health_check_peek_one_byte(njt_connection_t *c) {
    char buf[1];
    njt_int_t n;
    njt_err_t err;

    n = recv(c->fd, buf, 1, MSG_PEEK);
    err = njt_socket_errno;

    njt_log_debug2(NJT_LOG_DEBUG_HTTP, c->log, err,
                   "http check upstream recv(): %i, fd: %d",
                   n, c->fd);

    if (n == 1 || (n == -1 && err == NJT_EAGAIN)) {
        return NJT_OK;
    }

    return NJT_ERROR;
}

static njt_health_checker_t *
njt_http_get_health_check_type(njt_str_t *str) {
    njt_uint_t i;

    for (i = 0; /* void */ ; i++) {

        if (njt_health_checks[i].type == 0) {
            break;
        }

        if (str->len != njt_health_checks[i].name.len) {
            continue;
        }

        if (njt_strncasecmp(str->data, njt_health_checks[i].name.data,
                            str->len) == 0) {
            return &njt_health_checks[i];
        }
    }

    return NULL;
}

static void
njt_http_health_check_dummy_handler(njt_event_t *ev) {
    njt_log_debug0(NJT_LOG_DEBUG_EVENT, ev->log, 0,
                   "http health check dummy handler");
}

static njt_int_t
njt_http_health_check_http_write_handler(njt_event_t *wev) {
    njt_connection_t *c;
    njt_http_health_check_peer_t *hc_peer;
    ssize_t n, size;
    njt_http_health_check_conf_ctx_t *cf_ctx;
    njt_helper_health_check_conf_t *hhccf;

    c = wev->data;
    hc_peer = c->data;
    hhccf = hc_peer->hhccf;
    cf_ctx = hhccf->ctx;

    njt_log_debug0(NJT_LOG_DEBUG_HTTP, c->log, 0, "http check send.");


    if (hc_peer->send_buf == NULL) {
        hc_peer->send_buf = njt_create_temp_buf(hc_peer->pool, njt_pagesize);
        if (hc_peer->send_buf == NULL) {
            /*log the send buf allocation failure*/
            njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                           "malloc failure of the send buffer for health check.");
            return NJT_ERROR;
        }
        /*Fill in the buff*/
        hc_peer->send_buf->last = njt_snprintf(hc_peer->send_buf->last,
                                               hc_peer->send_buf->end - hc_peer->send_buf->last, "GET %V HTTP/1.1" CRLF,
                                               &cf_ctx->uri);
        hc_peer->send_buf->last = njt_snprintf(hc_peer->send_buf->last,
                                               hc_peer->send_buf->end - hc_peer->send_buf->last,
                                               "Connection: close" CRLF);
        hc_peer->send_buf->last = njt_snprintf(hc_peer->send_buf->last,
                                               hc_peer->send_buf->end - hc_peer->send_buf->last, "Host: %V" CRLF,
                                               &cf_ctx->upstream->host);
        hc_peer->send_buf->last = njt_snprintf(hc_peer->send_buf->last,
                                               hc_peer->send_buf->end - hc_peer->send_buf->last,
                                               "User-Agent: njet (health-check)" CRLF);
        hc_peer->send_buf->last = njt_snprintf(hc_peer->send_buf->last,
                                               hc_peer->send_buf->end - hc_peer->send_buf->last, CRLF);
    }

    size = hc_peer->send_buf->last - hc_peer->send_buf->pos;

    n = c->send(c, hc_peer->send_buf->pos,
                hc_peer->send_buf->last - hc_peer->send_buf->pos);

    if (n == NJT_ERROR) {
        return NJT_ERROR;
    }

    if (n > 0) {
        hc_peer->send_buf->pos += n;
        if (n == size) {
            wev->handler = njt_http_health_check_dummy_handler;

            if (njt_handle_write_event(wev, 0) != NJT_OK) {
                /*LOG the failure*/
                njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                               "write event handle error for health check");
                return NJT_ERROR;
            }
            return NJT_DONE;
        }
    }

    return NJT_AGAIN;
}

static njt_int_t
njt_http_health_check_http_read_handler(njt_event_t *rev) {
    njt_connection_t *c;
    njt_http_health_check_peer_t *hc_peer;
    ssize_t n, size;
    njt_buf_t *b;
    njt_int_t rc;
    njt_chain_t *chain, *node;
    njt_health_check_http_parse_t *hp;
    njt_http_health_check_conf_ctx_t *cf_ctx;
    njt_helper_health_check_conf_t *hhccf;

    c = rev->data;
    hc_peer = c->data;
    hhccf = hc_peer->hhccf;
    cf_ctx = hhccf->ctx;

    njt_log_debug0(NJT_LOG_DEBUG_HTTP, c->log, 0, "http check recv.");


    /*Init the internal parser*/
    if (hc_peer->parser == NULL) {
        hp = njt_pcalloc(hc_peer->pool, sizeof(njt_http_health_check_peer_t));
        if (hp == NULL) {
            /*log*/
            njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                           "memory allocation error for health check.");
            return NJT_ERROR;
        }

        hp->stage = NJT_HTTP_PARSE_STATUS_LINE;
        hp->process = njt_health_check_http_parse_status_line;

        hc_peer->parser = hp;
    }

    for (;;) {

        if (hc_peer->recv_buf == NULL) {
            b = njt_create_temp_buf(hc_peer->pool, njt_pagesize);
            if (b == NULL) {
                /*log*/
                njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                               "recv buffer memory allocation error for health check.");
                return NJT_ERROR;
            }
            hc_peer->recv_buf = b;
        }

        b = hc_peer->recv_buf;
        size = b->end - b->last;

        n = c->recv(c, b->last, size);

        if (n > 0) {
            b->last += n;

            rc = cf_ctx->checker->process(hc_peer);
            if (rc == NJT_ERROR) {
                return NJT_ERROR;
            }

            /*link chain buffer*/
            if (b->last == b->end) {

                hp = hc_peer->parser;

                if (hp->stage != NJT_HTTP_PARSE_BODY) {
                    /*log. The status and headers are too large to be hold in one buffer*/
                    njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                                   "status and headers exceed one page size");
                    return NJT_ERROR;
                }

                chain = njt_alloc_chain_link(hc_peer->pool);
                if (chain == NULL) {
                    /*log and process the error*/
                    njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                                   "memory allocation of the chain buf failed.");
                    return NJT_ERROR;
                }

                chain->buf = b;
                chain->next = NULL;

                node = hc_peer->recv_chain;
                if (node == NULL) {
                    hc_peer->recv_chain = chain;
                } else {
                    hc_peer->last_chain_node->next = chain;
                }
                hc_peer->last_chain_node = chain;

                /*Reset the recv buffer*/
                hc_peer->recv_buf = NULL;
            }

            continue;
        }

        if (n == NJT_AGAIN) {
            if (njt_handle_read_event(rev, 0) != NJT_OK) {
                /*log*/
                njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                               "read event handle error for health check");
                return NJT_ERROR;
            }
            return NJT_AGAIN;
        }

        if (n == NJT_ERROR) {
            /*log*/
            njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                           "read error for health check");
            return NJT_ERROR;
        }

        break;
    }


    hp = hc_peer->parser;
    hp->done = 1;
    rc = cf_ctx->checker->process(hc_peer);

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
njt_http_health_check_http_process(njt_http_health_check_peer_t *hc_peer) {
    njt_health_check_http_parse_t *parse;
    njt_int_t rc;

    parse = hc_peer->parser;
    rc = parse->process(hc_peer);

    return rc;
}

/*We assume the status line and headers are located in one buffer*/
static njt_int_t
njt_health_check_http_parse_status_line(njt_http_health_check_peer_t *hc_peer) {
    u_char ch;
    u_char *p;
    njt_health_check_http_parse_t *hp;
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

    hp = hc_peer->parser;
    b = hc_peer->recv_buf;
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

    hp->stage = NJT_HTTP_PARSE_HEADER;
    hp->process = njt_health_check_http_process_headers;

    hp->process(hc_peer);

    return NJT_OK;
}


static njt_int_t
njt_health_check_http_process_headers(njt_http_health_check_peer_t *hc_peer) {
    njt_int_t rc;
    njt_table_elt_t *h;
    njt_health_check_http_parse_t *hp;

    njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0, "http process header.");

    hp = hc_peer->parser;

    if (hp->headers.size == 0) {
        rc = njt_array_init(&hp->headers, hc_peer->pool, 4,
                            sizeof(njt_table_elt_t));
        if (rc != NJT_OK) {
            return NJT_ERROR;
        }
    }

    for (;;) {

        rc = njt_health_check_http_parse_header_line(hc_peer);

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
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                       "http process header error.");
        return NJT_ERROR;
    }

    /*TODO check if the first buffer is used out*/
    hp->stage = NJT_HTTP_PARSE_BODY;
    hp->process = njt_health_check_http_process_body;

    return hp->process(hc_peer);
}

static njt_int_t
njt_health_check_http_parse_header_line(njt_http_health_check_peer_t *hc_peer) {
    u_char c, ch, *p;
    njt_health_check_http_parse_t *hp;
    njt_buf_t *b;

    enum {
        sw_start = 0,
        sw_name,
        sw_space_before_value,
        sw_value,
        sw_space_after_value,
        sw_almost_done,
        sw_header_almost_done
    } state;

    b = hc_peer->recv_buf;
    hp = hc_peer->parser;
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
njt_health_check_http_process_body(njt_http_health_check_peer_t *hc_peer) {
    njt_health_check_http_parse_t *hp;

    hp = hc_peer->parser;

    if (hp->done) {
        return NJT_DONE;
    }
    return NJT_OK;
}

static njt_int_t
njt_http_health_check_update_status(njt_http_health_check_peer_t *hc_peer,
                                    njt_int_t status) {
    njt_int_t rc;
    njt_http_health_check_conf_ctx_t *cf_ctx;
    njt_helper_health_check_conf_t *hhccf;

    hhccf = hc_peer->hhccf;
    cf_ctx = hhccf->ctx;

    if (cf_ctx->checker->update == NULL) {
        rc = njt_http_health_check_common_update(hc_peer, status);
    } else {
        rc = cf_ctx->checker->update(hc_peer, status);
    }

    return rc;

}


static njt_int_t
njt_http_health_check_http_match_body(njt_http_match_body_t *body,
                                      njt_http_health_check_peer_t *hc_peer) {
    njt_int_t n;
    njt_str_t result;
    njt_health_check_http_parse_t *hp;
    njt_buf_t *content;
    njt_uint_t size;
    njt_chain_t *node;


    /* recreate the body buffer*/
    content = njt_create_temp_buf(hc_peer->pool, njt_pagesize);
    if (content == NULL) {
        /*log*/
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                       "content buffer allocation error for health check");
        return NJT_ERROR;
    }

    hp = hc_peer->parser;
    if (hc_peer->recv_chain == NULL) {

        size = hc_peer->recv_buf->last - hp->body_start;
        content->last = njt_snprintf(content->last, size, "%s", hp->body_start);

    } else {

        node = hc_peer->recv_chain;
        size = node->buf->last - hp->body_start;
        content->last = njt_snprintf(content->last, size, "%s", hp->body_start);

        if (node->buf->end == node->buf->last) {
            node = node->next;
            size = njt_pagesize - size;
            content->last = njt_snprintf(content->last, size, "%s", node->buf->pos);
        }

    }

    result.data = content->start;
    result.len = content->last - content->start;

    n = njt_regex_exec(body->regex, &result, NULL, 0);

    if (body->operation == NJT_HTTP_MATCH_REG_MATCH) {

        if (n == NJT_REGEX_NO_MATCHED) {
            return NJT_ERROR;
        } else {
            return NJT_OK;
        }

    } else if (body->operation == NJT_HTTP_MATCH_NOT_REG_MATCH) {

        if (n == NJT_REGEX_NO_MATCHED) {
            return NJT_OK;
        } else {
            return NJT_ERROR;
        }
    } else {
        return NJT_ERROR;
    }

    return NJT_OK;
}

static njt_int_t
njt_http_health_check_http_match_header(njt_http_match_header_t *input,
                                        njt_http_health_check_peer_t *hc_peer) {
    njt_health_check_http_parse_t *hp;
    njt_int_t rc;
    njt_uint_t i;
    njt_int_t n;
    njt_table_elt_t *headers, *header;

    hp = hc_peer->parser;
    headers = hp->headers.elts;

    rc = NJT_OK;

    switch (input->operation) {

        case NJT_HTTP_MATCH_CONTAIN:

            rc = NJT_ERROR;
            for (i = 0; i < hp->headers.nelts; i++) {
                header = headers + i;
                if (header->key.len != input->key.len) {
                    continue;
                }

                if (njt_strncmp(header->key.data, input->key.data, input->key.len) == 0) {
                    return NJT_OK;
                }
            }
            break;

        case NJT_HTTP_MATCH_NOT_CONTAIN:
            rc = NJT_OK;
            for (i = 0; i < hp->headers.nelts; i++) {
                header = headers + i;
                if (header->key.len != input->key.len) {
                    continue;
                }

                if (njt_strncmp(header->key.data, input->key.data, input->key.len) == 0) {
                    return NJT_ERROR;
                }
            }

            break;

            /*If the header doesn't occur means failure*/
        case NJT_HTTP_MATCH_EQUAL:

            rc = NJT_ERROR;

            for (i = 0; i < hp->headers.nelts; i++) {

                header = headers + i;
                if (header->key.len != input->key.len) {
                    continue;
                }

                if (njt_strncmp(header->key.data, input->key.data, input->key.len) == 0) {
                    if (header->value.len != input->value.len) {
                        return NJT_ERROR;
                    }

                    if (njt_strncmp(header->value.data, input->value.data,
                                    input->value.len) == 0) {
                        return NJT_OK;
                    }

                    return NJT_ERROR;
                }
            }

            break;

        case NJT_HTTP_MATCH_NOT_EQUAL:

            rc = NJT_ERROR;

            for (i = 0; i < hp->headers.nelts; i++) {

                header = headers + i;
                if (header->key.len != input->key.len) {
                    continue;
                }

                if (njt_strncmp(header->key.data, input->key.data, input->key.len) == 0) {

                    if (header->value.len != input->value.len) {
                        return NJT_OK;
                    }

                    if (njt_strncmp(header->value.data, input->value.data,
                                    input->value.len) == 0) {
                        return NJT_ERROR;
                    }

                    return NJT_OK;
                }
            }

            break;

#if (NJT_PCRE)
        case NJT_HTTP_MATCH_REG_MATCH:

            if (input->regex == NULL) {
                return NJT_ERROR;
            }

            rc = NJT_ERROR;

            for (i = 0; i < hp->headers.nelts; i++) {

                header = headers + i;
                if (header->key.len != input->key.len) {
                    continue;
                }

                if (njt_strncmp(header->key.data, input->key.data, input->key.len) == 0) {

                    n = njt_regex_exec(input->regex, &header->value, NULL, 0);

                    if (n == NJT_REGEX_NO_MATCHED) {
                        return NJT_ERROR;
                    }

                    return NJT_OK;
                }
            }

            break;

        case NJT_HTTP_MATCH_NOT_REG_MATCH:

            if (input->regex == NULL) {
                return NJT_ERROR;
            }

            rc = NJT_ERROR;

            for (i = 0; i < hp->headers.nelts; i++) {

                header = headers + i;
                if (header->key.len != input->key.len) {
                    continue;
                }

                if (njt_strncmp(header->key.data, input->key.data, input->key.len) == 0) {

                    n = njt_regex_exec(input->regex, &header->value, NULL, 0);

                    if (n == NJT_REGEX_NO_MATCHED) {
                        return NJT_OK;
                    }

                    return NJT_ERROR;
                }
            }

            break;
#endif
        default:
            njt_log_debug1(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                           "unsupported operation %u.\n", input->operation);
            return NJT_ERROR;
    }

    return rc;
}

static njt_int_t
njt_http_health_check_http_update_status(njt_http_health_check_peer_t *hc_peer,
                                         njt_int_t status) {
    njt_int_t rc;
    njt_health_check_http_parse_t *hp;
    njt_http_match_t *match;
    njt_uint_t i, result;
    njt_http_match_code_t *code, *codes;
    njt_http_match_header_t *header, *headers;
    njt_http_health_check_conf_ctx_t *cf_ctx;
    njt_helper_health_check_conf_t *hhccf;

    hp = hc_peer->parser;
    hhccf = hc_peer->hhccf;
    cf_ctx = hhccf->ctx;
    match = cf_ctx->match;

    /*By default, we change the code is in range 200 or 300*/
    rc = NJT_ERROR;
    if (hp == NULL) {
        rc = njt_http_health_check_common_update(hc_peer, rc);
        return rc;
    }

    if (match == NULL) {
        if (hp->code >= 200 && hp->code <= 399) {
            rc = NJT_OK;
        }
        goto set_status;
    }

    /*an empty match*/
    if (!match->conditions) {
        goto set_status;
    }

    /*check the status*/

    /*step1 check code*/
    if (match->status.codes.nelts == 0) {
        goto headers;
    }

    result = 0;
    codes = match->status.codes.elts;
    for (i = 0; i < match->status.codes.nelts; i++) {
        code = codes + i;
        if (code->single) {
            if (hp->code == code->code) {
                result = 1;
                break;
            }
        } else {
            if (hp->code >= code->code && hp->code <= code->last_code) {
                result = 1;
                break;
            }
        }
    }

    if (match->status.not_operation) {
        result = !result;
    }

    rc = result ? NJT_OK : NJT_ERROR;

    if (rc == NJT_ERROR) {
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0, "match status error.");
        goto set_status;
    }

    headers:
    /*step2 check header*/
    headers = match->headers.elts;
    for (i = 0; i < match->headers.nelts; i++) {
        header = headers + i;

        rc = njt_http_health_check_http_match_header(header, hc_peer);
        if (rc == NJT_ERROR) {
            njt_log_debug1(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0, "match header %V error.",
                           &header->value);
            goto set_status;
        }

    }

    /*step3 check body*/
    if (match->body.regex) {
        rc = njt_http_health_check_http_match_body(&match->body, hc_peer);
        /*regular expression match of the body*/
        if (rc == NJT_ERROR) {
            njt_log_debug1(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0, "match body %V error.",
                           &match->body.value);
            goto set_status;
        }
    }

    set_status:
    rc = njt_http_health_check_common_update(hc_peer, rc);
    return rc;
}

struct njt_http_grpc_hc_peer_s {
    njt_uint_t peer_id;
    njt_helper_health_check_conf_t *hhccf;
    njt_pool_t *pool;
    njt_peer_connection_t pc;
    njt_buf_t *send_buf;
    njt_buf_t *recv_buf;
    njt_chain_t *recv_chain;
    njt_chain_t *last_chain_node;
    void *parser;
};

typedef struct njt_http_grpc_hc_peer_s njt_http_grpc_hc_peer_t;

static void
njt_http_hc_grpc_loop_peer(njt_helper_health_check_conf_t *hhccf, njt_http_upstream_rr_peer_t *peer) {
    njt_int_t rc;
    njt_http_grpc_hc_peer_t *hc_peer;
    njt_http_request_t *r;
    njt_http_upstream_t *u;
    njt_http_upstream_state_t *state;
    njt_chain_t **last;
    njt_health_check_http_parse_t *hp;
    njt_list_part_t *part;
    njt_table_elt_t *header;
    njt_uint_t i;
    njt_peer_connection_t *pc;
    void *grpc_con;
    void *input_filter_ctx;
    njt_http_health_check_conf_ctx_t *cf_ctx;

    hc_peer = njt_calloc(sizeof(njt_http_grpc_hc_peer_t), njt_cycle->log);
    if (hc_peer == NULL) {
        /*log the malloc failure*/
        njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                       "memory allocate failure for health check.");
        goto OUT;
    }

    hc_peer->peer_id = peer->id;
    hc_peer->hhccf = hhccf;
    hc_peer->pc.sockaddr = peer->sockaddr;

    /*customized the peer's port*/
    if (hhccf->port) {
        njt_inet_set_port(hc_peer->pc.sockaddr, hhccf->port);
    }

    hc_peer->pc.socklen = peer->socklen;
    hc_peer->pc.name = &peer->name;
    hc_peer->pc.get = njt_event_get_peer;
    hc_peer->pc.log = njt_cycle->log;
    hc_peer->pc.log_error = NJT_ERROR_ERR;
    hc_peer->pool = njt_create_pool(njt_pagesize, njt_cycle->log);

    njt_pool_t *pool;
    pool = hc_peer->pool;

    /* allocate memory, create context for grpc health check */

    r = njt_pcalloc(pool, sizeof(njt_http_request_t));
    if (r == NULL) {
        goto OUT;
    }

    grpc_con = njt_http_grpc_hc_create_grpc_on(pool);
    if (grpc_con == NULL) {
        goto OUT;
    }

    input_filter_ctx = njt_http_grpc_hc_create_in_filter_ctx(pool, r, grpc_con);
    if (input_filter_ctx == NULL) {
        goto OUT;
    }

    /*Init the internal parser*/
    if (hc_peer->parser == NULL) {
        hp = njt_pcalloc(pool, sizeof(njt_http_grpc_hc_peer_t));
        if (hp == NULL) {
            njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                           "memory allocation error for health check.");
            goto OUT;
        }

        hc_peer->parser = hp;
    } else {
        hp = hc_peer->parser;
    }

    hp->state = njt_http_grpc_hc_st_start;
    hp->start = njt_current_msec;
    hp->code = GRPC_STATUS_CODE_INVALID;

    part = &r->headers_in.headers.part;
    part->nelts = sizeof(njt_http_grpc_hc_headers) / sizeof(njt_http_v2_header_t);
    part->next = NULL;
    part->elts = njt_pcalloc(pool, sizeof(njt_table_elt_t) * part->nelts);
    if (part->elts == NULL) {
        goto OUT;
    };

    header = part->elts;
    for (i = 0; i < part->nelts; i++) {
        header[i].key = njt_http_grpc_hc_headers[i].name;
        header[i].value = njt_http_grpc_hc_headers[i].value;
        header[i].lowcase_key = header[i].key.data;
        header[i].hash = njt_hash_key(header[i].key.data, header[i].key.len);
    }

    pc = &hc_peer->pc;
    pc->type = SOCK_STREAM;

    //r->hc = hc_peer; //zyg delete
    njt_http_set_ctx(r, hc_peer, njt_http_health_check_helper);
    cf_ctx = hhccf->ctx;
    r->unparsed_uri = cf_ctx->gsvc;
    r->uri.len = r->unparsed_uri.len;
    r->uri.data = r->unparsed_uri.data;
    r->pool = pool;
    r->method = NJT_HTTP_POST;
    r->main = r;

    u = njt_pcalloc(pool, sizeof(njt_http_upstream_t));
    if (u == NULL) {
        goto OUT;
    }
    r->upstream = u;

    rc = njt_event_connect_peer(pc);

    if (rc == NJT_ERROR || rc == NJT_DECLINED) {
        njt_log_error(NJT_LOG_ERR, njt_cycle->log, 0,
                      "health check connect to peer %V failed.", &peer->name);
        goto OUT;
    }

    r->connection = pc->connection;
    u->peer.connection = pc->connection;
    u->input_filter_ctx = input_filter_ctx;
    u->writer.pool = pool;
    u->writer.connection = pc->connection;

    last = njt_pcalloc(pool, sizeof(njt_chain_t *));
    if (last == NULL) {
        goto OUT;
    };
    u->writer.last = last;
    u->start_time = njt_current_msec;

    state = njt_pcalloc(pool, sizeof(njt_http_upstream_state_t));
    if (state == NULL) {
        goto OUT;
    }

    cf_ctx = hhccf->ctx;
    u->state = state;
    u->output.filter_ctx = r;
    u->output.output_filter = njt_http_grpc_body_output_filter;
    u->write_event_handler = njt_http_upstream_send_request_handler;
    u->read_event_handler = njt_http_upstream_process_header;

    njt_http_grpc_hc_set_upstream(u);

    rc = njt_http_grpc_create_request(r);

    pc->connection->data = r;
    pc->connection->log = njt_cycle->log;
    pc->connection->pool = pool;
    pc->connection->write->data = pc->connection;
    pc->connection->read->data = pc->connection;
    pc->connection->write->handler = njt_http_grpc_hc_handler;
    pc->connection->read->handler = njt_http_upstream_handler;

    return;

    OUT:
    /*release the memory and update the statistics*/
    njt_http_health_check_update_wo_lock(hhccf, (njt_http_health_check_peer_t *) hc_peer, peer, NJT_ERROR);
    return;
}


static void
njt_http_grpc_hc_handler(njt_event_t *wev) {
    njt_connection_t *c;
    njt_http_request_t *r;
    njt_http_grpc_hc_peer_t *hc_peer;
    njt_http_upstream_t *u;
    njt_health_check_http_parse_t *hp;
    ssize_t n, size;
    njt_peer_connection_t *pc;
    njt_http_health_check_conf_ctx_t *cf_ctx;
    njt_helper_health_check_conf_t *hhccf;

    njt_uint_t result = NJT_ERROR;

    c = wev->data;
    r = c->data;
    u = r->upstream;
    //hc_peer = r->hc;  zyg
    hc_peer = njt_http_get_module_ctx(r, njt_http_health_check_helper);
    hp = hc_peer->parser;
    pc = &hc_peer->pc;
    hhccf = hc_peer->hhccf;
    cf_ctx = hhccf->ctx;

    if ((hp->state >= njt_http_grpc_hc_st_sent) || (njt_current_msec - hp->start + 1000 >= hhccf->timeout)) {

        if ((hp->code != GRPC_STATUS_CODE_INVALID) || (njt_current_msec - hp->start + 1000 >= hhccf->timeout)) {
            if ((hp->code == GRPC_STATUS_CODE_OK) || (hp->code == cf_ctx->gstatus)) {
                result = NJT_OK;
            }

            njt_log_error(NJT_LOG_INFO, njt_cycle->log, 0,
                          "grpc hc %s for peer %V", (result == NJT_OK) ? "OK" : "ERROR", hc_peer->pc.name);

            /* update peer status and free the resource */
            njt_http_health_check_common_update((njt_http_health_check_peer_t *) hc_peer, result);
            return;
        }

        njt_add_timer(wev, 1000);
        return;
    }

    size = u->request_bufs->buf->last - u->request_bufs->buf->pos;
    n = pc->connection->send(pc->connection, u->request_bufs->buf->pos, size);

    if (n == NJT_ERROR) {
        njt_log_error(NJT_LOG_INFO, njt_cycle->log, 0,
                      "grpc hc ERROR for peer %V", hc_peer->pc.name);

        /* update peer status and free the resource */
        njt_http_health_check_common_update((njt_http_health_check_peer_t *) hc_peer, NJT_ERROR);
        return;
    }

    njt_add_timer(wev, 1000);

    if (n > 0) {
        u->request_bufs->buf->pos += n;
        if (n == size) {
            hp->state = njt_http_grpc_hc_st_sent;
            if (njt_handle_write_event(wev, 0) != NJT_OK) {
                /*LOG the failure*/
                njt_log_debug0(NJT_LOG_DEBUG_HTTP, njt_cycle->log, 0,
                               "write event handle error for health check");
            }
        }
    }
}


