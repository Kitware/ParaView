/*
 * Copyright © 2000 The author.
 * The author prefers this code not be used for military purposes.
 *
 *
 * Author:  Thomas Radke <tradke@aei-potsdam.mpg.de>
 *          Tuesday, September 12, 2000
 *
 * Purpose:     The public header file for the Stream Virtual File Driver.
 *
 * Version: Header
 *
 * Modifications:
 *          Thomas Radke, Thursday, October 26, 2000
 *          Added support for Windows.
 *
 */
#ifndef H5FDstream_H
#define H5FDstream_H

#ifdef H5_HAVE_STREAM

#include "H5Ipublic.h"

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

#define H5FD_STREAM     (H5FD_stream_init())

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
__DLL__ hid_t  H5FD_stream_init (void);
__DLL__ herr_t H5Pset_fapl_stream (hid_t fapl_id,
                                   H5FD_stream_fapl_t *fapl);
__DLL__ herr_t H5Pget_fapl_stream (hid_t fapl_id,
                                   H5FD_stream_fapl_t *fapl /*out*/ );

#ifdef __cplusplus
}
#endif

#endif /* H5_HAVE_STREAM */

#endif /* H5FDstream_H */
