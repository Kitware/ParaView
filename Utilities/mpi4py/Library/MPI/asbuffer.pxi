#---------------------------------------------------------------------

cdef extern from "Python.h":
    object PyLong_FromVoidPtr(void*)
    void*  PyLong_AsVoidPtr(object)

cdef extern from "Python.h":
    ctypedef void const_void "const void"
    int PyObject_AsReadBuffer (object, const_void**, Py_ssize_t*) except -1
    int PyObject_AsWriteBuffer(object, void**, Py_ssize_t*) except -1

cdef extern from "Python.h":
    object PyBuffer_FromReadWriteMemory(void*, Py_ssize_t)

#---------------------------------------------------------------------

cdef inline object asbuffer_r(object ob, void** bptr, MPI_Aint* blen):
    cdef const_void *p = NULL
    cdef Py_ssize_t n = 0
    PyObject_AsReadBuffer(ob, &p, &n)
    if bptr: bptr[0] = <void *>p
    if blen: blen[0] = <MPI_Aint>n
    return ob

cdef inline object asbuffer_w(object ob, void** bptr, MPI_Aint* blen):
    cdef void *p = NULL
    cdef Py_ssize_t n = 0
    PyObject_AsWriteBuffer(ob, &p, &n)
    if bptr: bptr[0] = p
    if blen: blen[0] = <MPI_Aint>n
    return ob

#---------------------------------------------------------------------
