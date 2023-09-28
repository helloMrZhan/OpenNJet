
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 * Copyright (C) 2021-2023  TMLake(Beijing) Technology Co., Ltd.
 */


#include <njt_config.h>
#include <njt_core.h>

#define NJT_CONF_BUFFER  4096

static njt_int_t njt_conf_add_dump(njt_conf_t *cf, njt_str_t *filename);
static njt_int_t njt_conf_handler(njt_conf_t *cf, njt_int_t last);
static njt_int_t njt_conf_read_token(njt_conf_t *cf);
static njt_int_t njt_conf_element_handler(njt_conf_t *cf, njt_int_t rc); // by lcm
static void njt_conf_flush_files(njt_cycle_t *cycle);


static njt_command_t  njt_conf_commands[] = {

    { njt_string("include"),
      NJT_ANY_CONF|NJT_CONF_TAKE1,
      njt_conf_include,
      0,
      0,
      NULL },

      njt_null_command
};


njt_module_t  njt_conf_module = {
    NJT_MODULE_V1,
    NULL,                                  /* module context */
    njt_conf_commands,                     /* module directives */
    NJT_CONF_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    njt_conf_flush_files,                  /* exit process */
    NULL,                                  /* exit master */
    NJT_MODULE_V1_PADDING
};


/* The eight fixed arguments */

static njt_uint_t argument_number[] = {
    NJT_CONF_NOARGS,
    NJT_CONF_TAKE1,
    NJT_CONF_TAKE2,
    NJT_CONF_TAKE3,
    NJT_CONF_TAKE4,
    NJT_CONF_TAKE5,
    NJT_CONF_TAKE6,
    NJT_CONF_TAKE7
};


char *
njt_conf_param(njt_conf_t *cf)
{
    char              *rv;
    njt_str_t         *param;
    njt_buf_t          b;
    njt_conf_file_t    conf_file;

    param = &cf->cycle->conf_param;

    if (param->len == 0) {
        return NJT_CONF_OK;
    }

    njt_memzero(&conf_file, sizeof(njt_conf_file_t));

    njt_memzero(&b, sizeof(njt_buf_t));

    b.start = param->data;
    b.pos = param->data;
    b.last = param->data + param->len;
    b.end = b.last;
    b.temporary = 1;

    conf_file.file.fd = NJT_INVALID_FILE;
    conf_file.file.name.data = NULL;
    conf_file.line = 0;

    cf->conf_file = &conf_file;
    cf->conf_file->buffer = &b;

    
    rv = njt_conf_parse(cf, NULL);

    cf->conf_file = NULL;

    return rv;
}


static njt_int_t
njt_conf_add_dump(njt_conf_t *cf, njt_str_t *filename)
{
    off_t             size;
    u_char           *p;
    uint32_t          hash;
    njt_buf_t        *buf;
    njt_str_node_t   *sn;
    njt_conf_dump_t  *cd;
 //byg zyg
 #if (NJT_HTTP_DYNAMIC_LOC)
	if(cf->dynamic == 1){
           cf->conf_file->dump = NULL;
	   return NJT_OK;
	}
 #endif
    hash = njt_crc32_long(filename->data, filename->len);

    sn = njt_str_rbtree_lookup(&cf->cycle->config_dump_rbtree, filename, hash);

    if (sn) {
        cf->conf_file->dump = NULL;
        return NJT_OK;
    }

    p = njt_pstrdup(cf->cycle->pool, filename);
    if (p == NULL) {
        return NJT_ERROR;
    }

    cd = njt_array_push(&cf->cycle->config_dump);
    if (cd == NULL) {
        return NJT_ERROR;
    }

    size = njt_file_size(&cf->conf_file->file.info);

    buf = njt_create_temp_buf(cf->cycle->pool, (size_t) size);
    if (buf == NULL) {
        return NJT_ERROR;
    }

    cd->name.data = p;
    cd->name.len = filename->len;
    cd->buffer = buf;

    cf->conf_file->dump = buf;

    sn = njt_palloc(cf->temp_pool, sizeof(njt_str_node_t));
    if (sn == NULL) {
        return NJT_ERROR;
    }

    sn->node.key = hash;
    sn->str = cd->name;

    njt_rbtree_insert(&cf->cycle->config_dump_rbtree, &sn->node);

    return NJT_OK;
}


