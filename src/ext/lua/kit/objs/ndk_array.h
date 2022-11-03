
/* 
 * 2010 (C) Marcus Clyne 
 *
 * DO NOT EDIT THIS FILE MANUALLY
 * ------------------------------
 * This file has been generated automatically from scripts in the $base/auto dir and
 * data in the $base/auto/data dir. If you wish to edit the output of this file, then
 * you should edit these files instead.
 *
*/


/* Non-generated macros */

#define     ndk_array_count(a)                  ((a)->nelts)
#define     ndk_array_get_first(a)              ((a)->elts)
#define     ndk_array_get_index(a,n)            ((void*) ((char*) (a)->elts + (a)->size * n))
#define     ndk_array_get_last(a)               ((void*) ((char*) (a)->elts + (a)->size * ((a)->nelts - 1)))
#define     ndk_array_get_reverse_index(a,n)    ((void*) ((char*) (a)->elts + (a)->size * ((a)->nelts - 1 - n)))
#define     ndk_array_push_clean(p,a)           {p = njt_array_push (a); ndk_zerop (p);}


/* base action macro macros */

#define     ndk_array_create_ac(a,pl,n,sz,ac)   {a = njt_array_create (pl,n,sz); if (a == NULL) ac;}
#define     ndk_array_init_ac(a,pl,n,sz,ac)     {if (njt_array_init (a,pl,n,sz) == NJT_ERROR) ac;}
#define     ndk_array_push_ac(p,a,ac)           {p = njt_array_push (a);     if (p == NULL) ac;}
#define     ndk_array_push_clean_ac(p,a,ac)     {p = njt_array_push (a);     if (p == NULL) ac; ndk_zerop (p);}
#define     ndk_array_push_n_ac(p,a,n,ac)       {p = njt_array_push_n (a,n); if (p == NULL) ac;}
#define     ndk_array_push_n_clean_ac(p,a,n,ac) {p = njt_array_push_n (a,n); if (p == NULL) ac; ndk_zeropn (p,n);}


/* generated action macros */

#define     ndk_array_create_r0(a,pl,n,sz)      ndk_array_create_ac        (a,pl,n,sz,return 0)
#define     ndk_array_create_r1(a,pl,n,sz)      ndk_array_create_ac        (a,pl,n,sz,return 1)
#define     ndk_array_create_r_1(a,pl,n,sz)     ndk_array_create_ac        (a,pl,n,sz,return -1)
#define     ndk_array_create_rok(a,pl,n,sz)     ndk_array_create_ac        (a,pl,n,sz,return NJT_OK)
#define     ndk_array_create_rce(a,pl,n,sz)     ndk_array_create_ac        (a,pl,n,sz,return NJT_CONF_ERROR)
#define     ndk_array_create_rcok(a,pl,n,sz)    ndk_array_create_ac        (a,pl,n,sz,return NJT_CONF_OK)
#define     ndk_array_create_re(a,pl,n,sz)      ndk_array_create_ac        (a,pl,n,sz,return NJT_ERROR)
#define     ndk_array_create_rn(a,pl,n,sz)      ndk_array_create_ac        (a,pl,n,sz,return NULL)
#define     ndk_array_create_rse(a,pl,n,sz)     ndk_array_create_ac        (a,pl,n,sz,{njt_script_error (e); return;})
#define     ndk_array_create_sce(a,pl,n,sz)     ndk_array_create_ac        (a,pl,n,sz,{njt_script_configure_error (c); return;})
#define     ndk_array_create_g(a,pl,n,sz,_lb)   ndk_array_create_ac        (a,pl,n,sz,goto _lb)
#define     ndk_array_create_ge(a,pl,n,sz)      ndk_array_create_ac        (a,pl,n,sz,goto error)

