/*-------------------------------------------------------------------------
 * Copyright (C) 2000   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             H5TSprivate.h
 *                      May 2 2000
 *                      Chee Wai LEE
 *
 * Purpose:             Private non-prototype header.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef H5TSprivate_H_
#define H5TSprivate_H_

/* Public headers needed by this file */
#ifdef LATER
#include "H5TSpublic.h"         /*Public API prototypes */
#endif /* LATER */

/* Library level data structures */

typedef struct H5TS_mutex_struct {
    pthread_t *owner_thread;            /* current lock owner */
    pthread_mutex_t atomic_lock;        /* lock for atomicity of new mechanism */
    pthread_cond_t cond_var;            /* condition variable */
    unsigned int lock_count;
} H5TS_mutex_t;

/* Extern global variables */
extern pthread_once_t H5TS_first_init_g;
extern pthread_key_t H5TS_errstk_key_g;

#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif  /* c_plusplus || __cplusplus */

__DLL__ void H5TS_first_thread_init(void);
__DLL__ herr_t H5TS_mutex_lock(H5TS_mutex_t *mutex);
__DLL__ herr_t H5TS_mutex_unlock(H5TS_mutex_t *mutex);
__DLL__ herr_t H5TS_cancel_count_inc(void);
__DLL__ herr_t H5TS_cancel_count_dec(void);

#if defined c_plusplus || defined __cplusplus
}
#endif  /* c_plusplus || __cplusplus */

#endif  /* H5TSprivate_H_ */