char *
njt_conf_parse(njt_conf_t *cf, njt_str_t *filename)
{
    char             *rv;
    njt_fd_t          fd;
    njt_int_t         rc;
    njt_buf_t         buf;
    njt_conf_file_t  *prev, conf_file;
    enum {
        parse_file = 0,
        parse_block,
        parse_param
    } type;

#if (NJT_SUPPRESS_WARN)
    fd = NJT_INVALID_FILE;
    prev = NULL;
#endif

    if (filename) {

        /* open configuration file */

        fd = njt_open_file(filename->data, NJT_FILE_RDONLY, NJT_FILE_OPEN, 0);

        if (fd == NJT_INVALID_FILE) {
            njt_conf_log_error(NJT_LOG_EMERG, cf, njt_errno,
                               njt_open_file_n " \"%s\" failed",
                               filename->data);
            return NJT_CONF_ERROR;
        }

        prev = cf->conf_file;

        cf->conf_file = &conf_file;

        if (njt_fd_info(fd, &cf->conf_file->file.info) == NJT_FILE_ERROR) {
            njt_log_error(NJT_LOG_EMERG, cf->log, njt_errno,
                          njt_fd_info_n " \"%s\" failed", filename->data);
        }

        cf->conf_file->buffer = &buf;

        buf.start = njt_alloc(NJT_CONF_BUFFER, cf->log);
        if (buf.start == NULL) {
            goto failed;
        }
        //by cheng xu
#if (NJT_DEBUG)
        njt_log_debug0(NJT_LOG_DEBUG_CORE, cf->log, 0,"alloc cf->conf_file->buffer ");
#endif
        //end
        buf.pos = buf.start;
        buf.last = buf.start;
        buf.end = buf.last + NJT_CONF_BUFFER;
        buf.temporary = 1;

        cf->conf_file->file.fd = fd;
        cf->conf_file->file.name.len = filename->len;
        cf->conf_file->file.name.data = filename->data;
        cf->conf_file->file.offset = 0;
        cf->conf_file->file.log = cf->log;
        cf->conf_file->line = 1;

        type = parse_file;

        if (njt_dump_config
#if (NJT_DEBUG)
            || 1
#endif
           )
        {
            if (njt_conf_add_dump(cf, filename) != NJT_OK) {
                goto failed;
            }

        } else {
            cf->conf_file->dump = NULL;
        }

    } else if (cf->conf_file->file.fd != NJT_INVALID_FILE) {

        type = parse_block;

    } else {
        type = parse_param;
    }

 
    for ( ;; ) {
        rc = njt_conf_read_token(cf);

        /*
         * njt_conf_read_token() may return
         *
         *    NJT_ERROR             there is error
         *    NJT_OK                the token terminated by ";" was found
         *    NJT_CONF_BLOCK_START  the token terminated by "{" was found
         *    NJT_CONF_BLOCK_DONE   the "}" was found
         *    NJT_CONF_FILE_DONE    the configuration file is done
         */

        if (rc == NJT_ERROR) {
            goto done;
        }

        // add by lcm
        for (int i = 0; i < (int)cf->args->nelts; i++) {
            if (i == 0) {
                printf("\"%s\": ", ((njt_str_t*)cf->args->elts)[i].data);
            } else {
                printf("\"%s\" ",  ((njt_str_t*)cf->args->elts)[i].data);
            }
        }
        printf("\n");

        if (njt_process == NJT_PROCESS_SINGLE) {
            if (njt_conf_element_handler(cf, rc) != NJT_OK) {
                printf("error occured \n");
            }
        }
        // end of add


        if (rc == NJT_CONF_BLOCK_DONE) {

            if (type != parse_block) {
                njt_conf_log_error(NJT_LOG_EMERG, cf, 0, "unexpected \"}\"");
                goto failed;
            }

            printf("}\n");
            printf("%ld, AAAAA } \n", cf->args->nelts);
            goto done;
        }

        if (rc == NJT_CONF_FILE_DONE) {

            if (type == parse_block) {
                njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                   "unexpected end of file, expecting \"}\"");
                goto failed;
            }

            goto done;
        }

        if (rc == NJT_CONF_BLOCK_START) {

            if (type == parse_param) {
                njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                   "block directives are not supported "
                                   "in -g option");
                goto failed;
            }
            printf("{\n");
        }

        /* rc == NJT_OK || rc == NJT_CONF_BLOCK_START */

        if (cf->handler) {

            /*
             * the custom handler, i.e., that is used in the http's
             * "types { ... }" directive
             */

            if (rc == NJT_CONF_BLOCK_START) {
                njt_conf_log_error(NJT_LOG_EMERG, cf, 0, "unexpected \"{\"");
                goto failed;
            }

            rv = (*cf->handler)(cf, NULL, cf->handler_conf);
            if (rv == NJT_CONF_OK) {
                continue;
            }

            if (rv == NJT_CONF_ERROR) {
                goto failed;
            }

            njt_conf_log_error(NJT_LOG_EMERG, cf, 0, "%s", rv);

            goto failed;
        }


        rc = njt_conf_handler(cf, rc);

        if (rc == NJT_ERROR) {
            goto failed;
        }
    }

failed:

    rc = NJT_ERROR;

done:

    if (filename) {
        if (cf->conf_file->buffer->start) {
            njt_free(cf->conf_file->buffer->start);
        }

        if (njt_close_file(fd) == NJT_FILE_ERROR) {
            njt_log_error(NJT_LOG_ALERT, cf->log, njt_errno,
                          njt_close_file_n " %s failed",
                          filename->data);
            rc = NJT_ERROR;
        }

        cf->conf_file = prev;
    }

    if (rc == NJT_ERROR) {
        return NJT_CONF_ERROR;
    }

    return NJT_CONF_OK;
}

static njt_int_t njt_conf_cmd_set_args(njt_conf_t *cf, njt_conf_cmd_t *ccmd){
    njt_uint_t      i, j;
    njt_uint_t      need_escape;
    njt_pool_t     *pool = cf->cycle->pool;
    njt_str_t      *arg, *value;
    size_t          vlen;
    char           *cur, *dst;
    njt_array_t    *pos, *tmp;

    pos = njt_array_push(ccmd->value);
    tmp = njt_array_create(pool, 1, sizeof(njt_str_t));
    if (pos == NULL || tmp == NULL) {
        return NJT_ERROR;
    }
    njt_memcpy(pos, tmp, sizeof(njt_array_t));

    if (cf->args->nelts == 1) {
        value = njt_array_push(pos);
        value->data = njt_palloc(pool, 1);
        if (value->data == NULL) {
            return NJT_ERROR;
        }
        njt_str_set(value, ""); // value->len = 0;
        return NJT_OK;
    }


    for (i = 1; i < cf->args->nelts; i++) {
        value = njt_array_push(pos);
        if (value == NULL) {
            return NJT_ERROR;
        }
        arg = &((njt_str_t*)cf->args->elts)[i];
        need_escape = 0;
        for (j = 0; j < arg->len; j++) {
            switch ((char)arg->data[j]) {
                case '\\': 
                // case '/':  need_convert = true; break;
                case '"':  
                case '\b':  
                case '\f':  
                case '\n':  
                case '\r':  
                case '\t':  need_escape ++; break;
                default:
                     break;
            }
        }

        vlen = need_escape + arg->len;
        value->data = njt_palloc(cf->cycle->pool, vlen);
        if (value->data == NULL) {
            return NJT_ERROR;
        }

        value->len = vlen;
        dst = (char*)value->data;
        cur = (char*)arg->data;

        if (!need_escape) {
            for (j = 0; j < arg->len; j++, cur++) {
                *dst++ = *cur;
            }
        } else {
            for (j = 0; j < arg->len; j++, cur++) {
                switch (*cur) {
                    case '"':  *dst++ = '\\'; *dst++ = '"'; break;
                    case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
                    // case '/':  *dst++ = '\\'; *dst++ = '/'; out->len++; break;
                    case '\b': *dst++ = '\\'; *dst++ = 'b'; break;
                    case '\f': *dst++ = '\\'; *dst++ = 'f'; break;
                    case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
                    case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
                    case '\t': *dst++ = '\\'; *dst++ = 't'; break;
                    default:
                        *dst++ = *cur;
                }
            }
        }
    }

    return NJT_OK;
}

