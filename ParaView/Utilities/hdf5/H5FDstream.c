/*
 * Copyright © 2000 The author.
 * The author prefers this code not be used for military purposes.
 *                  
 *
 * Author:  Thomas Radke <tradke@aei-potsdam.mpg.de>
 *          Tuesday, September 12, 2000
 *
 * Purpose: This code provides the Stream Virtual File Driver.
 *          It is very much based on the core VFD which keeps an
 *          entire HDF5 data file to be processed in main memory.
 *          In addition to that, the memory image of the file is
 *          read from/written to a socket during an open/flush operation.
 *
 * Version: Id
 *
 * Modifications:
 *          Thomas Radke, Thursday, October 26, 2000
 *          Added support for Windows.
 *          Catch SIGPIPE on an open socket.
 *
 */

#include "H5public.h"                 /* H5_HAVE_STREAM                      */

/* Only build this driver if it was configured with --with-Stream-VFD */
#ifdef H5_HAVE_STREAM

#include "H5Eprivate.h"               /* error handling                      */
#include "H5FDpublic.h"               /* VFD structures                      */
#include "H5MMprivate.h"              /* memory allocation                   */
#include "H5Ppublic.h"                /* files                               */
#include "H5Pprivate.h"               /* VFD prototypes                      */
#include "H5FDstream.h"               /* Stream VFD header                   */

#ifdef H5FD_STREAM_HAVE_UNIX_SOCKETS
#ifdef H5_HAVE_SYS_TYPES_H
#include <sys/types.h>                /* socket stuff                        */
#endif
#ifdef H5_HAVE_SYS_SOCKET_H
#include <sys/socket.h>               /* socket stuff                        */
#endif
#include <netdb.h>                    /* gethostbyname                       */
#include <netinet/in.h>               /* socket stuff                        */
#ifdef H5_HAVE_NETINET_TCP_H
#include <netinet/tcp.h>              /* socket stuff                        */
#endif
#ifdef H5_HAVE_SYS_FILIO_H
#include <sys/filio.h>                /* socket stuff                        */
#endif
#endif

#ifndef H5_HAVE_SOCKLEN_T
typedef int socklen_t;
#endif

/* Some useful macros */
#ifdef  MIN
#undef  MIN
#endif
#ifdef  MAX
#undef  MAX
#endif
#define MIN(x,y)        ((x) < (y) ? (x) : (y))
#define MAX(x,y)        ((x) > (y) ? (x) : (y))

/* Uncomment this to switch on debugging output */
/* #define DEBUG 1 */

/* Define some socket stuff which is different for UNIX and Windows */
#ifdef H5FD_STREAM_HAVE_UNIX_SOCKETS
#define H5FD_STREAM_CLOSE_SOCKET(a)          close(a)
#define H5FD_STREAM_IOCTL_SOCKET(a, b, c)    ioctl(a, b, c)
#define H5FD_STREAM_ERROR_CHECK(rc)          ((rc) < 0)
#else
#define H5FD_STREAM_CLOSE_SOCKET(a)          closesocket (a)
#define H5FD_STREAM_IOCTL_SOCKET(a, b, c)    ioctlsocket (a, b, (u_long *) (c))
#define H5FD_STREAM_ERROR_CHECK(rc)          ((rc) == (SOCKET) (SOCKET_ERROR))
#endif


/* The driver identification number, initialized at runtime */
static hid_t H5FD_STREAM_g = 0;

/*
 * The description of a file belonging to this driver. The `eoa' and `eof'
 * determine the amount of hdf5 address space in use and the high-water mark
 * of the file (the current size of the underlying memory).
 */
typedef struct H5FD_stream_t
{
  H5FD_t             pub;             /* public stuff, must be first         */
  H5FD_stream_fapl_t fapl;            /* file access property list           */
  unsigned char     *mem;             /* the underlying memory               */
  haddr_t            eoa;             /* end of allocated region             */
  haddr_t            eof;             /* current allocated size              */
  H5FD_STREAM_SOCKET_TYPE socket;     /* socket to write / read from         */
  hbool_t            dirty;           /* flag indicating unflushed data      */
  hbool_t            internal_socket; /* flag indicating an internal socket  */
} H5FD_stream_t;

/* Allocate memory in multiples of this size (in bytes) by default */
#define H5FD_STREAM_INCREMENT         8192

/* default backlog argument for listen call */
#define H5FD_STREAM_BACKLOG           1

/* number of successive ports to hunt for until bind(2) succeeds
   (default 0 means no port hunting - only try the one given in the filename) */
#define H5FD_STREAM_MAXHUNT           0

