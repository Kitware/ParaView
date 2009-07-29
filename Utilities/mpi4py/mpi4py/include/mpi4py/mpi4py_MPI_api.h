#ifndef __PYX_HAVE_API__mpi4py__MPI
#define __PYX_HAVE_API__mpi4py__MPI
#include "Python.h"
#include "mpi4py_MPI.h"

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Status;
#define PyMPIStatus_Type (*__pyx_ptype_6mpi4py_3MPI_Status)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Datatype;
#define PyMPIDatatype_Type (*__pyx_ptype_6mpi4py_3MPI_Datatype)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Request;
#define PyMPIRequest_Type (*__pyx_ptype_6mpi4py_3MPI_Request)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Prequest;
#define PyMPIPrequest_Type (*__pyx_ptype_6mpi4py_3MPI_Prequest)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Grequest;
#define PyMPIGrequest_Type (*__pyx_ptype_6mpi4py_3MPI_Grequest)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Op;
#define PyMPIOp_Type (*__pyx_ptype_6mpi4py_3MPI_Op)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Group;
#define PyMPIGroup_Type (*__pyx_ptype_6mpi4py_3MPI_Group)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Info;
#define PyMPIInfo_Type (*__pyx_ptype_6mpi4py_3MPI_Info)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Errhandler;
#define PyMPIErrhandler_Type (*__pyx_ptype_6mpi4py_3MPI_Errhandler)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Comm;
#define PyMPIComm_Type (*__pyx_ptype_6mpi4py_3MPI_Comm)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Intracomm;
#define PyMPIIntracomm_Type (*__pyx_ptype_6mpi4py_3MPI_Intracomm)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Cartcomm;
#define PyMPICartcomm_Type (*__pyx_ptype_6mpi4py_3MPI_Cartcomm)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Graphcomm;
#define PyMPIGraphcomm_Type (*__pyx_ptype_6mpi4py_3MPI_Graphcomm)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Intercomm;
#define PyMPIIntercomm_Type (*__pyx_ptype_6mpi4py_3MPI_Intercomm)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Win;
#define PyMPIWin_Type (*__pyx_ptype_6mpi4py_3MPI_Win)

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_File;
#define PyMPIFile_Type (*__pyx_ptype_6mpi4py_3MPI_File)

static PyObject *(*PyMPIDatatype_New)(MPI_Datatype);
static MPI_Datatype *(*PyMPIDatatype_Get)(PyObject *);
static PyObject *(*PyMPIStatus_New)(MPI_Status *);
static MPI_Status *(*PyMPIStatus_Get)(PyObject *);
static PyObject *(*PyMPIRequest_New)(MPI_Request);
static MPI_Request *(*PyMPIRequest_Get)(PyObject *);
static PyObject *(*PyMPIOp_New)(MPI_Op);
static MPI_Op *(*PyMPIOp_Get)(PyObject *);
static PyObject *(*PyMPIInfo_New)(MPI_Info);
static MPI_Info *(*PyMPIInfo_Get)(PyObject *);
static PyObject *(*PyMPIGroup_New)(MPI_Group);
static MPI_Group *(*PyMPIGroup_Get)(PyObject *);
static PyObject *(*PyMPIComm_New)(MPI_Comm);
static MPI_Comm *(*PyMPIComm_Get)(PyObject *);
static PyObject *(*PyMPIWin_New)(MPI_Win);
static MPI_Win *(*PyMPIWin_Get)(PyObject *);
static PyObject *(*PyMPIFile_New)(MPI_File);
static MPI_File *(*PyMPIFile_Get)(PyObject *);
static PyObject *(*PyMPIErrhandler_New)(MPI_Errhandler);
static MPI_Errhandler *(*PyMPIErrhandler_Get)(PyObject *);

#ifndef __PYX_HAVE_API_FUNC_import_module
#define __PYX_HAVE_API_FUNC_import_module

#ifndef __PYX_HAVE_RT_ImportModule
#define __PYX_HAVE_RT_ImportModule
static PyObject *__Pyx_ImportModule(const char *name) {
    PyObject *py_name = 0;
    PyObject *py_module = 0;

    #if PY_MAJOR_VERSION < 3
    py_name = PyString_FromString(name);
    #else
    py_name = PyUnicode_FromString(name);
    #endif
    if (!py_name)
        goto bad;
    py_module = PyImport_Import(py_name);
    Py_DECREF(py_name);
    return py_module;
bad:
    Py_XDECREF(py_name);
    return 0;
}
#endif