void njt_conf_get_json_length(njt_conf_element_t *root, size_t *length, njt_uint_t is_root) {
    njt_uint_t      i, j, k;
    njt_conf_cmd_t *cmd;
    njt_array_t    *values;
    njt_str_t      *arg;
    njt_conf_block_t    *blocks;
    njt_conf_element_t      *block;

    *length += 1;

    if (root->block_name != NULL) {
        *length += 7; // "_key":  "block_name",
        cmd = root->block_name;
        if (cmd->value->nelts == 1) {
            // "key": "value",
            values = cmd->value->elts;
            if (values->nelts == 1) {
                // all as string now, change to boolean or number later
                arg = (njt_str_t*)values->elts;// "key": "value",
                *length += arg->len + 2;
            } else {
                *length += 1; 
                for (k = 0; k < values->nelts; k++) {
                    arg = &((njt_str_t*)values->elts)[k];
                    *length += arg->len + 3;
                }
            }
        }
        *length += 1; 
    }
      

    for (i = 0; i < root->cmds->nelts; i++) {
        cmd = &((njt_conf_cmd_t*)root->cmds->elts)[i];
        *length += cmd->key.len + 3;
        if (cmd->value->nelts == 1) {
            // "key": "value",
            values = cmd->value->elts;
            if (values->nelts == 1) {
                // all as string now, change to boolean or number later
                arg = (njt_str_t*)values->elts;// "key": "value",
                *length += arg->len + 2;
            } else {
                *length += 1; 
                for (k = 0; k < values->nelts; k++) {
                    arg = &((njt_str_t*)values->elts)[k];
                    *length += arg->len + 3;
                }
            }
        } else {
            *length += 1;
            for (j = 0; j < cmd->value->nelts; j++) {
                *length += 1;
                values = &((njt_array_t*)cmd->value->elts)[j];
                for (k = 0; k < values->nelts; k++) {
                    arg = &((njt_str_t*)values->elts)[k];
                    *length += arg->len + 3;
                }
                *length += 1;
            }
        }
        *length += 1;
    }

    if (root->blocks == NULL) {
        return;
    }

    for (i = 0; i < root->blocks->nelts; i++) {
        blocks = &((njt_conf_block_t*)root->blocks->elts)[i];
        printf("length: %ld    key: %s\n", *length, (char*)blocks->key.data);
        *length += blocks->key.len + 3;
        if (blocks->value->nelts == 1) {
            block = blocks->value->elts; // http : {}
            njt_conf_get_json_length(block, length, 0);
        } else { // locaction [{location1}, {location2}, ...]
            *length += 1;
            for (j = 0; j < blocks->value->nelts; j++) {
                block = &((njt_conf_element_t*)blocks->value->elts)[j];
                njt_conf_get_json_length(block, length, 0);
                *length += 1;
            }
        }
        *length += 1;
    }
}

void njt_conf_get_json_str(njt_conf_element_t *root, njt_str_t *out, njt_uint_t is_root) {
    njt_uint_t             i, j, k;
    njt_conf_cmd_t        *cmd;
    njt_array_t           *values;
    njt_str_t             *arg;
    njt_conf_block_t      *blocks;
    njt_conf_element_t    *block;
    u_char *dst;


    dst = out->data + out->len;
    *dst++ = '{'; out->len++;

    if (root->block_name != NULL) {
        dst = njt_sprintf(dst, "\"_key\":"); //"_key":  "block_name",
        cmd = root->block_name;
        if (cmd->value->nelts == 1) {
            // "key": "value",
            values = cmd->value->elts;
            if (values->nelts == 1) {
                // all as string now, change to boolean or number later
                arg = (njt_str_t*)values->elts;// "key": "value",
                dst = njt_sprintf(dst, "\"%V\"", arg);
            } else {
                *dst++ = '['; 
                for (k = 0; k < values->nelts; k++) {
                    arg = &((njt_str_t*)values->elts)[k];
                    dst = njt_sprintf(dst, "\"%V\",", arg);
                }
                dst--; // last ,
                *dst++ = ']'; 
            }
        }
        *dst++ = ','; 
    }

    for (i = 0; i < root->cmds->nelts; i++) {
        cmd = &((njt_conf_cmd_t*)root->cmds->elts)[i];
        dst = njt_sprintf(dst, "\"%V\":", &cmd->key); // "key":
        if (cmd->value->nelts == 1) {
            // "key": "value",
            values = cmd->value->elts;
            if (values->nelts == 1) {
                // all as string now, change to boolean or number later
                arg = (njt_str_t*)values->elts;// "key": "value",
                dst = njt_sprintf(dst, "\"%V\"", arg);
            } else {
                *dst++ = '['; 
                for (k = 0; k < values->nelts; k++) {
                    arg = &((njt_str_t*)values->elts)[k];
                    dst = njt_sprintf(dst, "\"%V\",", arg);
                }
                dst--; // last ,
                *dst++ = ']'; 
                out->len = dst - out->data;
            }
        } else {
            *dst++ = '['; 
            for (j = 0; j < cmd->value->nelts; j++) {
                *dst++ = '['; 
                values = &((njt_array_t*)cmd->value->elts)[j];
                for (k = 0; k < values->nelts; k++) {
                    arg = &((njt_str_t*)values->elts)[k];
                    dst = njt_sprintf(dst, "\"%V\",", arg);
                }
                dst--; // last ,
                *dst++ = ']'; 
                *dst++ = ','; 
                out->len = dst - out->data;
            }
            dst--; // last ,
            *dst++ = ']'; 
        }
        *dst++ = ','; 
    }
    out->len = dst - out->data;

    if (root->blocks == NULL) {
        dst--;
        *dst++ = '}'; 
        out->len = dst - out->data;
        return;
    }

    for (i = 0; i < root->blocks->nelts; i++) {
        dst = out->data + out->len;
        blocks = &((njt_conf_block_t*)root->blocks->elts)[i];
        dst = njt_sprintf(dst, "\"%V\":", &blocks->key); // "key":
        out->len = dst - out->data;
        if (blocks->value->nelts == 1) {
            // http : {}
            block = blocks->value->elts;
            njt_conf_get_json_str(block, out, 0);
        } else {
            *dst++ = '['; 
            out->len = dst - out->data;
            for (j = 0; j < blocks->value->nelts; j++) {
                block = &((njt_conf_element_t*)blocks->value->elts)[j];
                njt_conf_get_json_str(block, out, 0);
                dst = out->data + out->len;
                *dst++ = ','; 
                out->len = dst - out->data;
            }
            dst = out->data + out->len;
            dst--; // last ,
            *dst++ = ']'; 
        }
        dst = out->data + out->len;
        *dst++ = ','; 
        out->len = dst - out->data;
    }
    
    dst = out->data + out->len;
    dst--;
    *dst++ = '}'; 
    out->len = dst - out->data;
}