/* default file access property list */
static const H5FD_stream_fapl_t default_fapl =
{
  H5FD_STREAM_INCREMENT,              /* address space allocation blocksize */
  H5FD_STREAM_INVALID_SOCKET,         /* no external socket descriptor      */
  TRUE,                               /* enable I/O on socket               */
  H5FD_STREAM_BACKLOG,                /* default backlog for listen(2)      */
  NULL,                               /* do not broadcast received files    */
  NULL,                               /* argument to READ broadcast routine */
  H5FD_STREAM_MAXHUNT,                /* default number of ports to hunt    */
  0                                   /* unknown port for unbound socket    */
};

/*
 * These macros check for overflow of various quantities.  These macros
 * assume that file_offset_t is signed and haddr_t and size_t are unsigned.
 * 
 * ADDR_OVERFLOW:        Checks whether a file address of type `haddr_t'
 *                        is too large to be represented by the second argument
 *                        of the file seek function.
 *
 * SIZE_OVERFLOW:        Checks whether a buffer size of type `hsize_t' is too
 *                        large to be represented by the `size_t' type.
 *
 * REGION_OVERFLOW:      Checks whether an address and size pair describe data
 *                        which can be addressed entirely in memory.
 */
#ifdef H5_HAVE_LSEEK64
#   define file_offset_t        off64_t
#else
#   define file_offset_t        off_t
#endif
#define MAXADDR                 (((haddr_t)1<<(8*sizeof(file_offset_t)-1))-1)
#define ADDR_OVERFLOW(A)        (HADDR_UNDEF==(A) ||                          \
                                 ((A) & ~(haddr_t)MAXADDR))
#define SIZE_OVERFLOW(Z)        ((Z) & ~(hsize_t)MAXADDR)
#define REGION_OVERFLOW(A,Z)    (ADDR_OVERFLOW(A) || SIZE_OVERFLOW(Z) ||      \
                                 HADDR_UNDEF==(A)+(Z) ||                      \
                                 (size_t)((A)+(Z))<(size_t)(A))

/* Function prototypes */
static void   *H5FD_stream_fapl_get (H5FD_t *_stream);
static H5FD_t *H5FD_stream_open (const char *name, unsigned flags,
                                 hid_t fapl_id, haddr_t maxaddr);
static herr_t  H5FD_stream_flush (H5FD_t *_stream);
static herr_t  H5FD_stream_close (H5FD_t *_stream);
static herr_t H5FD_stream_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t H5FD_stream_get_eoa (H5FD_t *_stream);
static herr_t  H5FD_stream_set_eoa (H5FD_t *_stream, haddr_t addr);
static haddr_t H5FD_stream_get_eof (H5FD_t *_stream);
static herr_t  H5FD_stream_read (H5FD_t *_stream, H5FD_mem_t type,
                                 hid_t fapl_id, haddr_t addr,
                                 hsize_t size, void *buf);
static herr_t  H5FD_stream_write (H5FD_t *_stream, H5FD_mem_t type,
                                  hid_t fapl_id, haddr_t addr,
                                  hsize_t size, const void *buf);

/* The Stream VFD's class information structure */
static const H5FD_class_t H5FD_stream_g =
{
  "stream",                         /* name                                */
  MAXADDR,                          /* maxaddr                             */
  NULL,                             /* sb_size                             */
  NULL,                             /* sb_encode                           */
  NULL,                             /* sb_decode                           */
  sizeof (H5FD_stream_fapl_t),      /* fapl_size                           */
  H5FD_stream_fapl_get,             /* fapl_get                            */
  NULL,                             /* fapl_copy                           */
  NULL,                             /* fapl_free                           */
  0,                                /* dxpl_size                           */
  NULL,                             /* dxpl_copy                           */
  NULL,                             /* dxpl_free                           */
  H5FD_stream_open,                 /* open                                */
  H5FD_stream_close,                /* close                               */
  NULL,                             /* cmp                                 */
  H5FD_stream_query,                /* query                               */
  NULL,                             /* alloc                               */
  NULL,                             /* free                                */
  H5FD_stream_get_eoa,              /* get_eoa                             */
  H5FD_stream_set_eoa,              /* set_eoa                             */
  H5FD_stream_get_eof,              /* get_eof                             */
  H5FD_stream_read,                 /* read                                */
  H5FD_stream_write,                /* write                               */
  H5FD_stream_flush,                /* flush                               */
  H5FD_FLMAP_SINGLE,                /* fl_map                              */
};

/* Interface initialization */
#define INTERFACE_INIT        H5FD_stream_init
static int interface_initialize_g = 0;