#endif


#ifndef __PYX_HAVE_RT_ImportFunction
#define __PYX_HAVE_RT_ImportFunction
static int __Pyx_ImportFunction(PyObject *module, const char *funcname, void (**f)(void), const char *sig) {
#if PY_VERSION_HEX < 0x02050000
    char *api = (char *)"__pyx_capi__";
#else
    const char *api = "__pyx_capi__";
#endif
    PyObject *d = 0;
    PyObject *cobj = 0;
    const char *desc;
    const char *s1, *s2;
    union {
        void (*fp)(void);
        void *p;
    } tmp;

    d = PyObject_GetAttrString(module, api);
    if (!d)
        goto bad;
    cobj = PyDict_GetItemString(d, funcname);
    if (!cobj) {
        PyErr_Format(PyExc_ImportError,
            "%s does not export expected C function %s",
                PyModule_GetName(module), funcname);
        goto bad;
    }
    desc = (const char *)PyCObject_GetDesc(cobj);
    if (!desc)
        goto bad;
    s1 = desc; s2 = sig;
    while (*s1 != '\0' && *s1 == *s2) { s1++; s2++; }
    if (*s1 != *s2) {
        PyErr_Format(PyExc_TypeError,
            "C function %s.%s has wrong signature (expected %s, got %s)",
             PyModule_GetName(module), funcname, sig, desc);
        goto bad;
    }
    tmp.p = PyCObject_AsVoidPtr(cobj);
    *f = tmp.fp;
    Py_DECREF(d);
    return 0;
bad:
    Py_XDECREF(d);
    return -1;
}
#endif


#ifndef __PYX_HAVE_RT_ImportType
#define __PYX_HAVE_RT_ImportType
static PyTypeObject *__Pyx_ImportType(const char *module_name, const char *class_name,
    long size)
{
    PyObject *py_module = 0;
    PyObject *result = 0;
    PyObject *py_name = 0;

    py_module = __Pyx_ImportModule(module_name);
    if (!py_module)
        goto bad;
    #if PY_MAJOR_VERSION < 3
    py_name = PyString_FromString(class_name);
    #else
    py_name = PyUnicode_FromString(class_name);
    #endif
    if (!py_name)
        goto bad;
    result = PyObject_GetAttr(py_module, py_name);
    Py_DECREF(py_name);
    py_name = 0;
    Py_DECREF(py_module);
    py_module = 0;
    if (!result)
        goto bad;
    if (!PyType_Check(result)) {
        PyErr_Format(PyExc_TypeError, 
            "%s.%s is not a type object",
            module_name, class_name);
        goto bad;
    }
    if (((PyTypeObject *)result)->tp_basicsize != size) {
        PyErr_Format(PyExc_ValueError, 
            "%s.%s does not appear to be the correct type object",
            module_name, class_name);
        goto bad;
    }
    return (PyTypeObject *)result;
bad:
    Py_XDECREF(py_module);
    Py_XDECREF(result);
    return 0;
}
#endif