static njt_int_t
njt_conf_element_handler(njt_conf_t *cf, njt_int_t rc)
{
    // njt_uint_t         i;
    njt_uint_t         j, found; // , found_cmd;
    njt_str_t          *name;
    // njt_command_t      *cmd;
    njt_conf_element_t *cur;
    njt_conf_element_t *new_block, *bpos;
    njt_conf_block_t   *block;
    njt_conf_cmd_t     *ccmd; 
    njt_pool_t         *pool = cf->cycle->pool; // TODO CHECK!!!

    cur = (njt_conf_element_t*)njt_conf_cur;
    // // all conditions will be checked in njt_conf_handler() 
    if (rc == NJT_CONF_BLOCK_DONE) {
        njt_conf_cur = cur->parent;
        return NJT_OK;
    }

    if (rc == NJT_CONF_FILE_DONE) {
        return NJT_OK;
    }

    name = cf->args->elts;
    // found_cmd = 0;

    // for (i = 0; cf->cycle->modules[i]; i++) {

    //     cmd = cf->cycle->modules[i]->commands;
    //     if (cmd == NULL) {
    //         continue;
    //     }

    //     for ( /* void */ ; cmd->name.len; cmd++) {

    //         if (name->len != cmd->name.len) {
    //             continue;
    //         }

    //         if (njt_strcmp(name->data, cmd->name.data) != 0) {
    //             continue;
    //         }


    //         if (cf->cycle->modules[i]->type != NJT_CONF_MODULE
    //             && cf->cycle->modules[i]->type != cf->module_type)
    //         {
    //             continue;
    //         }

    //         /* is the directive's location right ? */

    //         if (!(cmd->type & cf->cmd_type)) {
    //             continue;
    //         }
    //         found_cmd = 1;
    //         break;
    //     }
    //     if (found_cmd) {
    //         break;
    //     }
    // }
    // return NJT_OK;
    found = 0;
    if (rc == NJT_OK)
    {
        if (!cur->cmds)
        {
            cur->cmds = njt_array_create(pool, 1, sizeof(njt_conf_cmd_t));
            if (!cur->cmds)
            {
                return NJT_ERROR;
            }
        }
        // find cmd by key
        for (j = 0; j < cur->cmds->nelts; j++)
        {
            ccmd = &((njt_conf_cmd_t *)(cur->cmds->elts))[j];
            if (ccmd->key.len != name->len)
            {
                continue;
            }

            // 这里之前用njt_strcmp出过错误，有时ccmd->key_data的len之后不是\0
            if (njt_memcmp(name->data, ccmd->key.data, name->len) != 0)
            {
                continue;
            }

            found = 1;
            break;
        }

        if (!found)
        {
            ccmd = njt_array_push(cur->cmds);
            if (ccmd == NULL) {
                return NJT_ERROR;
            }
            ccmd->key.data = njt_palloc(pool, name->len);
            if (ccmd->key.data == NULL) {
                return NJT_ERROR;
            }
            ccmd->key.len = name->len;
            njt_memcpy(ccmd->key.data, name->data, name->len);
            ccmd->value = njt_array_create(pool, 1, sizeof(njt_array_t));
            // ccmd->is_num = 0;
            // ccmd->is_flag = 0;
        }
        // set value to ccmd
        if (njt_conf_cmd_set_args(cf, ccmd) == NJT_ERROR) {
            return NJT_ERROR;
        }
        // if (found_cmd) {
        //     if (cmd->type & NJT_CONF_FLAG) {
        //         // ccmd->is_flag = 1;
        //     }
        // }
        return NJT_OK;
    }

    if (rc == NJT_CONF_BLOCK_START) {
        new_block = njt_pcalloc(pool, sizeof(njt_conf_element_t));
        if (new_block == NULL) {
            return NJT_ERROR;
        }
        if (cf->args->nelts > 1) {
            new_block->block_name = njt_palloc(pool, sizeof(njt_conf_cmd_t));
            if (new_block->block_name == NULL) {
                return NJT_ERROR;
            }
            new_block->block_name->value = njt_array_create(pool, 1, sizeof(njt_array_t));
            if (new_block->block_name->value == NULL) {
                return NJT_ERROR;
            }
            if (njt_conf_cmd_set_args(cf, new_block->block_name) == NJT_ERROR) {
                return NJT_ERROR;
            }
        }

        if (!cur->blocks)
        {
            cur->blocks = njt_array_create(pool, 1, sizeof(njt_conf_block_t));
            if (!cur->blocks)
            {
                return NJT_ERROR;
            }
        }
        // find cmd by key
        for (j = 0; j < cur->blocks->nelts; j++)
        {
            block = &((njt_conf_block_t *)(cur->blocks->elts))[j];
            if (block->key.len != name->len)
            {
                continue;
            }

            if (njt_memcmp(name->data, block->key.data, name->len) != 0)
            {
                continue;
            }

            found = 1;
            break;
        }

        if (!found) {
            block = njt_array_push(cur->blocks);
            if (block == NULL) {
                return NJT_ERROR;
            }
            block->key.data = njt_palloc(pool, name->len);
            if (block->key.data == NULL) {
                return NJT_ERROR;
            }
            block->key.len = name->len;
            njt_memcpy(block->key.data, name->data, name->len);
            block->value = njt_array_create(pool, 1, sizeof(njt_conf_element_t));
            if (block->value == NULL) {
                return NJT_ERROR;
            }
        }

        bpos = njt_array_push(block->value);
        if (bpos == NULL) {
            return NJT_ERROR;
        }
        njt_memcpy(bpos, new_block, sizeof(njt_conf_element_t));
        bpos->parent = cur;
        njt_conf_cur = bpos;
        
        return NJT_OK;
    }

    return NJT_ERROR;
}

static njt_int_t
njt_conf_handler(njt_conf_t *cf, njt_int_t last)
{
    char           *rv;
    void           *conf, **confp;
    njt_uint_t      i, found;
    njt_str_t      *name;
    njt_command_t  *cmd;

    name = cf->args->elts;

    found = 0;

    for (i = 0; cf->cycle->modules[i]; i++) {

        cmd = cf->cycle->modules[i]->commands;
        if (cmd == NULL) {
            continue;
        }

        for ( /* void */ ; cmd->name.len; cmd++) {

            if (name->len != cmd->name.len) {
                continue;
            }

            if (njt_strcmp(name->data, cmd->name.data) != 0) {
                continue;
            }

            found = 1;

            if (cf->cycle->modules[i]->type != NJT_CONF_MODULE
                && cf->cycle->modules[i]->type != cf->module_type)
            {
                continue;
            }

            /* is the directive's location right ? */

            if (!(cmd->type & cf->cmd_type)) {
                continue;
            }

            if (!(cmd->type & NJT_CONF_BLOCK) && last != NJT_OK) {
                njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                  "directive \"%s\" is not terminated by \";\"",
                                  name->data);
                return NJT_ERROR;
            }

            if ((cmd->type & NJT_CONF_BLOCK) && last != NJT_CONF_BLOCK_START) {
                njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                   "directive \"%s\" has no opening \"{\"",
                                   name->data);
                return NJT_ERROR;
            }

            /* is the directive's argument count right ? */

            if (!(cmd->type & NJT_CONF_ANY)) {

                if (cmd->type & NJT_CONF_FLAG) {

                    if (cf->args->nelts != 2) {
                        goto invalid;
                    }

                } else if (cmd->type & NJT_CONF_1MORE) {

                    if (cf->args->nelts < 2) {
                        goto invalid;
                    }

                } else if (cmd->type & NJT_CONF_2MORE) {

                    if (cf->args->nelts < 3) {
                        goto invalid;
                    }

                } else if (cf->args->nelts > NJT_CONF_MAX_ARGS) {

                    goto invalid;

                } else if (!(cmd->type & argument_number[cf->args->nelts - 1]))
                {
                    goto invalid;
                }
            }

            /* set up the directive's configuration context */

            conf = NULL;

            if (cmd->type & NJT_DIRECT_CONF) {
                conf = ((void **) cf->ctx)[cf->cycle->modules[i]->index];

            } else if (cmd->type & NJT_MAIN_CONF) {
                conf = &(((void **) cf->ctx)[cf->cycle->modules[i]->index]);

            } else if (cf->ctx) {
                confp = *(void **) ((char *) cf->ctx + cmd->conf);

                if (confp) {
                    conf = confp[cf->cycle->modules[i]->ctx_index];
                }
            }

            rv = cmd->set(cf, cmd, conf);

            if (rv == NJT_CONF_OK) {
                return NJT_OK;
            }

            if (rv == NJT_CONF_ERROR) {
                return NJT_ERROR;
            }

            njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                               "\"%s\" directive %s", name->data, rv);

            return NJT_ERROR;
        }
    }

    if (found) {
        njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                           "\"%s\" directive is not allowed here", name->data);

        return NJT_ERROR;
    }

    njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                       "unknown directive \"%s\"", name->data);

    return NJT_ERROR;

