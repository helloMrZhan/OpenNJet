
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#define NJT_HTTP_REFERER_NO_URI_PART  ((void *) 4)


typedef struct {
    ngx_hash_combined_t      hash;

#if (NJT_PCRE)
    ngx_array_t             *regex;
    ngx_array_t             *server_name_regex;
#endif

    ngx_flag_t               no_referer;
    ngx_flag_t               blocked_referer;
    ngx_flag_t               server_names;

    ngx_hash_keys_arrays_t  *keys;

    ngx_uint_t               referer_hash_max_size;
    ngx_uint_t               referer_hash_bucket_size;
} ngx_http_referer_conf_t;


static ngx_int_t ngx_http_referer_add_variables(ngx_conf_t *cf);
static void * ngx_http_referer_create_conf(ngx_conf_t *cf);
static char * ngx_http_referer_merge_conf(ngx_conf_t *cf, void *parent,
    void *child);
static char *ngx_http_valid_referers(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static ngx_int_t ngx_http_add_referer(ngx_conf_t *cf,
    ngx_hash_keys_arrays_t *keys, ngx_str_t *value, ngx_str_t *uri);
static ngx_int_t ngx_http_add_regex_referer(ngx_conf_t *cf,
    ngx_http_referer_conf_t *rlcf, ngx_str_t *name);
#if (NJT_PCRE)
static ngx_int_t ngx_http_add_regex_server_name(ngx_conf_t *cf,
    ngx_http_referer_conf_t *rlcf, ngx_http_regex_t *regex);
#endif
static int ngx_libc_cdecl ngx_http_cmp_referer_wildcards(const void *one,
    const void *two);


static ngx_command_t  ngx_http_referer_commands[] = {

    { ngx_string("valid_referers"),
      NJT_HTTP_SRV_CONF|NJT_HTTP_LOC_CONF|NJT_CONF_1MORE,
      ngx_http_valid_referers,
      NJT_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("referer_hash_max_size"),
      NJT_HTTP_SRV_CONF|NJT_HTTP_LOC_CONF|NJT_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NJT_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_referer_conf_t, referer_hash_max_size),
      NULL },

    { ngx_string("referer_hash_bucket_size"),
      NJT_HTTP_SRV_CONF|NJT_HTTP_LOC_CONF|NJT_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NJT_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_referer_conf_t, referer_hash_bucket_size),
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_referer_module_ctx = {
    ngx_http_referer_add_variables,        /* preconfiguration */
    NULL,                                  /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_referer_create_conf,          /* create location configuration */
    ngx_http_referer_merge_conf            /* merge location configuration */
};


ngx_module_t  ngx_http_referer_module = {
    NJT_MODULE_V1,
    &ngx_http_referer_module_ctx,          /* module context */
    ngx_http_referer_commands,             /* module directives */
    NJT_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NJT_MODULE_V1_PADDING
};


static ngx_str_t  ngx_http_invalid_referer_name = ngx_string("invalid_referer");


static ngx_int_t
ngx_http_referer_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    u_char                    *p, *ref, *last;
    size_t                     len;
    ngx_str_t                 *uri;
    ngx_uint_t                 i, key;
    ngx_http_referer_conf_t   *rlcf;
    u_char                     buf[256];
#if (NJT_PCRE)
    ngx_int_t                  rc;
    ngx_str_t                  referer;
#endif

    rlcf = ngx_http_get_module_loc_conf(r, ngx_http_referer_module);

    if (rlcf->hash.hash.buckets == NULL
        && rlcf->hash.wc_head == NULL
        && rlcf->hash.wc_tail == NULL
#if (NJT_PCRE)
        && rlcf->regex == NULL
        && rlcf->server_name_regex == NULL
#endif
       )
    {
        goto valid;
    }

    if (r->headers_in.referer == NULL) {
        if (rlcf->no_referer) {
            goto valid;
        }

        goto invalid;
    }

    len = r->headers_in.referer->value.len;
    ref = r->headers_in.referer->value.data;

    if (len >= sizeof("http://i.ru") - 1) {
        last = ref + len;

        if (ngx_strncasecmp(ref, (u_char *) "http://", 7) == 0) {
            ref += 7;
            len -= 7;
            goto valid_scheme;

        } else if (ngx_strncasecmp(ref, (u_char *) "https://", 8) == 0) {
            ref += 8;
            len -= 8;
            goto valid_scheme;
        }
    }

    if (rlcf->blocked_referer) {
        goto valid;
    }

    goto invalid;

valid_scheme:

    i = 0;
    key = 0;

    for (p = ref; p < last; p++) {
        if (*p == '/' || *p == ':') {
            break;
        }

        if (i == 256) {
            goto invalid;
        }

        buf[i] = ngx_tolower(*p);
        key = ngx_hash(key, buf[i++]);
    }

    uri = ngx_hash_find_combined(&rlcf->hash, key, buf, p - ref);

    if (uri) {
        goto uri;
    }

#if (NJT_PCRE)

    if (rlcf->server_name_regex) {
        referer.len = p - ref;
        referer.data = buf;

        rc = ngx_regex_exec_array(rlcf->server_name_regex, &referer,
                                  r->connection->log);

        if (rc == NJT_OK) {
            goto valid;
        }

        if (rc == NJT_ERROR) {
            return rc;
        }

        /* NJT_DECLINED */
    }

    if (rlcf->regex) {
        referer.len = len;
        referer.data = ref;

        rc = ngx_regex_exec_array(rlcf->regex, &referer, r->connection->log);

        if (rc == NJT_OK) {
            goto valid;
        }

        if (rc == NJT_ERROR) {
            return rc;
        }

        /* NJT_DECLINED */
    }

#endif

invalid:

    *v = ngx_http_variable_true_value;

    return NJT_OK;

uri:

    for ( /* void */ ; p < last; p++) {
        if (*p == '/') {
            break;
        }
    }

    len = last - p;

    if (uri == NJT_HTTP_REFERER_NO_URI_PART) {
        goto valid;
    }

    if (len < uri->len || ngx_strncmp(uri->data, p, uri->len) != 0) {
        goto invalid;
    }

valid:

    *v = ngx_http_variable_null_value;

    return NJT_OK;
}