#define     ndk_array_init_r0(a,pl,n,sz)        ndk_array_init_ac          (a,pl,n,sz,return 0)
#define     ndk_array_init_r1(a,pl,n,sz)        ndk_array_init_ac          (a,pl,n,sz,return 1)
#define     ndk_array_init_r_1(a,pl,n,sz)       ndk_array_init_ac          (a,pl,n,sz,return -1)
#define     ndk_array_init_rok(a,pl,n,sz)       ndk_array_init_ac          (a,pl,n,sz,return NJT_OK)
#define     ndk_array_init_rce(a,pl,n,sz)       ndk_array_init_ac          (a,pl,n,sz,return NJT_CONF_ERROR)
#define     ndk_array_init_rcok(a,pl,n,sz)      ndk_array_init_ac          (a,pl,n,sz,return NJT_CONF_OK)
#define     ndk_array_init_re(a,pl,n,sz)        ndk_array_init_ac          (a,pl,n,sz,return NJT_ERROR)
#define     ndk_array_init_rn(a,pl,n,sz)        ndk_array_init_ac          (a,pl,n,sz,return NULL)
#define     ndk_array_init_rse(a,pl,n,sz)       ndk_array_init_ac          (a,pl,n,sz,{njt_script_error (e); return;})
#define     ndk_array_init_sce(a,pl,n,sz)       ndk_array_init_ac          (a,pl,n,sz,{njt_script_configure_error (c); return;})
#define     ndk_array_init_g(a,pl,n,sz,_lb)     ndk_array_init_ac          (a,pl,n,sz,goto _lb)
#define     ndk_array_init_ge(a,pl,n,sz)        ndk_array_init_ac          (a,pl,n,sz,goto error)

#define     ndk_array_push_r0(p,a)              ndk_array_push_ac          (p,a,return 0)
#define     ndk_array_push_r1(p,a)              ndk_array_push_ac          (p,a,return 1)
#define     ndk_array_push_r_1(p,a)             ndk_array_push_ac          (p,a,return -1)
#define     ndk_array_push_rok(p,a)             ndk_array_push_ac          (p,a,return NJT_OK)
#define     ndk_array_push_rce(p,a)             ndk_array_push_ac          (p,a,return NJT_CONF_ERROR)
#define     ndk_array_push_rcok(p,a)            ndk_array_push_ac          (p,a,return NJT_CONF_OK)
#define     ndk_array_push_re(p,a)              ndk_array_push_ac          (p,a,return NJT_ERROR)
#define     ndk_array_push_rn(p,a)              ndk_array_push_ac          (p,a,return NULL)
#define     ndk_array_push_rse(p,a)             ndk_array_push_ac          (p,a,{njt_script_error (e); return;})
#define     ndk_array_push_sce(p,a)             ndk_array_push_ac          (p,a,{njt_script_configure_error (c); return;})
#define     ndk_array_push_g(p,a,_lb)           ndk_array_push_ac          (p,a,goto _lb)
#define     ndk_array_push_ge(p,a)              ndk_array_push_ac          (p,a,goto error)

#define     ndk_array_push_clean_r0(p,a)        ndk_array_push_clean_ac    (p,a,return 0)
#define     ndk_array_push_clean_r1(p,a)        ndk_array_push_clean_ac    (p,a,return 1)
#define     ndk_array_push_clean_r_1(p,a)       ndk_array_push_clean_ac    (p,a,return -1)
#define     ndk_array_push_clean_rok(p,a)       ndk_array_push_clean_ac    (p,a,return NJT_OK)
#define     ndk_array_push_clean_rce(p,a)       ndk_array_push_clean_ac    (p,a,return NJT_CONF_ERROR)
#define     ndk_array_push_clean_rcok(p,a)      ndk_array_push_clean_ac    (p,a,return NJT_CONF_OK)
#define     ndk_array_push_clean_re(p,a)        ndk_array_push_clean_ac    (p,a,return NJT_ERROR)
#define     ndk_array_push_clean_rn(p,a)        ndk_array_push_clean_ac    (p,a,return NULL)
#define     ndk_array_push_clean_rse(p,a)       ndk_array_push_clean_ac    (p,a,{njt_script_error (e); return;})
#define     ndk_array_push_clean_sce(p,a)       ndk_array_push_clean_ac    (p,a,{njt_script_configure_error (c); return;})
#define     ndk_array_push_clean_g(p,a,_lb)     ndk_array_push_clean_ac    (p,a,goto _lb)
#define     ndk_array_push_clean_ge(p,a)        ndk_array_push_clean_ac    (p,a,goto error)