invalid:

    njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                       "invalid number of arguments in \"%s\" directive",
                       name->data);

    return NJT_ERROR;
}


static njt_int_t
njt_conf_read_token(njt_conf_t *cf)
{
    u_char      *start, ch, *src, *dst;
    off_t        file_size;
    size_t       len;
    ssize_t      n, size;
    njt_uint_t   found, need_space, last_space, sharp_comment, variable;
    njt_uint_t   quoted, s_quoted, d_quoted, start_line;
    njt_str_t   *word;
    njt_buf_t   *b, *dump;

    found = 0;
    need_space = 0;
    last_space = 1;
    sharp_comment = 0;
    variable = 0;
    quoted = 0;
    s_quoted = 0;
    d_quoted = 0;

    cf->args->nelts = 0;
    b = cf->conf_file->buffer;
    dump = cf->conf_file->dump;
    start = b->pos;
    start_line = cf->conf_file->line;

    file_size = njt_file_size(&cf->conf_file->file.info);

    for ( ;; ) {

        if (b->pos >= b->last) {

            if (cf->conf_file->file.offset >= file_size) {

                if (cf->args->nelts > 0 || !last_space) {

                    if (cf->conf_file->file.fd == NJT_INVALID_FILE) {
                        njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                           "unexpected end of parameter, "
                                           "expecting \";\"");
                        return NJT_ERROR;
                    }

                    njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                  "unexpected end of file, "
                                  "expecting \";\" or \"}\"");
                    return NJT_ERROR;
                }

                return NJT_CONF_FILE_DONE;
            }

            len = b->pos - start;

            if (len == NJT_CONF_BUFFER) {
                cf->conf_file->line = start_line;

                if (d_quoted) {
                    ch = '"';

                } else if (s_quoted) {
                    ch = '\'';

                } else {
                    njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                       "too long parameter \"%*s...\" started",
                                       10, start);
                    return NJT_ERROR;
                }

                njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                   "too long parameter, probably "
                                   "missing terminating \"%c\" character", ch);
                return NJT_ERROR;
            }

            if (len) {
                njt_memmove(b->start, start, len);
            }

            size = (ssize_t) (file_size - cf->conf_file->file.offset);

            if (size > b->end - (b->start + len)) {
                size = b->end - (b->start + len);
            }

            n = njt_read_file(&cf->conf_file->file, b->start + len, size,
                              cf->conf_file->file.offset);

            if (n == NJT_ERROR) {
                return NJT_ERROR;
            }

            if (n != size) {
                njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                   njt_read_file_n " returned "
                                   "only %z bytes instead of %z",
                                   n, size);
                return NJT_ERROR;
            }

            b->pos = b->start + len;
            b->last = b->pos + n;
            start = b->start;

            if (dump) {
                dump->last = njt_cpymem(dump->last, b->pos, size);
            }
        }

        ch = *b->pos++;

        if (ch == LF) {
            cf->conf_file->line++;

            if (sharp_comment) {
                sharp_comment = 0;
            }
        }

        if (sharp_comment) {
            continue;
        }

        if (quoted) {
            quoted = 0;
            continue;
        }

        if (need_space) {
            if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
                last_space = 1;
                need_space = 0;
                continue;
            }

            if (ch == ';') {
                return NJT_OK;
            }

            if (ch == '{') {
                return NJT_CONF_BLOCK_START;
            }

            if (ch == ')') {
                last_space = 1;
                need_space = 0;

            } else {
                njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                   "unexpected \"%c\"", ch);
                return NJT_ERROR;
            }
        }

        if (last_space) {

            start = b->pos - 1;
            start_line = cf->conf_file->line;

            if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
                continue;
            }

            switch (ch) {

            case ';':
            case '{':
                if (cf->args->nelts == 0) {
                    njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                       "unexpected \"%c\"", ch);
                    return NJT_ERROR;
                }

                if (ch == '{') {
                    return NJT_CONF_BLOCK_START;
                }

                return NJT_OK;

            case '}':
                if (cf->args->nelts != 0) {
                    njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                       "unexpected \"}\"");
                    return NJT_ERROR;
                }

                return NJT_CONF_BLOCK_DONE;

            case '#':
                sharp_comment = 1;
                continue;

            case '\\':
                quoted = 1;
                last_space = 0;
                continue;

            case '"':
                start++;
                d_quoted = 1;
                last_space = 0;
                continue;

            case '\'':
                start++;
                s_quoted = 1;
                last_space = 0;
                continue;

            case '$':
                variable = 1;
                last_space = 0;
                continue;

            default:
                last_space = 0;
            }

        } else {
            if (ch == '{' && variable) {
                continue;
            }

            variable = 0;

            if (ch == '\\') {
                quoted = 1;
                continue;
            }

            if (ch == '$') {
                variable = 1;
                continue;
            }

            if (d_quoted) {
                if (ch == '"') {
                    d_quoted = 0;
                    need_space = 1;
                    found = 1;
                }

            } else if (s_quoted) {
                if (ch == '\'') {
                    s_quoted = 0;
                    need_space = 1;
                    found = 1;
                }

            } else if (ch == ' ' || ch == '\t' || ch == CR || ch == LF
                       || ch == ';' || ch == '{')
            {
                last_space = 1;
                found = 1;
            }

            if (found) {
                word = njt_array_push(cf->args);
                if (word == NULL) {
                    return NJT_ERROR;
                }
                word->data = njt_pnalloc(cf->pool, b->pos - 1 - start + 1);
                if (word->data == NULL) {
                    return NJT_ERROR;
                }

                for (dst = word->data, src = start, len = 0;
                     src < b->pos - 1;
                     len++)
                {
                    if (*src == '\\') {
                        switch (src[1]) {
                        case '"':
                        case '\'':
                        case '\\':
                            src++;
                            break;

                        case 't':
                            *dst++ = '\t';
                            src += 2;
                            continue;

                        case 'r':
                            *dst++ = '\r';
                            src += 2;
                            continue;

                        case 'n':
                            *dst++ = '\n';
                            src += 2;
                            continue;
                        }

                    }
                    *dst++ = *src++;
                }
                *dst = '\0';
                word->len = len;

                if (ch == ';') {
                    return NJT_OK;
                }

                if (ch == '{') {
                    return NJT_CONF_BLOCK_START;
                }

                found = 0;
            }
        }
    }
}