static ngx_int_t
ngx_http_referer_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var;

    var = ngx_http_add_variable(cf, &ngx_http_invalid_referer_name,
                                NJT_HTTP_VAR_CHANGEABLE);
    if (var == NULL) {
        return NJT_ERROR;
    }

    var->get_handler = ngx_http_referer_variable;

    return NJT_OK;
}


static void *
ngx_http_referer_create_conf(ngx_conf_t *cf)
{
    ngx_http_referer_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_referer_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->hash = { NULL };
     *     conf->server_names = 0;
     *     conf->keys = NULL;
     */

#if (NJT_PCRE)
    conf->regex = NJT_CONF_UNSET_PTR;
    conf->server_name_regex = NJT_CONF_UNSET_PTR;
#endif

    conf->no_referer = NJT_CONF_UNSET;
    conf->blocked_referer = NJT_CONF_UNSET;
    conf->referer_hash_max_size = NJT_CONF_UNSET_UINT;
    conf->referer_hash_bucket_size = NJT_CONF_UNSET_UINT;

    return conf;
}


static char *
ngx_http_referer_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_referer_conf_t *prev = parent;
    ngx_http_referer_conf_t *conf = child;

    ngx_uint_t                 n;
    ngx_hash_init_t            hash;
    ngx_http_server_name_t    *sn;
    ngx_http_core_srv_conf_t  *cscf;

    if (conf->keys == NULL) {
        conf->hash = prev->hash;

#if (NJT_PCRE)
        ngx_conf_merge_ptr_value(conf->regex, prev->regex, NULL);
        ngx_conf_merge_ptr_value(conf->server_name_regex,
                                 prev->server_name_regex, NULL);
#endif
        ngx_conf_merge_value(conf->no_referer, prev->no_referer, 0);
        ngx_conf_merge_value(conf->blocked_referer, prev->blocked_referer, 0);
        ngx_conf_merge_uint_value(conf->referer_hash_max_size,
                                  prev->referer_hash_max_size, 2048);
        ngx_conf_merge_uint_value(conf->referer_hash_bucket_size,
                                  prev->referer_hash_bucket_size, 64);

        return NJT_CONF_OK;
    }

    if (conf->server_names == 1) {
        cscf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_core_module);

        sn = cscf->server_names.elts;
        for (n = 0; n < cscf->server_names.nelts; n++) {

#if (NJT_PCRE)
            if (sn[n].regex) {

                if (ngx_http_add_regex_server_name(cf, conf, sn[n].regex)
                    != NJT_OK)
                {
                    return NJT_CONF_ERROR;
                }

                continue;
            }
#endif

            if (ngx_http_add_referer(cf, conf->keys, &sn[n].name, NULL)
                != NJT_OK)
            {
                return NJT_CONF_ERROR;
            }
        }
    }

    if ((conf->no_referer == 1 || conf->blocked_referer == 1)
        && conf->keys->keys.nelts == 0
        && conf->keys->dns_wc_head.nelts == 0
        && conf->keys->dns_wc_tail.nelts == 0)
    {
        ngx_log_error(NJT_LOG_EMERG, cf->log, 0,
                      "the \"none\" or \"blocked\" referers are specified "
                      "in the \"valid_referers\" directive "
                      "without any valid referer");
        return NJT_CONF_ERROR;
    }

    ngx_conf_merge_uint_value(conf->referer_hash_max_size,
                              prev->referer_hash_max_size, 2048);
    ngx_conf_merge_uint_value(conf->referer_hash_bucket_size,
                              prev->referer_hash_bucket_size, 64);
    conf->referer_hash_bucket_size = ngx_align(conf->referer_hash_bucket_size,
                                               ngx_cacheline_size);

    hash.key = ngx_hash_key_lc;
    hash.max_size = conf->referer_hash_max_size;
    hash.bucket_size = conf->referer_hash_bucket_size;
    hash.name = "referer_hash";
    hash.pool = cf->pool;

    if (conf->keys->keys.nelts) {
        hash.hash = &conf->hash.hash;
        hash.temp_pool = NULL;

        if (ngx_hash_init(&hash, conf->keys->keys.elts, conf->keys->keys.nelts)
            != NJT_OK)
        {
            return NJT_CONF_ERROR;
        }
    }

    if (conf->keys->dns_wc_head.nelts) {

        ngx_qsort(conf->keys->dns_wc_head.elts,
                  (size_t) conf->keys->dns_wc_head.nelts,
                  sizeof(ngx_hash_key_t),
                  ngx_http_cmp_referer_wildcards);

        hash.hash = NULL;
        hash.temp_pool = cf->temp_pool;

        if (ngx_hash_wildcard_init(&hash, conf->keys->dns_wc_head.elts,
                                   conf->keys->dns_wc_head.nelts)
            != NJT_OK)
        {
            return NJT_CONF_ERROR;
        }

        conf->hash.wc_head = (ngx_hash_wildcard_t *) hash.hash;
    }

    if (conf->keys->dns_wc_tail.nelts) {

        ngx_qsort(conf->keys->dns_wc_tail.elts,
                  (size_t) conf->keys->dns_wc_tail.nelts,
                  sizeof(ngx_hash_key_t),
                  ngx_http_cmp_referer_wildcards);

        hash.hash = NULL;
        hash.temp_pool = cf->temp_pool;

        if (ngx_hash_wildcard_init(&hash, conf->keys->dns_wc_tail.elts,
                                   conf->keys->dns_wc_tail.nelts)
            != NJT_OK)
        {
            return NJT_CONF_ERROR;
        }

        conf->hash.wc_tail = (ngx_hash_wildcard_t *) hash.hash;
    }

