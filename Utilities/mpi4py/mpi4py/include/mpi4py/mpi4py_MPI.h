#ifndef __PYX_HAVE__mpi4py__MPI
#define __PYX_HAVE__mpi4py__MPI
#ifdef __cplusplus
#define __PYX_EXTERN_C extern "C"
#else
#define __PYX_EXTERN_C extern
#endif

/* "include/mpi4py/MPI.pxd":50
 * ctypedef MPI_Offset Offset
 * 
 * ctypedef public api class Status [type PyMPIStatus_Type, object PyMPIStatusObject]:             # <<<<<<<<<<<<<<
 *     cdef MPI_Status ob_mpi
 *     cdef int        flags
 */

typedef struct {
  PyObject_HEAD
  MPI_Status ob_mpi;
  int flags;
} PyMPIStatusObject;

/* "include/mpi4py/MPI.pxd":54
 *     cdef int        flags
 * 
 * ctypedef public api class Datatype [type PyMPIDatatype_Type, object PyMPIDatatypeObject]:             # <<<<<<<<<<<<<<
 *     cdef MPI_Datatype ob_mpi
 *     cdef int          flags
 */

typedef struct {
  PyObject_HEAD
  MPI_Datatype ob_mpi;
  int flags;
} PyMPIDatatypeObject;

/* "include/mpi4py/MPI.pxd":58
 *     cdef int          flags
 * 
 * ctypedef public api class Request [type PyMPIRequest_Type, object PyMPIRequestObject]:             # <<<<<<<<<<<<<<
 *     cdef MPI_Request ob_mpi
 *     cdef int         flags
 */

typedef struct {
  PyObject_HEAD
  MPI_Request ob_mpi;
  int flags;
} PyMPIRequestObject;

/* "include/mpi4py/MPI.pxd":62
 *     cdef int         flags
 * 
 * ctypedef public api class Prequest(Request) [type PyMPIPrequest_Type, object PyMPIPrequestObject]:             # <<<<<<<<<<<<<<
 *     pass
 * 
 */

typedef struct {
  PyMPIRequestObject __pyx_base;
} PyMPIPrequestObject;

/* "include/mpi4py/MPI.pxd":65
 *     pass
 * 
 * ctypedef public api class Grequest(Request) [type PyMPIGrequest_Type, object PyMPIGrequestObject]:             # <<<<<<<<<<<<<<
 *     cdef MPI_Request ob_grequest
 * 
 */

typedef struct {
  PyMPIRequestObject __pyx_base;
  MPI_Request ob_grequest;
} PyMPIGrequestObject;

/* "include/mpi4py/MPI.pxd":68
 *     cdef MPI_Request ob_grequest
 * 
 * ctypedef public api class Op [type PyMPIOp_Type, object PyMPIOpObject]:             # <<<<<<<<<<<<<<
 *     cdef MPI_Op ob_mpi
 *     cdef int    flags
 */

typedef struct {
  PyObject_HEAD
  MPI_Op ob_mpi;
  int flags;
  PyObject *(*ob_func)(PyObject *, PyObject *);
  PyObject *ob_callable;
  int ob_commute;
} PyMPIOpObject;

/* "include/mpi4py/MPI.pxd":75
 *     cdef bint   ob_commute
 * 
 * ctypedef public api class Group [type PyMPIGroup_Type, object PyMPIGroupObject]:             # <<<<<<<<<<<<<<
 *     cdef MPI_Group ob_mpi
 *     cdef int       flags
 */

typedef struct {
  PyObject_HEAD
  MPI_Group ob_mpi;
  int flags;
} PyMPIGroupObject;

/* "include/mpi4py/MPI.pxd":79
 *     cdef int       flags
 * 
 * ctypedef public api class Info [type PyMPIInfo_Type, object PyMPIInfoObject]:             # <<<<<<<<<<<<<<
 *     cdef MPI_Info ob_mpi
 *     cdef int      flags
 */

