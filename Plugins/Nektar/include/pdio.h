/*
 * PDIO client library definitions
 *
 * Authors: Nathan Stone
 *
 * Copyright 2005, Pittsburgh Supercomputing Center (PSC),
 * an organizational unit of Carnegie Mellon University.
 *
 * Permission  to use  and copy  this  software  and its documentation
 * without  fee for personal  use or use  within your  organization is
 * hereby  granted,  provided  that  the  above  copyright  notice  is
 * preserved in all copies and that that copyright and this permission
 * notice  appear  in  any  supporting  documentation.  Permission  to
 * redistribute this software to other organizations or individuals is
 * NOT  granted;  that  must be negotiated  with the PSC.  Neither the
 * PSC nor Carnegie Mellon University  make any  representations about
 * the suitability of this software  for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * $Id: pdio.h,v 1.9 2005/09/27 03:28:01 nstone Exp $
 */

#ifndef _pdio_h
#define _pdio_h

#include <sys/param.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

/* PDIO users must fill out the appropriate values in this struct
 * then supply the resulting struct to pdio_init(), and others */
typedef struct pdio_info {
  char   host[MAXHOSTNAMELEN];   /* remote TCP target hostname */
  int    port;                   /* remote TCP target port # */
  int    nStreams;               /* number of TCP streams to use between I/O daemon and remote target host */
  size_t minTCPchunk;            /* minimum number of bytes for TCP network buffers */
  size_t maxTCPchunk;            /* maximum number of bytes for TCP network buffers */
  size_t TCPRingSize;            /* total size of the TCP Ring Buffer memory across all PDIO daemons */
  int    nWriters;               /* number of compute PEs that will write via PDIO */
  size_t writesize;              /* max write size that each PE will write via PDIO (same for all PEs) */
  char   rfile_name[MAXPATHLEN]; /* remote file name */
  size_t readsize;               /* max read size */

  /* !!! internal use only !!! */
  int    rank;
  int    size;
  int    nid;
  int    pid;
  int    jobid;
  size_t offset;
  size_t len;
} pdio_info_t;

/* must be called _by_every_PE_ that will use PDIO */
int pdio_init(pdio_info_t *info);

/* write the data to the remote stream
 * !!! offset is currently ignored !!!
 * info pointer may be null, if not specifying any relevant values
 * returns the number of bytes successfully written,
 * or negative value for failure */
ssize_t pdio_write(void *buf, size_t len, size_t offset, pdio_info_t *info);

/* read data from the remote stream
 * info pointer may be null, if not specifying any relevant values
 * returns the number of bytes successfully read
 * or negative value for failure */
ssize_t pdio_read(void *buf, size_t len, pdio_info_t *info);

/* must be called _by_every_PE_ (that called pdio_init) */
int pdio_fini(void);


#ifdef ENABLE_MULTIWRITE_SYNCHRONIZATION
int pdio_begin_iteration(void);

int pdio_end_iteration(void);
#endif

#endif /* _pdio_h */