#if (NJT_PCRE)
    ngx_conf_merge_ptr_value(conf->regex, prev->regex, NULL);
    ngx_conf_merge_ptr_value(conf->server_name_regex, prev->server_name_regex,
                             NULL);
#endif

    if (conf->no_referer == NJT_CONF_UNSET) {
        conf->no_referer = 0;
    }

    if (conf->blocked_referer == NJT_CONF_UNSET) {
        conf->blocked_referer = 0;
    }

    conf->keys = NULL;

    return NJT_CONF_OK;
}


static char *
ngx_http_valid_referers(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_referer_conf_t  *rlcf = conf;

    u_char      *p;
    ngx_str_t   *value, uri;
    ngx_uint_t   i;

    if (rlcf->keys == NULL) {
        rlcf->keys = ngx_pcalloc(cf->temp_pool, sizeof(ngx_hash_keys_arrays_t));
        if (rlcf->keys == NULL) {
            return NJT_CONF_ERROR;
        }

        rlcf->keys->pool = cf->pool;
        rlcf->keys->temp_pool = cf->pool;

        if (ngx_hash_keys_array_init(rlcf->keys, NJT_HASH_SMALL) != NJT_OK) {
            return NJT_CONF_ERROR;
        }
    }

    value = cf->args->elts;

    for (i = 1; i < cf->args->nelts; i++) {
        if (value[i].len == 0) {
            ngx_conf_log_error(NJT_LOG_EMERG, cf, 0,
                               "invalid referer \"%V\"", &value[i]);
            return NJT_CONF_ERROR;
        }

        if (ngx_strcmp(value[i].data, "none") == 0) {
            rlcf->no_referer = 1;
            continue;
        }

        if (ngx_strcmp(value[i].data, "blocked") == 0) {
            rlcf->blocked_referer = 1;
            continue;
        }

        if (ngx_strcmp(value[i].data, "server_names") == 0) {
            rlcf->server_names = 1;
            continue;
        }

        if (value[i].data[0] == '~') {
            if (ngx_http_add_regex_referer(cf, rlcf, &value[i]) != NJT_OK) {
                return NJT_CONF_ERROR;
            }

            continue;
        }

        ngx_str_null(&uri);

        p = (u_char *) ngx_strchr(value[i].data, '/');

        if (p) {
            uri.len = (value[i].data + value[i].len) - p;
            uri.data = p;
            value[i].len = p - value[i].data;
        }

        if (ngx_http_add_referer(cf, rlcf->keys, &value[i], &uri) != NJT_OK) {
            return NJT_CONF_ERROR;
        }
    }

    return NJT_CONF_OK;
}


