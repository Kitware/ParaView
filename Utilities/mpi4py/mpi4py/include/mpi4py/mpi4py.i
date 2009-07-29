/* ---------------------------------------------------------------- */

%header %{#include "mpi4py/mpi4py.h"%}
%init   %{if (import_mpi4py() < 0) return;%}

/* ---------------------------------------------------------------- */

%runtime %{
SWIGINTERNINLINE PyObject*
SWIG_getattr_this(PyObject* obj) {
  if (!obj) return NULL;
  obj = PyObject_GetAttr(obj, SWIG_This());
  if (!obj) PyErr_Clear();
  return obj;
}
SWIGINTERNINLINE int
SWIG_convert_ptr(PyObject *obj, void **ptr, swig_type_info *ty, int flags) {
  int res = SWIG_ConvertPtr(obj, ptr, ty, flags);
  if (!SWIG_IsOK(res)) {
    PyObject* _this = SWIG_getattr_this(obj);
    res = SWIG_ConvertPtr(_this, ptr, ty, flags);
    Py_XDECREF(_this);
  }
  return res;
}
#undef  SWIG_ConvertPtr
#define SWIG_ConvertPtr(obj, pptr, type, flags) \
        SWIG_convert_ptr(obj, pptr, type, flags)
%}

/* ---------------------------------------------------------------- */

%define %mpi4py_fragments(PyType, Type)
/* --- AsPtr --- */
%fragment(SWIG_AsPtr_frag(Type),"header") {
SWIGINTERN int
SWIG_AsPtr_dec(Type)(SWIG_Object input, Type **p) {
  if (input == Py_None) {
    if (p) *p = 0;
    return SWIG_OK;
  } else if (PyObject_TypeCheck(input,&PyMPI##PyType##_Type)) {
    if (p) *p = PyMPI##PyType##_Get(input);
    return SWIG_OK;
  } else {
    void *argp = 0;
    int res = SWIG_ConvertPtr(input,&argp,%descriptor(p_##Type), 0);
    if (!SWIG_IsOK(res)) return res;
    if (!argp) return SWIG_ValueError;
    if (p) *p = %static_cast(argp,Type*);
    return SWIG_OK;
  }
}
}
/* --- From --- */
%fragment(SWIG_From_frag(Type),"header")
{
SWIGINTERN SWIG_Object
SWIG_From_dec(Type)(Type v) {
  return PyMPI##PyType##_New(v);
}
}
%enddef /*mpi4py_fragments*/

/* ---------------------------------------------------------------- */

%define SWIG_TYPECHECK_MPI_Comm  600 %enddef

%define %mpi4py_checkcode(Type)
%checkcode(Type)
%enddef /*mpi4py_checkcode*/

/* ---------------------------------------------------------------- */

%define %mpi4py_typemap(PyType, Type)
%types(Type*);
%mpi4py_fragments(PyType, Type);
%typemaps_asptrfromn(%mpi4py_checkcode(Type), Type);
%enddef /*mpi4py_typemap*/

/* ---------------------------------------------------------------- */


/*
 * Local Variables:
 * mode: C
 * End:
 */
