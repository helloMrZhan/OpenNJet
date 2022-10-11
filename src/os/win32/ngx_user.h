
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NJT_USER_H_INCLUDED_
#define _NJT_USER_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/* STUB */
#define ngx_uid_t  ngx_int_t
#define ngx_gid_t  ngx_int_t


ngx_int_t ngx_libc_crypt(ngx_pool_t *pool, u_char *key, u_char *salt,
    u_char **encrypted);


#endif /* _NJT_USER_H_INCLUDED_ */