/*-------------------------------------------------------------------------
 * Function:      H5FD_stream_init
 *
 * Purpose:       Initialize this driver by registering it with the library.
 *
 * Return:        Success:        The driver ID for the Stream driver.
 *                Failure:        Negative.
 *
 * Programmer:    Thomas Radke
 *                Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t H5FD_stream_init (void)
{
  FUNC_ENTER (H5FD_stream_init, FAIL);

  if (H5I_VFL != H5Iget_type (H5FD_STREAM_g))
  {
    H5FD_STREAM_g = H5FDregister (&H5FD_stream_g);

    /* set the process signal mask to ignore SIGPIPE signals */
    /* NOTE: Windows doesn't know SIGPIPE signals that's why the #ifdef */
#ifdef SIGPIPE
    if (signal (SIGPIPE, SIG_IGN) == SIG_ERR)
    {
      fprintf (stderr, "Stream VFD warning: failed to set the process signal "
                       "mask to ignore SIGPIPE signals\n");
    }
#endif
  }

  FUNC_LEAVE (H5FD_STREAM_g);
}


/*-------------------------------------------------------------------------
 * Function:      H5Pset_fapl_stream
 *
 * Purpose:       Modify the file access property list to use the Stream
 *                driver defined in this source file.  The INCREMENT specifies
 *                how much to grow the memory each time we need more.
 *                If a valid socket argument is given this will be used
 *                by the driver instead of parsing the 'hostname:port' filename
 *                and opening a socket internally.
 *                
 * Return:        Success:        Non-negative
 *                Failure:        Negative
 *
 * Programmer:    Thomas Radke
 *                Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t H5Pset_fapl_stream (hid_t fapl_id, H5FD_stream_fapl_t *fapl)
{
  herr_t             result;
  H5FD_stream_fapl_t user_fapl;


  FUNC_ENTER (H5Pset_fapl_stream, FAIL);
  H5TRACE2 ("e", "ix", fapl_id, fapl);

  if (H5P_FILE_ACCESS != H5Pget_class (fapl_id))
  {
    HRETURN_ERROR (H5E_PLIST, H5E_BADTYPE, FAIL, "not a fapl");
  }

  if (fapl)
  {
    if (! fapl->do_socket_io && fapl->broadcast_fn == NULL)
    {
      HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                     "read broadcast function pointer is NULL");
    }

    user_fapl = *fapl;
    if (fapl->increment == 0)
    {
      user_fapl.increment = H5FD_STREAM_INCREMENT;
    }
    user_fapl.port = 0;
    result = H5Pset_driver (fapl_id, H5FD_STREAM, &user_fapl);
  }
  else
  {
    result = H5Pset_driver (fapl_id, H5FD_STREAM, &default_fapl);
  }

  FUNC_LEAVE (result);
}


/*-------------------------------------------------------------------------
 * Function:      H5Pget_fapl_stream
 *
 * Purpose:       Queries properties set by the H5Pset_fapl_stream() function.
 *
 * Return:        Success:        Non-negative
 *                Failure:        Negative
 *
 * Programmer:    Thomas Radke
 *                Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t H5Pget_fapl_stream(hid_t fapl_id, H5FD_stream_fapl_t *fapl /* out */)
{
  H5FD_stream_fapl_t *this_fapl;


  FUNC_ENTER (H5Pget_fapl_stream, FAIL);
  H5TRACE2("e","ix",fapl_id,fapl);

  if (H5P_FILE_ACCESS != H5Pget_class (fapl_id))
  {
    HRETURN_ERROR (H5E_PLIST, H5E_BADTYPE, FAIL, "not a fapl");
  }
  if (H5FD_STREAM != H5P_get_driver (fapl_id))
  {
    HRETURN_ERROR (H5E_PLIST, H5E_BADVALUE, FAIL, "incorrect VFL driver");
  }
  if (NULL == (this_fapl = H5Pget_driver_info (fapl_id)))
  {
    HRETURN_ERROR (H5E_PLIST, H5E_BADVALUE, FAIL, "bad VFL driver info");
  }

  if (fapl)
  {
    *fapl = *this_fapl;
  }

  FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:      H5FD_stream_fapl_get
 *
 * Purpose:       Returns a copy of the file access properties
 *
 * Return:        Success:        Ptr to new file access properties
 *                Failure:        NULL
 *
 * Programmer:    Thomas Radke
 *                Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *H5FD_stream_fapl_get (H5FD_t *_stream)
{
  H5FD_stream_t      *stream = (H5FD_stream_t *) _stream;
  H5FD_stream_fapl_t *fapl;


  FUNC_ENTER (H5FD_stream_fapl_get, NULL);

  if ((fapl = H5MM_calloc (sizeof (H5FD_stream_fapl_t))) == NULL)
  {
    HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
  }

  *fapl = stream->fapl;

  FUNC_LEAVE (fapl);
}


static H5FD_STREAM_SOCKET_TYPE
H5FDstream_open_socket (const char *filename, int o_flags,
                        H5FD_stream_fapl_t *fapl,
                        const char **errormsg,
                        H5E_major_t *major, H5E_minor_t *minor)
{
  struct sockaddr_in server;
  struct hostent *he;
  H5FD_STREAM_SOCKET_TYPE sock;
  char *hostname;
  unsigned short int first_port;
  const char *separator, *tmp; 
  int on = 1;


  sock = H5FD_STREAM_INVALID_SOCKET;
  *errormsg = NULL;

  /* Parse "hostname:port" from filename argument */
  for (separator = filename; *separator != ':' && *separator; separator++);
  if (separator == filename || !*separator)
  {
    *errormsg = "invalid host address";
  }
  else
  {
    tmp = separator;
    if (! tmp[1])
    {
      *errormsg = "no port number";
    }
    while (*errormsg == NULL && *++tmp)
    {
      if (! isdigit (*tmp))
      {
        *errormsg = "invalid port number";
      }
    }
  }
  
  /* Return if parsing the filename failed */
  if (*errormsg)
  {
    *major = H5E_ARGS; *minor = H5E_BADVALUE;
    return (sock);
  }

  hostname = (char *) H5MM_malloc ((size_t)(separator - filename + 1));

  /* Return if out of memory */
  if (hostname == NULL)
  {
    *major = H5E_RESOURCE; *minor = H5E_NOSPACE;
    *errormsg = "memory allocation failed";
    return (sock);
  }

  HDstrncpy (hostname, filename, (size_t)(separator - filename));
  hostname[separator - filename] = 0;
  fapl->port = atoi (separator + 1);

  HDmemset (&server, 0, sizeof (server));
  server.sin_family = AF_INET;
  server.sin_port = htons (fapl->port);

  if (! (he = gethostbyname (hostname)))
  {
    *errormsg = "unable to get host address";
  }
  else if (H5FD_STREAM_ERROR_CHECK (sock = socket (AF_INET, SOCK_STREAM, 0)))
  {
    *errormsg = "unable to open socket";
  }

  if (*errormsg == NULL)
  {
    if (O_RDONLY == o_flags)
    {
      HDmemcpy (&server.sin_addr, he->h_addr, (size_t)he->h_length);
#ifdef DEBUG
      fprintf (stderr, "Stream VFD: connecting to host '%s' port %d\n",
               hostname, fapl->port);
#endif
      if (connect (sock, (struct sockaddr *) &server, sizeof (server)) < 0)
      {
        *errormsg = "unable to connect";
      }
    }
    else
    {
      server.sin_addr.s_addr = INADDR_ANY;
      if (H5FD_STREAM_IOCTL_SOCKET (sock, FIONBIO, &on) < 0)
      {
        *errormsg = "unable to set non-blocking mode for socket";
      }
      else if (setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, (const char *) &on,
                           sizeof(on)) < 0)
      {
        *errormsg = "unable to set socket option TCP_NODELAY";
      }
      else if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &on,
                           sizeof(on)) < 0)
      {
        *errormsg = "unable to set socket option SO_REUSEADDR";
      }
      else
      {
        /* Try to bind the socket to the given port.
           If maxhunt is given try some successive ports also. */
        first_port = fapl->port;
        while (fapl->port <= first_port + fapl->maxhunt)
        {
#ifdef DEBUG
          fprintf (stderr, "Stream VFD: binding to port %d\n", fapl->port);
#endif
          server.sin_port = htons (fapl->port);
          if (bind (sock, (struct sockaddr *) &server, sizeof (server)) < 0)
          {
            fapl->port++;
          }
          else
          {
            break;
          }
        }
        if (fapl->port > first_port + fapl->maxhunt)
        {
          fapl->port = 0;
          *errormsg = "unable to bind socket";
        }
        else if (listen (sock, fapl->backlog) < 0)
        {
          *errormsg = "unable to listen on socket";
        }
      }
    }
  }

  H5MM_xfree (hostname);

  /* Return if opening the socket failed */
  if (*errormsg)
  {
    if (H5FD_STREAM_ERROR_CHECK (sock))
    {
      H5FD_STREAM_CLOSE_SOCKET (sock);
      sock = H5FD_STREAM_INVALID_SOCKET;
    }
    *major = H5E_FILE; *minor = H5E_CANTOPENFILE;
  }

  return (sock);
}


