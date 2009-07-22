#---------------------------------------------------------------------

cdef inline object asmemory(object ob, void** base, MPI_Aint* size):
    cdef void *p = NULL
    cdef Py_ssize_t n = 0
    PyObject_AsWriteBuffer(ob, &p, &n)
    if base: base[0] = p
    if size: size[0] = n
    return ob

cdef inline object tomemory(void *base, MPI_Aint size):
    return PyBuffer_FromReadWriteMemory(base, <Py_ssize_t>size)

#---------------------------------------------------------------------
