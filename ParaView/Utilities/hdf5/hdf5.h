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
 * This is the main public HDF5 include file.  Put further information in
 * a particular header file and include that here, don't fill this file with
 * lots of gunk...
 */
#ifndef _HDF5_H
#define _HDF5_H

#include "H5public.h"
#include "H5Ipublic.h"          /* Interface abstraction                */
#include "H5Apublic.h"          /* Attributes                           */
#include "H5ACpublic.h"         /* Metadata cache                       */
#include "H5Bpublic.h"          /* B-trees                              */
#include "H5Dpublic.h"          /* Datasets                             */
#include "H5Epublic.h"          /* Errors                               */
#include "H5Fpublic.h"          /* Files                                */
#include "H5FDpublic.h"         /* File drivers                         */
#include "H5Gpublic.h"          /* Groups                               */
#include "H5HGpublic.h"         /* Global heaps                         */
#include "H5HLpublic.h"         /* Local heaps                          */
#include "H5MMpublic.h"         /* Memory management                    */
#include "H5Opublic.h"          /* Object headers                       */
#include "H5Ppublic.h"          /* Property lists                       */
#include "H5Rpublic.h"          /* References                           */
#include "H5Spublic.h"          /* Dataspaces                           */
#include "H5Tpublic.h"          /* Datatypes                            */
#include "H5Zpublic.h"          /* Data filters                         */

/* Predefined file drivers */
#include "H5FDcore.h"           /* Files stored entirely in memory      */
#include "H5FDfamily.h"         /* File families                        */
#include "H5FDmpio.h"           /* Parallel files using MPI-2 I/O       */
#include "H5FDsec2.h"           /* POSIX unbuffered file I/O            */
#include "H5FDstdio.h"          /* Standard C buffered I/O              */
#include "H5FDsrb.h"            /* Remote access using SRB              */
#include "H5FDgass.h"           /* Remote files using GASS I/O          */
#include "H5FDstream.h"         /* in-memory files streamed via sockets */
#include "H5FDmulti.h"          /* Usage-partitioned file family        */
#include "H5FDlog.h"            /* sec2 driver with I/O logging (for debugging) */

#endif