static void H5FDstream_read_from_socket (H5FD_stream_t *stream,
                                         const char **errormsg,
                                         H5E_major_t *major, H5E_minor_t *minor)
{
  int size;
  size_t max_size = 0;
  unsigned char *ptr;


  ptr = NULL;
  *errormsg = NULL;
  stream->eof = 0;
  stream->mem = NULL;

  while (1)
  {
    if (max_size <= 0)
    {
      /*
       * Allocate initial buffer as increment + 1
       * to prevent unnecessary reallocation
       * if increment is exactly a multiple of the filesize
       */
      max_size = stream->fapl.increment;
      if (! stream->mem)
      {
        max_size++;
      }
      ptr = H5MM_realloc (stream->mem, (size_t) (stream->eof + max_size));
      if (! ptr)
      {
        *major = H5E_RESOURCE; *minor = H5E_NOSPACE;
        *errormsg = "unable to allocate file space buffer";
        return;
      }
      stream->mem = ptr;
      ptr += stream->eof;
    }

    /* now receive the next chunk of data */
    size = recv (stream->socket, ptr, max_size, 0);

    if (size < 0 && (EINTR == errno || EAGAIN == errno || EWOULDBLOCK))
    {
      continue;
    }
    if (size < 0)
    {
      *major = H5E_IO; *minor = H5E_READERROR;
      *errormsg = "error reading from file from socket";
      return;
    }
    if (! size)
    {
      break;
    }
    max_size -= (size_t) size;
    stream->eof += (haddr_t) size;
    ptr += size;
#ifdef DEBUG
    fprintf (stderr, "Stream VFD: read %d bytes (%d total) from socket\n",
             size, (int) stream->eof);
#endif
  }

#ifdef DEBUG
  fprintf (stderr, "Stream VFD: read total of %d bytes from socket\n",
           (int) stream->eof);
#endif
}


