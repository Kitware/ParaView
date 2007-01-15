/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdf.ncsa.uiuc.edu/HDF5/doc/Copyright.html.  If you do not have     *
 * access to either file, you may request a copy from hdfhelp@ncsa.uiuc.edu. *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Copyright © 2000 The author.
 * The author prefers this code not be used for military purposes.
 *
 *
 * Author:  Thomas Radke <tradke@aei-potsdam.mpg.de>
 *          Tuesday, September 12, 2000
 *
 * Purpose:  The public header file for the Stream Virtual File Driver.
 *
 * Modifications:
 *          Thomas Radke, Thursday, October 26, 2000
 *          Added support for Windows.
 *
 */
#ifndef H5FDstream_H
#define H5FDstream_H

#ifdef H5_HAVE_STREAM
#   define H5FD_STREAM  (H5FD_stream_init())
#else
#   define H5FD_STREAM (-1)
#endif /*H5_HAVE_STREAM */

#ifdef H5_HAVE_STREAM

/* check what sockets type we have (Unix or Windows sockets)
   Note that only MS compilers require to use Windows sockets
   but gcc under Windows does not. */
#if ! defined(H5_HAVE_WINSOCK_H) || defined(__GNUC__)
#define H5FD_STREAM_HAVE_UNIX_SOCKETS  1
#endif

/* define the data type for socket descriptors
   and the constant indicating an invalid descriptor */
#ifdef H5FD_STREAM_HAVE_UNIX_SOCKETS

#define H5FD_STREAM_SOCKET_TYPE            int
#define H5FD_STREAM_INVALID_SOCKET         -1

#else
#include <winsock.h>

#define H5FD_STREAM_SOCKET_TYPE            SOCKET
#define H5FD_STREAM_INVALID_SOCKET         INVALID_SOCKET

#endif

#ifdef __cplusplus
extern "C" {
#endif

/* prototype for read broadcast callback routine */
typedef int (*H5FD_stream_broadcast_t) (unsigned char **file,
                                        haddr_t *len,
                                        void *arg);

/* driver-specific file access properties */
typedef struct H5FD_stream_fapl_t
{
  size_t       increment;            /* how much to grow memory in reallocs  */
  H5FD_STREAM_SOCKET_TYPE socket;    /* externally provided socket descriptor*/
  hbool_t      do_socket_io;         /* do I/O on socket                     */
  int          backlog;              /* backlog argument for listen call     */
  H5FD_stream_broadcast_t broadcast_fn; /* READ broadcast callback           */
  void        *broadcast_arg;        /* READ broadcast callback user argument*/
  unsigned int maxhunt;              /* how many more ports to try to bind to*/
  unsigned short int port;           /* port a socket was bound/connected to */
} H5FD_stream_fapl_t;


/* prototypes of exported functions */
H5_DLL hid_t  H5FD_stream_init (void);
H5_DLL void H5FD_stream_term(void);
H5_DLL herr_t H5Pset_fapl_stream (hid_t fapl_id,
                                   H5FD_stream_fapl_t *fapl);
H5_DLL herr_t H5Pget_fapl_stream (hid_t fapl_id,
                                   H5FD_stream_fapl_t *fapl /*out*/ );

#ifdef __cplusplus
}
#endif

#endif /* H5_HAVE_STREAM */

#endif /* H5FDstream_H */