static int import_mpi4py__MPI(void) {
  PyObject *module = 0;
  module = __Pyx_ImportModule("mpi4py.MPI");
  if (!module) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIDatatype_New", (void (**)(void))&PyMPIDatatype_New, "PyObject *(MPI_Datatype)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIDatatype_Get", (void (**)(void))&PyMPIDatatype_Get, "MPI_Datatype *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIStatus_New", (void (**)(void))&PyMPIStatus_New, "PyObject *(MPI_Status *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIStatus_Get", (void (**)(void))&PyMPIStatus_Get, "MPI_Status *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIRequest_New", (void (**)(void))&PyMPIRequest_New, "PyObject *(MPI_Request)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIRequest_Get", (void (**)(void))&PyMPIRequest_Get, "MPI_Request *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIOp_New", (void (**)(void))&PyMPIOp_New, "PyObject *(MPI_Op)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIOp_Get", (void (**)(void))&PyMPIOp_Get, "MPI_Op *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIInfo_New", (void (**)(void))&PyMPIInfo_New, "PyObject *(MPI_Info)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIInfo_Get", (void (**)(void))&PyMPIInfo_Get, "MPI_Info *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIGroup_New", (void (**)(void))&PyMPIGroup_New, "PyObject *(MPI_Group)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIGroup_Get", (void (**)(void))&PyMPIGroup_Get, "MPI_Group *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIComm_New", (void (**)(void))&PyMPIComm_New, "PyObject *(MPI_Comm)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIComm_Get", (void (**)(void))&PyMPIComm_Get, "MPI_Comm *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIWin_New", (void (**)(void))&PyMPIWin_New, "PyObject *(MPI_Win)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIWin_Get", (void (**)(void))&PyMPIWin_Get, "MPI_Win *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIFile_New", (void (**)(void))&PyMPIFile_New, "PyObject *(MPI_File)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIFile_Get", (void (**)(void))&PyMPIFile_Get, "MPI_File *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIErrhandler_New", (void (**)(void))&PyMPIErrhandler_New, "PyObject *(MPI_Errhandler)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIErrhandler_Get", (void (**)(void))&PyMPIErrhandler_Get, "MPI_Errhandler *(PyObject *)") < 0) goto bad;
  Py_DECREF(module); module = 0;
  __pyx_ptype_6mpi4py_3MPI_Status = __Pyx_ImportType("mpi4py.MPI", "Status", sizeof(PyMPIStatusObject)); if (!__pyx_ptype_6mpi4py_3MPI_Status) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Datatype = __Pyx_ImportType("mpi4py.MPI", "Datatype", sizeof(PyMPIDatatypeObject)); if (!__pyx_ptype_6mpi4py_3MPI_Datatype) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Request = __Pyx_ImportType("mpi4py.MPI", "Request", sizeof(PyMPIRequestObject)); if (!__pyx_ptype_6mpi4py_3MPI_Request) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Prequest = __Pyx_ImportType("mpi4py.MPI", "Prequest", sizeof(PyMPIPrequestObject)); if (!__pyx_ptype_6mpi4py_3MPI_Prequest) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Grequest = __Pyx_ImportType("mpi4py.MPI", "Grequest", sizeof(PyMPIGrequestObject)); if (!__pyx_ptype_6mpi4py_3MPI_Grequest) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Op = __Pyx_ImportType("mpi4py.MPI", "Op", sizeof(PyMPIOpObject)); if (!__pyx_ptype_6mpi4py_3MPI_Op) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Group = __Pyx_ImportType("mpi4py.MPI", "Group", sizeof(PyMPIGroupObject)); if (!__pyx_ptype_6mpi4py_3MPI_Group) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Info = __Pyx_ImportType("mpi4py.MPI", "Info", sizeof(PyMPIInfoObject)); if (!__pyx_ptype_6mpi4py_3MPI_Info) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Errhandler = __Pyx_ImportType("mpi4py.MPI", "Errhandler", sizeof(PyMPIErrhandlerObject)); if (!__pyx_ptype_6mpi4py_3MPI_Errhandler) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Comm = __Pyx_ImportType("mpi4py.MPI", "Comm", sizeof(PyMPICommObject)); if (!__pyx_ptype_6mpi4py_3MPI_Comm) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Intracomm = __Pyx_ImportType("mpi4py.MPI", "Intracomm", sizeof(PyMPIIntracommObject)); if (!__pyx_ptype_6mpi4py_3MPI_Intracomm) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Cartcomm = __Pyx_ImportType("mpi4py.MPI", "Cartcomm", sizeof(PyMPICartcommObject)); if (!__pyx_ptype_6mpi4py_3MPI_Cartcomm) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Graphcomm = __Pyx_ImportType("mpi4py.MPI", "Graphcomm", sizeof(PyMPIGraphcommObject)); if (!__pyx_ptype_6mpi4py_3MPI_Graphcomm) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Intercomm = __Pyx_ImportType("mpi4py.MPI", "Intercomm", sizeof(PyMPIIntercommObject)); if (!__pyx_ptype_6mpi4py_3MPI_Intercomm) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Win = __Pyx_ImportType("mpi4py.MPI", "Win", sizeof(PyMPIWinObject)); if (!__pyx_ptype_6mpi4py_3MPI_Win) goto bad;
  __pyx_ptype_6mpi4py_3MPI_File = __Pyx_ImportType("mpi4py.MPI", "File", sizeof(PyMPIFileObject)); if (!__pyx_ptype_6mpi4py_3MPI_File) goto bad;
  return 0;
  bad:
  Py_XDECREF(module);
  return -1;
}

#endif
