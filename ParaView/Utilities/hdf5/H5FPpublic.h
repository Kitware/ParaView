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
#ifndef H5FPPUBLIC_H__
#define H5FPPUBLIC_H__ 0

#include "H5public.h"

#ifdef H5_HAVE_FPHDF5

#ifdef __cplusplus
extern "C" {
#endif

H5_DLL herr_t H5FPinit(MPI_Comm comm, int sap_rank,
                       MPI_Comm *FP_comm, MPI_Comm *FP_barrier_comm);
H5_DLL herr_t H5FPfinalize(void);

#ifdef __cplusplus
}
#endif

#endif  /* H5_HAVE_FPHDF5 */

#endif  /* H5FPPUBLIC_H__ */