char *
njt_conf_include(njt_conf_t *cf, njt_command_t *cmd, void *conf)
{
    char        *rv;
    njt_int_t    n;
    njt_str_t   *value, file, name;
    njt_glob_t   gl;

    value = cf->args->elts;
    file = value[1];

    njt_log_debug1(NJT_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);

    if (njt_conf_full_name(cf->cycle, &file, 1) != NJT_OK) {
        return NJT_CONF_ERROR;
    }

    if (strpbrk((char *) file.data, "*?[") == NULL) {

        njt_log_debug1(NJT_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);

        return njt_conf_parse(cf, &file);
    }

    njt_memzero(&gl, sizeof(njt_glob_t));

    gl.pattern = file.data;
    gl.log = cf->log;
    gl.test = 1;

    if (njt_open_glob(&gl) != NJT_OK) {
        njt_conf_log_error(NJT_LOG_EMERG, cf, njt_errno,
                           njt_open_glob_n " \"%s\" failed", file.data);
        return NJT_CONF_ERROR;
    }

    rv = NJT_CONF_OK;

    for ( ;; ) {
        n = njt_read_glob(&gl, &name);

        if (n != NJT_OK) {
            break;
        }

        file.len = name.len++;
        file.data = njt_pstrdup(cf->pool, &name);
        if (file.data == NULL) {
            return NJT_CONF_ERROR;
        }

        njt_log_debug1(NJT_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);

        rv = njt_conf_parse(cf, &file);

        if (rv != NJT_CONF_OK) {
            break;
        }
    }

    njt_close_glob(&gl);

    return rv;
}


njt_int_t
njt_conf_full_name(njt_cycle_t *cycle, njt_str_t *name, njt_uint_t conf_prefix)
{
    njt_str_t  *prefix;

    prefix = conf_prefix ? &cycle->conf_prefix : &cycle->prefix;

    return njt_get_full_name(cycle->pool, prefix, name);
}


njt_open_file_t *
njt_conf_open_file(njt_cycle_t *cycle, njt_str_t *name)
{
    njt_str_t         full;
    njt_uint_t        i;
    njt_list_part_t  *part;
    njt_open_file_t  *file;

#if (NJT_SUPPRESS_WARN)
    njt_str_null(&full);
#endif

    if (name->len) {
        full = *name;

        if (njt_conf_full_name(cycle, &full, 0) != NJT_OK) {
            return NULL;
        }

        part = &cycle->open_files.part;
        file = part->elts;

        for (i = 0; /* void */ ; i++) {

            if (i >= part->nelts) {
                if (part->next == NULL) {
                    break;
                }
                part = part->next;
                file = part->elts;
                i = 0;
            }

            if (full.len != file[i].name.len) {
                continue;
            }

            if (njt_strcmp(full.data, file[i].name.data) == 0) {
                return &file[i];
            }
        }
    }

    file = njt_list_push(&cycle->open_files);
    if (file == NULL) {
        return NULL;
    }

    if (name->len) {
        file->fd = NJT_INVALID_FILE;
        file->name = full;

    } else {
        file->fd = njt_stderr;
        file->name = *name;
    }

    file->flush = NULL;
    file->data = NULL;

    return file;
}


static void
njt_conf_flush_files(njt_cycle_t *cycle)
{
    njt_uint_t        i;
    njt_list_part_t  *part;
    njt_open_file_t  *file;

    njt_log_debug0(NJT_LOG_DEBUG_CORE, cycle->log, 0, "flush files");

    part = &cycle->open_files.part;
    file = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            file = part->elts;
            i = 0;
        }

        if (file[i].flush) {
            file[i].flush(&file[i], cycle->log);
        }
    }
}


void njt_cdecl
njt_conf_log_error(njt_uint_t level, njt_conf_t *cf, njt_err_t err,
    const char *fmt, ...)
{
    u_char   errstr[NJT_MAX_CONF_ERRSTR], *p, *last;
    va_list  args;

    last = errstr + NJT_MAX_CONF_ERRSTR;

    va_start(args, fmt);
    p = njt_vslprintf(errstr, last, fmt, args);
    va_end(args);

    if (err) {
        p = njt_log_errno(p, last, err);
    }

    if (cf->conf_file == NULL) {
        njt_log_error(level, cf->log, 0, "%*s", p - errstr, errstr);
        return;
    }

    if (cf->conf_file->file.fd == NJT_INVALID_FILE) {
        njt_log_error(level, cf->log, 0, "%*s in command line",
                      p - errstr, errstr);
        return;
    }
    njt_log_error(level, cf->log, 0, "%*s in %s:%ui",
                  p - errstr, errstr,
                  cf->conf_file->file.name.data, cf->conf_file->line);
    //by zyg	
    if(cf->errstr){
    	p = njt_snprintf(cf->errstr->data,cf->errstr->len,"%*s in %s:%ui",p - errstr, errstr,cf->conf_file->file.name.data, cf->conf_file->line);
    	cf->errstr->len = p - cf->errstr->data;
    }
    //end
}


char *
njt_conf_set_flag_slot(njt_conf_t *cf, njt_command_t *cmd, void *conf)
{
    char  *p = conf;

    njt_str_t        *value;
    njt_flag_t       *fp;
    njt_conf_post_t  *post;

    fp = (njt_flag_t *) (p + cmd->offset);

    if (*fp != NJT_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;

    if (njt_strcasecmp(value[1].data, (u_char *) "on") == 0) {
        *fp = 1;

    } else if (njt_strcasecmp(value[1].data, (u_char *) "off") == 0) {
        *fp = 0;

    } else {
        njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                     "invalid value \"%s\" in \"%s\" directive, "
                     "it must be \"on\" or \"off\"",
                     value[1].data, cmd->name.data);
        return NJT_CONF_ERROR;
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, fp);
    }

    return NJT_CONF_OK;
}


char *
njt_conf_set_str_slot(njt_conf_t *cf, njt_command_t *cmd, void *conf)
{
    char  *p = conf;

    njt_str_t        *field, *value;
    njt_conf_post_t  *post;

    field = (njt_str_t *) (p + cmd->offset);

    if (field->data) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *field = value[1];

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, field);
    }

    return NJT_CONF_OK;
}