static ngx_int_t
ngx_http_add_referer(ngx_conf_t *cf, ngx_hash_keys_arrays_t *keys,
    ngx_str_t *value, ngx_str_t *uri)
{
    ngx_int_t   rc;
    ngx_str_t  *u;

    if (uri == NULL || uri->len == 0) {
        u = NJT_HTTP_REFERER_NO_URI_PART;

    } else {
        u = ngx_palloc(cf->pool, sizeof(ngx_str_t));
        if (u == NULL) {
            return NJT_ERROR;
        }

        *u = *uri;
    }

    rc = ngx_hash_add_key(keys, value, u, NJT_HASH_WILDCARD_KEY);

    if (rc == NJT_OK) {
        return NJT_OK;
    }

    if (rc == NJT_DECLINED) {
        ngx_conf_log_error(NJT_LOG_EMERG, cf, 0,
                           "invalid hostname or wildcard \"%V\"", value);
    }

    if (rc == NJT_BUSY) {
        ngx_conf_log_error(NJT_LOG_EMERG, cf, 0,
                           "conflicting parameter \"%V\"", value);
    }

    return NJT_ERROR;
}


static ngx_int_t
ngx_http_add_regex_referer(ngx_conf_t *cf, ngx_http_referer_conf_t *rlcf,
    ngx_str_t *name)
{
#if (NJT_PCRE)
    ngx_regex_elt_t      *re;
    ngx_regex_compile_t   rc;
    u_char                errstr[NJT_MAX_CONF_ERRSTR];

    if (name->len == 1) {
        ngx_conf_log_error(NJT_LOG_EMERG, cf, 0, "empty regex in \"%V\"", name);
        return NJT_ERROR;
    }

    if (rlcf->regex == NJT_CONF_UNSET_PTR) {
        rlcf->regex = ngx_array_create(cf->pool, 2, sizeof(ngx_regex_elt_t));
        if (rlcf->regex == NULL) {
            return NJT_ERROR;
        }
    }

    re = ngx_array_push(rlcf->regex);
    if (re == NULL) {
        return NJT_ERROR;
    }

    name->len--;
    name->data++;

    ngx_memzero(&rc, sizeof(ngx_regex_compile_t));

    rc.pattern = *name;
    rc.pool = cf->pool;
    rc.options = NJT_REGEX_CASELESS;
    rc.err.len = NJT_MAX_CONF_ERRSTR;
    rc.err.data = errstr;

    if (ngx_regex_compile(&rc) != NJT_OK) {
        ngx_conf_log_error(NJT_LOG_EMERG, cf, 0, "%V", &rc.err);
        return NJT_ERROR;
    }

    re->regex = rc.regex;
    re->name = name->data;

    return NJT_OK;

#else

    ngx_conf_log_error(NJT_LOG_EMERG, cf, 0,
                       "the using of the regex \"%V\" requires PCRE library",
                       name);

    return NJT_ERROR;

#endif
}


#if (NJT_PCRE)

static ngx_int_t
ngx_http_add_regex_server_name(ngx_conf_t *cf, ngx_http_referer_conf_t *rlcf,
    ngx_http_regex_t *regex)
{
    ngx_regex_elt_t  *re;

    if (rlcf->server_name_regex == NJT_CONF_UNSET_PTR) {
        rlcf->server_name_regex = ngx_array_create(cf->pool, 2,
                                                   sizeof(ngx_regex_elt_t));
        if (rlcf->server_name_regex == NULL) {
            return NJT_ERROR;
        }
    }

    re = ngx_array_push(rlcf->server_name_regex);
    if (re == NULL) {
        return NJT_ERROR;
    }

    re->regex = regex->regex;
    re->name = regex->name.data;

    return NJT_OK;
}

#endif


static int ngx_libc_cdecl
ngx_http_cmp_referer_wildcards(const void *one, const void *two)
{
    ngx_hash_key_t  *first, *second;

    first = (ngx_hash_key_t *) one;
    second = (ngx_hash_key_t *) two;

    return ngx_dns_strcmp(first->key.data, second->key.data);
}