/*-------------------------------------------------------------------------
 * Function:      H5FD_stream_open
 *
 * Purpose:       Opens an HDF5 file in memory.
 *
 * Return:        Success:        A pointer to a new file data structure. The
 *                                public fields will be initialized by the
 *                                caller, which is always H5FD_open().
 *                Failure:        NULL
 *
 * Programmer:    Thomas Radke
 *                Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5FD_t *H5FD_stream_open (const char *filename,
                                 unsigned flags,
                                 hid_t fapl_id,
                                 haddr_t maxaddr)
{
  H5FD_stream_t             _stream, *stream;
  const H5FD_stream_fapl_t *fapl;
  int                       o_flags;
  H5E_major_t               major;
  H5E_minor_t               minor;
  const char               *errormsg;
#ifdef WIN32
  WSADATA wsadata;
#endif


  FUNC_ENTER (H5FD_stream_open, NULL);

  /* Check arguments */
  if (filename == NULL|| *filename == '\0')
  {
    HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, NULL,"invalid file name");
  }
  if (maxaddr == 0 || HADDR_UNDEF == maxaddr)
  {
    HRETURN_ERROR (H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr");
  }
  if (ADDR_OVERFLOW (maxaddr))
  {
    HRETURN_ERROR (H5E_ARGS, H5E_OVERFLOW, NULL, "maxaddr overflow");
  }

  /* Build the open flags */
  o_flags = (H5F_ACC_RDWR & flags) ? O_RDWR : O_RDONLY;
  if (H5F_ACC_TRUNC & flags) o_flags |= O_TRUNC;
  if (H5F_ACC_CREAT & flags) o_flags |= O_CREAT;
  if (H5F_ACC_EXCL & flags)  o_flags |= O_EXCL;

  if ((O_RDWR & o_flags) && ! (O_CREAT & o_flags))
  {
    HRETURN_ERROR (H5E_ARGS, H5E_UNSUPPORTED, NULL,
                   "open stream for read/write not supported");
  }

#ifdef WIN32
  if (WSAStartup (MAKEWORD (2, 0), &wsadata))
  {
    HRETURN_ERROR (H5E_IO, H5E_CANTINIT, NULL,
                   "Couldn't start Win32 socket layer");
  }
#endif

  fapl = NULL;
  if (H5P_DEFAULT != fapl_id)
  {
    fapl = H5Pget_driver_info (fapl_id);
  }
  if (fapl == NULL)
  {
    fapl = &default_fapl;
  }

  /* zero out file structure and set file access property list */
  HDmemset (&_stream, 0, sizeof (_stream));
  _stream.fapl = *fapl;

  errormsg = NULL;

  /* if an external socket is provided with the file access property list
     we use that, otherwise the filename argument is parsed and a socket
     is opened internally */
  if (fapl->do_socket_io)
  {
    if (! H5FD_STREAM_ERROR_CHECK (fapl->socket))
    {
      _stream.internal_socket = FALSE;
      _stream.socket = fapl->socket;
    }
    else
    {
      _stream.internal_socket = TRUE;
      _stream.socket = H5FDstream_open_socket (filename, o_flags, &_stream.fapl,
                                               &errormsg, &major, &minor);
      if (_stream.socket != H5FD_STREAM_INVALID_SOCKET)
      {
        /* update the port ID in the file access property
           so that it can be queried via H5P_get_fapl_stream() later on */
        H5Pset_driver (fapl_id, H5FD_STREAM, &_stream.fapl);
      }
    }
  }
  else
  {
    _stream.socket = H5FD_STREAM_INVALID_SOCKET;
  }

  /* read the data from socket into memory */
  if (O_RDONLY == o_flags)
  {
    if (errormsg == NULL && fapl->do_socket_io)
    {
#ifdef DEBUG
      fprintf (stderr, "Stream VFD: reading file from socket\n");
#endif
      H5FDstream_read_from_socket (&_stream, &errormsg, &major, &minor);
    }

    /* Now call the user's broadcast routine if given */
    if (fapl->broadcast_fn)
    {
      if ((fapl->broadcast_fn) (&_stream.mem, &_stream.eof,
                                fapl->broadcast_arg) < 0)
      {
        /* don't override previous error codes */
        if (errormsg == NULL)
        {
          major = H5E_IO; minor = H5E_READERROR;
          errormsg = "broadcast error";
        }
      }

      /* check for filesize of zero bytes */
      if (errormsg == NULL && _stream.eof == 0)
      {
        major = H5E_IO; minor = H5E_READERROR;
        errormsg = "zero filesize";
      }
    }

    /* For files which are read from a socket:
       the opened socket is not needed anymore */
    if (errormsg == NULL)
    {
      if (_stream.internal_socket && H5FD_STREAM_ERROR_CHECK (_stream.socket))
      {
        H5FD_STREAM_CLOSE_SOCKET (_stream.socket);
      }
      _stream.socket = H5FD_STREAM_INVALID_SOCKET;
    }
  }

  /* Create the new file struct */
  stream = NULL;
  if (errormsg == NULL)
  {
    stream = (H5FD_stream_t *) H5MM_calloc (sizeof (H5FD_stream_t));
    if (stream == NULL)
    {
      major = H5E_RESOURCE; minor = H5E_NOSPACE;
      errormsg = "unable to allocate file struct";
    }
    else
    {
      *stream = _stream;
    }
  }

  if (errormsg)
  {
    if (_stream.mem)
    {
      H5MM_xfree (_stream.mem);
    }
    if (_stream.internal_socket && H5FD_STREAM_ERROR_CHECK (_stream.socket))
    {
      H5FD_STREAM_CLOSE_SOCKET (_stream.socket);
    }
    HRETURN_ERROR (major, minor, NULL, errormsg);
  }

  FUNC_LEAVE ((H5FD_t *) stream);
}


