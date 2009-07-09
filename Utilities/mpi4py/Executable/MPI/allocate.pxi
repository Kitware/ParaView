#---------------------------------------------------------------------

cdef extern from "Python.h":
    void *PyMem_Malloc(size_t)
    void PyMem_Free(void *)
    object PyCObject_FromVoidPtr(void *, void (*)(void*))

#---------------------------------------------------------------------

cdef inline void *memnew(size_t n):
    return PyMem_Malloc(n)

cdef inline void memdel(void *p):
    PyMem_Free(p)

cdef inline object allocate(size_t n, void **pp):
    cdef object cob
    cdef void *p = memnew(n)
    if p == NULL: raise MemoryError
    try:    cob = PyCObject_FromVoidPtr(p, memdel)
    except: memdel(p); raise
    pp[0] = p
    return cob

#---------------------------------------------------------------------
