/****************************************************************************
 * NCSA HDF                                                                 *
 * Software Development Group                                               *
 * National Center for Supercomputing Applications                          *
 * University of Illinois at Urbana-Champaign                               *
 * 605 E. Springfield, Champaign IL 61820                                   *
 *                                                                          *
 * For conditions of distribution and use, see the accompanying             *
 * hdf/COPYING file.                                                        *
 *                                                                          *
 ****************************************************************************/

/*
 *  Header file for error values, etc.
 */
#ifndef _H5Eprivate_H
#define _H5Eprivate_H

#include "H5Epublic.h"

/* Private headers needed by this file */
#include "H5private.h"

#define H5E_NSLOTS      32      /*number of slots in an error stack          */

/*
 * HERROR macro, used to facilitate error reporting between a FUNC_ENTER()
 * and a FUNC_LEAVE() within a function body.  The arguments are the major
 * error number, the minor error number, and a description of the error.
 */
#define HERROR(maj, min, str) H5E_push(maj, min, FUNC, __FILE__, __LINE__, str)

/*
 * HRETURN_ERROR macro, used to facilitate error reporting between a
 * FUNC_ENTER() and a FUNC_LEAVE() within a function body.  The arguments are
 * the major error number, the minor error number, a return value, and a
 * description of the error.
 */
#define HRETURN_ERROR(maj, min, ret_val, str) {                               \
   HERROR (maj, min, str);                                                    \
   if (H5_IS_API(FUNC) && H5E_auto_g) {                                       \
       (H5E_auto_g)(H5E_auto_data_g);                                         \
   }                                                                          \
   HRETURN(ret_val);                                                          \
}

/*
 * HRETURN macro, used to facilitate returning from a function between a
 * FUNC_ENTER() and a FUNC_LEAVE() within a function body.  The argument is
 * the return value.
 */
#define HRETURN(ret_val) {                                                    \
   PABLO_TRACE_OFF (PABLO_MASK, pablo_func_id);                               \
   H5TRACE_RETURN(ret_val);                                                   \
   H5_API_UNLOCK_BEGIN                                                        \
   H5_API_UNLOCK_END                                                          \
   H5_API_SET_CANCEL                                                          \
   return (ret_val);                                                          \
}

/*
 * HGOTO_ERROR macro, used to facilitate error reporting between a
 * FUNC_ENTER() and a FUNC_LEAVE() within a function body.  The arguments are
 * the major error number, the minor error number, the return value, and an
 * error string.  The return value is assigned to a variable `ret_value' and
 * control branches to the `done' label.
 */
#define HGOTO_ERROR(maj, min, ret_val, str) {                                 \
   HERROR (maj, min,  str);                                                   \
   if (H5_IS_API(FUNC) && H5E_auto_g) {                                       \
       (H5E_auto_g)(H5E_auto_data_g);                                         \
   }                                                                          \
   ret_value = ret_val;                                                       \
   goto done;                                                                 \
}

/*
 * HGOTO_DONE macro, used to facilitate normal return between a FUNC_ENTER()
 * and a FUNC_LEAVE() within a function body. The argument is the return
 * value which is assigned to the `ret_value' variable.  Control branches to
 * the `done' label.
 */
#define HGOTO_DONE(ret_val) {ret_value = ret_val; goto done;}

/*
 * The list of error messages in the system is kept as an array of
 * error_code/message pairs, one for major error numbers and another for
 * minor error numbers.
 */
typedef struct H5E_major_mesg_t {
    H5E_major_t error_code;
    const char  *str;
} H5E_major_mesg_t;

typedef struct H5E_minor_mesg_t {
    H5E_minor_t error_code;
    const char  *str;
} H5E_minor_mesg_t;

/* An error stack */
typedef struct H5E_t {
    int nused;                  /*num slots currently used in stack  */
    H5E_error_t slot[H5E_NSLOTS];       /*array of error records             */
} H5E_t;

__DLLVAR__ const hbool_t H5E_clearable_g;/*safe to call H5E_clear() on enter?*/
__DLLVAR__ herr_t (*H5E_auto_g)(void *client_data);
__DLLVAR__ void *H5E_auto_data_g;

__DLL__ herr_t H5E_push (H5E_major_t maj_num, H5E_minor_t min_num,
                         const char *func_name, const char *file_name,
                         unsigned line, const char *desc);
__DLL__ herr_t H5E_clear (void);
__DLL__ herr_t H5E_walk (H5E_direction_t dir, H5E_walk_t func,
                         void *client_data);
#endif