char *
njt_conf_set_str_array_slot(njt_conf_t *cf, njt_command_t *cmd, void *conf)
{
    char  *p = conf;

    njt_str_t         *value, *s;
    njt_array_t      **a;
    njt_conf_post_t   *post;

    a = (njt_array_t **) (p + cmd->offset);

    if (*a == NJT_CONF_UNSET_PTR) {
        *a = njt_array_create(cf->pool, 4, sizeof(njt_str_t));
        if (*a == NULL) {
            return NJT_CONF_ERROR;
        }
    }

    s = njt_array_push(*a);
    if (s == NULL) {
        return NJT_CONF_ERROR;
    }

    value = cf->args->elts;

    *s = value[1];

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, s);
    }

    return NJT_CONF_OK;
}


char *
njt_conf_set_keyval_slot(njt_conf_t *cf, njt_command_t *cmd, void *conf)
{
    char  *p = conf;

    njt_str_t         *value;
    njt_array_t      **a;
    njt_keyval_t      *kv;
    njt_conf_post_t   *post;

    a = (njt_array_t **) (p + cmd->offset);

    if (*a == NJT_CONF_UNSET_PTR || *a == NULL) {
        *a = njt_array_create(cf->pool, 4, sizeof(njt_keyval_t));
        if (*a == NULL) {
            return NJT_CONF_ERROR;
        }
    }

    kv = njt_array_push(*a);
    if (kv == NULL) {
        return NJT_CONF_ERROR;
    }

    value = cf->args->elts;

    kv->key = value[1];
    kv->value = value[2];

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, kv);
    }

    return NJT_CONF_OK;
}


char *
njt_conf_set_num_slot(njt_conf_t *cf, njt_command_t *cmd, void *conf)
{
    char  *p = conf;

    njt_int_t        *np;
    njt_str_t        *value;
    njt_conf_post_t  *post;


    np = (njt_int_t *) (p + cmd->offset);

    if (*np != NJT_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;
    *np = njt_atoi(value[1].data, value[1].len);
    if (*np == NJT_ERROR) {
        return "invalid number";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, np);
    }

    return NJT_CONF_OK;
}


char *
njt_conf_set_size_slot(njt_conf_t *cf, njt_command_t *cmd, void *conf)
{
    char  *p = conf;

    size_t           *sp;
    njt_str_t        *value;
    njt_conf_post_t  *post;


    sp = (size_t *) (p + cmd->offset);
    if (*sp != NJT_CONF_UNSET_SIZE) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *sp = njt_parse_size(&value[1]);
    if (*sp == (size_t) NJT_ERROR) {
        return "invalid value";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, sp);
    }

    return NJT_CONF_OK;
}


char *
njt_conf_set_off_slot(njt_conf_t *cf, njt_command_t *cmd, void *conf)
{
    char  *p = conf;

    off_t            *op;
    njt_str_t        *value;
    njt_conf_post_t  *post;


    op = (off_t *) (p + cmd->offset);
    if (*op != NJT_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *op = njt_parse_offset(&value[1]);
    if (*op == (off_t) NJT_ERROR) {
        return "invalid value";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, op);
    }

    return NJT_CONF_OK;
}


char *
njt_conf_set_msec_slot(njt_conf_t *cf, njt_command_t *cmd, void *conf)
{
    char  *p = conf;

    njt_msec_t       *msp;
    njt_str_t        *value;
    njt_conf_post_t  *post;


    msp = (njt_msec_t *) (p + cmd->offset);
    if (*msp != NJT_CONF_UNSET_MSEC) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *msp = njt_parse_time(&value[1], 0);
    if (*msp == (njt_msec_t) NJT_ERROR) {
        return "invalid value";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, msp);
    }

    return NJT_CONF_OK;
}


char *
njt_conf_set_sec_slot(njt_conf_t *cf, njt_command_t *cmd, void *conf)
{
    char  *p = conf;

    time_t           *sp;
    njt_str_t        *value;
    njt_conf_post_t  *post;


    sp = (time_t *) (p + cmd->offset);
    if (*sp != NJT_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *sp = njt_parse_time(&value[1], 1);
    if (*sp == (time_t) NJT_ERROR) {
        return "invalid value";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, sp);
    }

    return NJT_CONF_OK;
}


char *
njt_conf_set_bufs_slot(njt_conf_t *cf, njt_command_t *cmd, void *conf)
{
    char *p = conf;

    njt_str_t   *value;
    njt_bufs_t  *bufs;


    bufs = (njt_bufs_t *) (p + cmd->offset);
    if (bufs->num) {
        return "is duplicate";
    }

    value = cf->args->elts;

    bufs->num = njt_atoi(value[1].data, value[1].len);
    if (bufs->num == NJT_ERROR || bufs->num == 0) {
        return "invalid value";
    }

    bufs->size = njt_parse_size(&value[2]);
    if (bufs->size == (size_t) NJT_ERROR || bufs->size == 0) {
        return "invalid value";
    }

    return NJT_CONF_OK;
}


char *
njt_conf_set_enum_slot(njt_conf_t *cf, njt_command_t *cmd, void *conf)
{
    char  *p = conf;

    njt_uint_t       *np, i;
    njt_str_t        *value;
    njt_conf_enum_t  *e;

    np = (njt_uint_t *) (p + cmd->offset);

    if (*np != NJT_CONF_UNSET_UINT) {
        return "is duplicate";
    }

    value = cf->args->elts;
    e = cmd->post;

    for (i = 0; e[i].name.len != 0; i++) {
        if (e[i].name.len != value[1].len
            || njt_strcasecmp(e[i].name.data, value[1].data) != 0)
        {
            continue;
        }

        *np = e[i].value;

        return NJT_CONF_OK;
    }

    njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                       "invalid value \"%s\"", value[1].data);

    return NJT_CONF_ERROR;
}


char *
njt_conf_set_bitmask_slot(njt_conf_t *cf, njt_command_t *cmd, void *conf)
{
    char  *p = conf;

    njt_uint_t          *np, i, m;
    njt_str_t           *value;
    njt_conf_bitmask_t  *mask;


    np = (njt_uint_t *) (p + cmd->offset);
    value = cf->args->elts;
    mask = cmd->post;

    for (i = 1; i < cf->args->nelts; i++) {
        for (m = 0; mask[m].name.len != 0; m++) {

            if (mask[m].name.len != value[i].len
                || njt_strcasecmp(mask[m].name.data, value[i].data) != 0)
            {
                continue;
            }

            if (*np & mask[m].mask) {
                njt_conf_log_error(NJT_LOG_WARN, cf, 0,
                                   "duplicate value \"%s\"", value[i].data);

            } else {
                *np |= mask[m].mask;
            }

            break;
        }

        if (mask[m].name.len == 0) {
            njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                               "invalid value \"%s\"", value[i].data);

            return NJT_CONF_ERROR;
        }
    }

    return NJT_CONF_OK;
}