/*-------------------------------------------------------------------------
 * Function:      H5FD_stream_flush
 *
 * Purpose:       Flushes the file via sockets to any connected clients
 *                if its dirty flag is set.
 *
 * Return:        Success:        0
 *                Failure:        -1
 *
 * Programmer:    Thomas Radke
 *                Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5FD_stream_flush (H5FD_t *_stream)
{
  H5FD_stream_t *stream = (H5FD_stream_t *) _stream;
  size_t size;
  ssize_t bytes_send;
  int on = 1;
  unsigned char *ptr;
  struct sockaddr from;
  socklen_t fromlen;
  H5FD_STREAM_SOCKET_TYPE sock;


  FUNC_ENTER (H5FD_stream_flush, FAIL);

  /* Write to backing store */
  if (stream->dirty && ! H5FD_STREAM_ERROR_CHECK (stream->socket))
  {
#ifdef DEBUG
    fprintf (stderr, "Stream VFD: accepting client connections\n");
#endif
    fromlen = sizeof (from);
    while (! H5FD_STREAM_ERROR_CHECK (sock = accept (stream->socket,
                                                     &from, &fromlen)))
    {
      if (H5FD_STREAM_IOCTL_SOCKET (sock, FIONBIO, &on) < 0)
      {
        H5FD_STREAM_CLOSE_SOCKET (sock);
        continue;           /* continue the loop for other clients to connect */
      }

      size = stream->eof;
      ptr = stream->mem;

      while (size)
      {
        bytes_send = send (sock, ptr, size, 0);
        if (bytes_send < 0)
        {
          if (EINTR == errno || EAGAIN == errno || EWOULDBLOCK == errno)
          {
            continue;
          }

          /* continue the outermost loop for other clients to connect */
          break;
        }
        ptr += bytes_send;
        size -= bytes_send;
#ifdef DEBUG
        fprintf (stderr, "Stream VFD: wrote %d bytes to socket, %d in total, "
                 "%d left\n", bytes_send, (int) (ptr - stream->mem), size);
#endif
      }
      H5FD_STREAM_CLOSE_SOCKET (sock);
    }
    stream->dirty = FALSE;
  }

  FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:      H5FD_stream_close
 *
 * Purpose:       Closes the file.
 *
 * Return:        Success:        0
 *                Failure:        -1
 *
 * Programmer:    Thomas Radke
 *                Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5FD_stream_close (H5FD_t *_stream)
{
  H5FD_stream_t *stream = (H5FD_stream_t *) _stream;


  FUNC_ENTER (H5FD_stream_close, FAIL);

  /* Flush */
  if (H5FD_stream_flush (_stream) != SUCCEED)
  {
    HRETURN_ERROR (H5E_FILE, H5E_CANTFLUSH, FAIL, "unable to flush file");
  }

  /* Release resources */
  if (H5FD_STREAM_ERROR_CHECK (stream->socket) && stream->internal_socket)
  {
    H5FD_STREAM_CLOSE_SOCKET (stream->socket);
  }
  if (stream->mem)
  {
    H5MM_xfree (stream->mem);
  }
  HDmemset (stream, 0, sizeof (H5FD_stream_t));
  H5MM_xfree (stream);

  FUNC_LEAVE (0);
}