#define     ndk_array_push_n_r0(p,a,n)          ndk_array_push_n_ac        (p,a,n,return 0)
#define     ndk_array_push_n_r1(p,a,n)          ndk_array_push_n_ac        (p,a,n,return 1)
#define     ndk_array_push_n_r_1(p,a,n)         ndk_array_push_n_ac        (p,a,n,return -1)
#define     ndk_array_push_n_rok(p,a,n)         ndk_array_push_n_ac        (p,a,n,return NJT_OK)
#define     ndk_array_push_n_rce(p,a,n)         ndk_array_push_n_ac        (p,a,n,return NJT_CONF_ERROR)
#define     ndk_array_push_n_rcok(p,a,n)        ndk_array_push_n_ac        (p,a,n,return NJT_CONF_OK)
#define     ndk_array_push_n_re(p,a,n)          ndk_array_push_n_ac        (p,a,n,return NJT_ERROR)
#define     ndk_array_push_n_rn(p,a,n)          ndk_array_push_n_ac        (p,a,n,return NULL)
#define     ndk_array_push_n_rse(p,a,n)         ndk_array_push_n_ac        (p,a,n,{njt_script_error (e); return;})
#define     ndk_array_push_n_sce(p,a,n)         ndk_array_push_n_ac        (p,a,n,{njt_script_configure_error (c); return;})
#define     ndk_array_push_n_g(p,a,n,_lb)       ndk_array_push_n_ac        (p,a,n,goto _lb)
#define     ndk_array_push_n_ge(p,a,n)          ndk_array_push_n_ac        (p,a,n,goto error)

#define     ndk_array_push_n_clean_r0(p,a,n)    ndk_array_push_n_clean_ac  (p,a,n,return 0)
#define     ndk_array_push_n_clean_r1(p,a,n)    ndk_array_push_n_clean_ac  (p,a,n,return 1)
#define     ndk_array_push_n_clean_r_1(p,a,n)   ndk_array_push_n_clean_ac  (p,a,n,return -1)
#define     ndk_array_push_n_clean_rok(p,a,n)   ndk_array_push_n_clean_ac  (p,a,n,return NJT_OK)
#define     ndk_array_push_n_clean_rce(p,a,n)   ndk_array_push_n_clean_ac  (p,a,n,return NJT_CONF_ERROR)
#define     ndk_array_push_n_clean_rcok(p,a,n)  ndk_array_push_n_clean_ac  (p,a,n,return NJT_CONF_OK)
#define     ndk_array_push_n_clean_re(p,a,n)    ndk_array_push_n_clean_ac  (p,a,n,return NJT_ERROR)
#define     ndk_array_push_n_clean_rn(p,a,n)    ndk_array_push_n_clean_ac  (p,a,n,return NULL)
#define     ndk_array_push_n_clean_rse(p,a,n)   ndk_array_push_n_clean_ac  (p,a,n,{njt_script_error (e); return;})
#define     ndk_array_push_n_clean_sce(p,a,n)   ndk_array_push_n_clean_ac  (p,a,n,{njt_script_configure_error (c); return;})
#define     ndk_array_push_n_clean_g(p,a,n,_lb) ndk_array_push_n_clean_ac  (p,a,n,goto _lb)
#define     ndk_array_push_n_clean_ge(p,a,n)    ndk_array_push_n_clean_ac  (p,a,n,goto error)