typedef struct {
  PyObject_HEAD
  MPI_Info ob_mpi;
  int flags;
} PyMPIInfoObject;

/* "include/mpi4py/MPI.pxd":83
 *     cdef int      flags
 * 
 * ctypedef public api class Errhandler [type PyMPIErrhandler_Type, object PyMPIErrhandlerObject]:             # <<<<<<<<<<<<<<
 *     cdef MPI_Errhandler ob_mpi
 *     cdef int            flags
 */

typedef struct {
  PyObject_HEAD
  MPI_Errhandler ob_mpi;
  int flags;
} PyMPIErrhandlerObject;

/* "include/mpi4py/MPI.pxd":87
 *     cdef int            flags
 * 
 * ctypedef public api class Comm [type PyMPIComm_Type, object PyMPICommObject]:             # <<<<<<<<<<<<<<
 *     cdef MPI_Comm ob_mpi
 *     cdef int      flags
 */

typedef struct {
  PyObject_HEAD
  MPI_Comm ob_mpi;
  int flags;
} PyMPICommObject;

/* "include/mpi4py/MPI.pxd":91
 *     cdef int      flags
 * 
 * ctypedef public api class Intracomm(Comm) [type PyMPIIntracomm_Type, object PyMPIIntracommObject]:             # <<<<<<<<<<<<<<
 *     pass
 * 
 */

typedef struct {
  PyMPICommObject __pyx_base;
} PyMPIIntracommObject;

/* "include/mpi4py/MPI.pxd":94
 *     pass
 * 
 * ctypedef public api class Cartcomm(Intracomm) [type PyMPICartcomm_Type, object PyMPICartcommObject]:             # <<<<<<<<<<<<<<
 *     pass
 * 
 */

typedef struct {
  PyMPIIntracommObject __pyx_base;
} PyMPICartcommObject;

/* "include/mpi4py/MPI.pxd":97
 *     pass
 * 
 * ctypedef public api class Graphcomm(Intracomm) [type PyMPIGraphcomm_Type, object PyMPIGraphcommObject]:             # <<<<<<<<<<<<<<
 *     pass
 * 
 */

typedef struct {
  PyMPIIntracommObject __pyx_base;
} PyMPIGraphcommObject;

/* "include/mpi4py/MPI.pxd":100
 *     pass
 * 
 * ctypedef public api class Intercomm(Comm) [type PyMPIIntercomm_Type, object PyMPIIntercommObject]:             # <<<<<<<<<<<<<<
 *     pass
 * 
 */

typedef struct {
  PyMPICommObject __pyx_base;
} PyMPIIntercommObject;

/* "include/mpi4py/MPI.pxd":103
 *     pass
 * 
 * ctypedef public api class Win [type PyMPIWin_Type, object PyMPIWinObject]:             # <<<<<<<<<<<<<<
 *     cdef MPI_Win ob_mpi
 *     cdef int     flags
 */

typedef struct {
  PyObject_HEAD
  MPI_Win ob_mpi;
  int flags;
} PyMPIWinObject;

/* "include/mpi4py/MPI.pxd":107
 *     cdef int     flags
 * 
 * ctypedef public api class File [type PyMPIFile_Type, object PyMPIFileObject]:             # <<<<<<<<<<<<<<
 *     cdef MPI_File ob_mpi
 *     cdef int      flags
 */

typedef struct {
  PyObject_HEAD
  MPI_File ob_mpi;
  int flags;
} PyMPIFileObject;

#ifndef __PYX_HAVE_API__mpi4py__MPI

__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIStatus_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIDatatype_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIRequest_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIPrequest_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIGrequest_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIOp_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIGroup_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIInfo_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIErrhandler_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIComm_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIIntracomm_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPICartcomm_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIGraphcomm_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIIntercomm_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIWin_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIFile_Type;

#endif

PyMODINIT_FUNC initMPI(void);

#endif