/*-------------------------------------------------------------------------
 * Function:      H5FD_stream_query
 *
 * Purpose:       Set the flags that this VFL driver is capable of supporting.
 *                 (listed in H5FDpublic.h)
 *
 * Return:        Success:        non-negative
 *
 *                Failure:        negative
 *
 * Programmer:    Quincey Koziol
 *                Tuesday, September 26, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_stream_query(const H5FD_t UNUSED *_f,
                  unsigned long *flags/*out*/)
{
    FUNC_ENTER (H5FD_stream_query, SUCCEED);

    /* Set the VFL feature flags that this driver supports */
    if (flags) {
        *flags = 0;
        /* OK to perform data sieving for faster raw data reads & writes */
        *flags |= H5FD_FEAT_DATA_SIEVE;
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:      H5FD_stream_get_eoa
 *
 * Purpose:       Gets the end-of-address marker for the file. The EOA marker
 *                is the first address past the last byte allocated in the
 *                format address space.
 *
 * Return:        Success:        The end-of-address marker.
 *                Failure:        HADDR_UNDEF
 *
 * Programmer:    Thomas Radke
 *                Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t H5FD_stream_get_eoa (H5FD_t *_stream)
{
  H5FD_stream_t *stream = (H5FD_stream_t *) _stream;


  FUNC_ENTER (H5FD_stream_get_eoa, HADDR_UNDEF);

  FUNC_LEAVE (stream->eoa);
}


/*-------------------------------------------------------------------------
 * Function:      H5FD_stream_set_eoa
 *
 * Purpose:       Set the end-of-address marker for the file. This function is
 *                called shortly after an existing HDF5 file is opened in order
 *                to tell the driver where the end of the HDF5 data is located.
 *
 * Return:        Success:        0
 *                Failure:        -1
 *
 * Programmer:    Thomas Radke
 *                Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5FD_stream_set_eoa (H5FD_t *_stream,
                                   haddr_t addr)
{
  H5FD_stream_t        *stream = (H5FD_stream_t *) _stream;


  FUNC_ENTER (H5FD_stream_set_eoa, FAIL);

  if (ADDR_OVERFLOW (addr))
  {
    HRETURN_ERROR (H5E_ARGS, H5E_OVERFLOW, FAIL, "address overflow");
  }

  stream->eoa = addr;

  FUNC_LEAVE (0);
}


/*-------------------------------------------------------------------------
 * Function:      H5FD_stream_get_eof
 *
 * Purpose:       Returns the end-of-file marker, which is the greater of
 *                either the size of the underlying memory or the HDF5
 *                end-of-address markers.
 *
 * Return:        Success:        End of file address, the first address past
 *                                the end of the "file", either the memory
 *                                or the HDF5 file.
 *                Failure:        HADDR_UNDEF
 *
 * Programmer:    Thomas Radke
 *                Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t H5FD_stream_get_eof (H5FD_t *_stream)
{
  H5FD_stream_t        *stream = (H5FD_stream_t *) _stream;

  FUNC_ENTER (H5FD_stream_get_eof, HADDR_UNDEF);

  FUNC_LEAVE (MAX (stream->eof, stream->eoa));
}


/*-------------------------------------------------------------------------
 * Function:      H5FD_stream_read
 *
 * Purpose:       Reads SIZE bytes of data from FILE beginning at address ADDR
 *                into buffer BUF according to data transfer properties in
 *                DXPL_ID.
 *
 * Return:        Success:        0
 *                                Result is stored in caller-supplied buffer BUF
 *                Failure:        -1
 *                                Contents of buffer BUF are undefined
 *
 * Programmer:    Thomas Radke
 *                Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5FD_stream_read (H5FD_t *_stream,
                                H5FD_mem_t UNUSED type,
                                hid_t UNUSED dxpl_id,
                                haddr_t addr,
                                hsize_t size,
                                void *buf /*out*/)
{
  H5FD_stream_t *stream = (H5FD_stream_t *) _stream;
  ssize_t        nbytes;

  FUNC_ENTER (H5FD_stream_read, FAIL);

  assert (stream && stream->pub.cls);
  assert (buf);

  /* Check for overflow conditions */
  if (HADDR_UNDEF == addr)
  {
    HRETURN_ERROR (H5E_IO, H5E_OVERFLOW, FAIL, "file address overflowed");
  }
  if (REGION_OVERFLOW (addr, size))
  {
    HRETURN_ERROR (H5E_IO, H5E_OVERFLOW, FAIL, "file address overflowed");
  }
  if (addr + size > stream->eoa)
  {
    HRETURN_ERROR (H5E_IO, H5E_OVERFLOW, FAIL, "file address overflowed");
  }

  /* Read the part which is before the EOF marker */
  if (addr < stream->eof)
  {
    nbytes = (ssize_t) MIN (size, stream->eof - addr);
    HDmemcpy (buf, stream->mem + addr, (size_t) nbytes);
    size -= nbytes;
    addr += nbytes;
    buf = (char *) buf + nbytes;
  }

  /* Read zeros for the part which is after the EOF markers */
  if (size > 0)
  {
    HDmemset (buf, 0, (size_t) size);
  }

  FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:      H5FD_stream_write
 *
 * Purpose:       Writes SIZE bytes of data to FILE beginning at address ADDR
 *                from buffer BUF according to data transfer properties in
 *                DXPL_ID.
 *
 * Return:        Success:        Zero
 *                Failure:        -1
 *
 * Programmer:    Thomas Radke
 *                Tuesday, September 12, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5FD_stream_write (H5FD_t *_stream,
                                 H5FD_mem_t UNUSED type,
                                 hid_t UNUSED dxpl_id,
                                 haddr_t addr,
                                 hsize_t size,
                                 const void *buf)
{
  H5FD_stream_t                *stream = (H5FD_stream_t *) _stream;


  FUNC_ENTER (H5FD_stream_write, FAIL);

  assert (stream && stream->pub.cls);
  assert (buf);

  /* Check for overflow conditions */
  if (REGION_OVERFLOW (addr, size))
  {
    HRETURN_ERROR (H5E_IO, H5E_OVERFLOW, FAIL, "file address overflowed");
  }
  if (addr + size > stream->eoa)
  {
    HRETURN_ERROR (H5E_IO, H5E_OVERFLOW, FAIL, "file address overflowed");
  }

  /*
   * Allocate more memory if necessary, careful of overflow. Also, if the
   * allocation fails then the file should remain in a usable state.  Be
   * careful of non-Posix realloc() that doesn't understand what to do when
   * the first argument is null.
   */
  if (addr + size > stream->eof)
  {
    unsigned char *x;
    haddr_t new_eof = stream->fapl.increment *
                      ((addr+size) / stream->fapl.increment);


    if ((addr+size) % stream->fapl.increment)
    {
      new_eof += stream->fapl.increment;
    }
    if (stream->mem == NULL)
    {
      x = H5MM_malloc ((size_t) new_eof);
    }
    else
    {
      x = H5MM_realloc (stream->mem, (size_t) new_eof);
    }
    if (x == NULL)
    {
      HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                     "unable to allocate memory block");
    }
    stream->mem = x;
    stream->eof = new_eof;
  }

  /* Write from BUF to memory */
  HDmemcpy (stream->mem + addr, buf, (size_t) size);
  stream->dirty = TRUE;

  FUNC_LEAVE (SUCCEED);
}

#endif /* H5_HAVE_STREAM */