#if 0

char *
njt_conf_unsupported(njt_conf_t *cf, njt_command_t *cmd, void *conf)
{
    return "unsupported on this platform";
}

#endif


char *
njt_conf_deprecated(njt_conf_t *cf, void *post, void *data)
{
    njt_conf_deprecated_t  *d = post;

    njt_conf_log_error(NJT_LOG_WARN, cf, 0,
                       "the \"%s\" directive is deprecated, "
                       "use the \"%s\" directive instead",
                       d->old_name, d->new_name);

    return NJT_CONF_OK;
}


char *
njt_conf_check_num_bounds(njt_conf_t *cf, void *post, void *data)
{
    njt_conf_num_bounds_t  *bounds = post;
    njt_int_t  *np = data;

    if (bounds->high == -1) {
        if (*np >= bounds->low) {
            return NJT_CONF_OK;
        }

        njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                           "value must be equal to or greater than %i",
                           bounds->low);

        return NJT_CONF_ERROR;
    }

    if (*np >= bounds->low && *np <= bounds->high) {
        return NJT_CONF_OK;
    }

    njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                       "value must be between %i and %i",
                       bounds->low, bounds->high);

    return NJT_CONF_ERROR;
}

njt_int_t
njt_conf_read_memory_token(njt_conf_t *cf,njt_str_t data)
{
    u_char      *start, ch, *src, *dst;
    size_t       len;
    njt_uint_t   found, need_space, last_space, sharp_comment, variable;
    njt_uint_t   quoted, s_quoted, d_quoted;
    njt_str_t   *word;
    njt_buf_t   *b,  new_buf;

    found = 0;
    need_space = 0;
    last_space = 1;
    sharp_comment = 0;
    variable = 0;
    quoted = 0;
    s_quoted = 0;
    d_quoted = 0;

    cf->args->nelts = 0;
    njt_memzero(&new_buf,sizeof(new_buf));

    b = &new_buf;
    new_buf.start = data.data;
    new_buf.end = data.data + data.len;

    new_buf.pos = new_buf.start;
    new_buf.last = new_buf.end;
    start = b->pos;


    for ( ;; ) {

        if (b->pos >= b->last) {
	    if(start <= b->last) {
		word = njt_array_push(cf->args);
                if (word == NULL) {
                    return NJT_ERROR;
                }
                word->data = njt_pnalloc(cf->pool, b->pos - 1 - start + 1);
                if (word->data == NULL) {
                    return NJT_ERROR;
                }

                for (dst = word->data, src = start, len = 0;
                     src < b->pos;
                     len++)
                {
                    if (*src == '\\') {
                        switch (src[1]) {
                        case '"':
                        case '\'':
                        case '\\':
                            src++;
                            break;

                        case 't':
                            *dst++ = '\t';
                            src += 2;
                            continue;

                        case 'r':
                            *dst++ = '\r';
                            src += 2;
                            continue;

                        case 'n':
                            *dst++ = '\n';
                            src += 2;
                            continue;
                        }

                    }
                    *dst++ = *src++;
                }
                *dst = '\0';
                word->len = len;
	    }
	    break;
        }

        ch = *b->pos++;

        if (ch == LF) {
            cf->conf_file->line++;

            if (sharp_comment) {
                sharp_comment = 0;
            }
        }

        if (sharp_comment) {
            continue;
        }

        if (quoted) {
            quoted = 0;
            continue;
        }

        if (need_space) {
            if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
                last_space = 1;
                need_space = 0;
                continue;
            }

            if (ch == ';') {
                return NJT_OK;
            }

            if (ch == '{') {
                return NJT_CONF_BLOCK_START;
            }

            if (ch == ')') {
                last_space = 1;
                need_space = 0;

            } else {
                njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                   "unexpected \"%c\"", ch);
                return NJT_ERROR;
            }
        }

        if (last_space) {

            start = b->pos - 1;

            if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
                continue;
            }

            switch (ch) {

            case ';':
            case '{':
                if (cf->args->nelts == 0) {
                    njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                       "unexpected \"%c\"", ch);
                    return NJT_ERROR;
                }

                if (ch == '{') {
                    return NJT_CONF_BLOCK_START;
                }

                return NJT_OK;

            case '}':
                if (cf->args->nelts != 0) {
                    njt_conf_log_error(NJT_LOG_EMERG, cf, 0,
                                       "unexpected \"}\"");
                    return NJT_ERROR;
                }

                return NJT_CONF_BLOCK_DONE;

            case '#':
                sharp_comment = 1;
                continue;

            case '\\':
                quoted = 1;
                last_space = 0;
                continue;

            case '"':
                start++;
                d_quoted = 1;
                last_space = 0;
                continue;

            case '\'':
                start++;
                s_quoted = 1;
                last_space = 0;
                continue;

            case '$':
                variable = 1;
                last_space = 0;
                continue;

            default:
                last_space = 0;
            }

        } else {
            if (ch == '{' && variable) {
                continue;
            }

            variable = 0;

            if (ch == '\\') {
                quoted = 1;
                continue;
            }

            if (ch == '$') {
                variable = 1;
                continue;
            }

            if (d_quoted) {
                if (ch == '"') {
                    d_quoted = 0;
                    need_space = 1;
                    found = 1;
                }

            } else if (s_quoted) {
                if (ch == '\'') {
                    s_quoted = 0;
                    need_space = 1;
                    found = 1;
                }

            } else if (ch == ' ' || ch == '\t' || ch == CR || ch == LF
                       || ch == ';' || ch == '{')
            {
                last_space = 1;
                found = 1;
            }

            if (found) {
                word = njt_array_push(cf->args);
                if (word == NULL) {
                    return NJT_ERROR;
                }
                word->data = njt_pnalloc(cf->pool, b->pos - 1 - start + 1);
                if (word->data == NULL) {
                    return NJT_ERROR;
                }

                for (dst = word->data, src = start, len = 0;
                     src < b->pos - 1;
                     len++)
                {
                    if (*src == '\\') {
                        switch (src[1]) {
                        case '"':
                        case '\'':
                        case '\\':
                            src++;
                            break;

                        case 't':
                            *dst++ = '\t';
                            src += 2;
                            continue;

                        case 'r':
                            *dst++ = '\r';
                            src += 2;
                            continue;

                        case 'n':
                            *dst++ = '\n';
                            src += 2;
                            continue;
                        }

                    }
                    *dst++ = *src++;
                }
                *dst = '\0';
                word->len = len;

                if (ch == ';') {
                    return NJT_OK;
                }

                if (ch == '{') {
                    return NJT_CONF_BLOCK_START;
                }

                found = 0;
            }
        }
    }
  return NJT_OK;
}
