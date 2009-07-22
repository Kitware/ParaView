cdef void win_memory_pydecref(void *ob) with gil:
    Py_DECREF(<object>ob)

cdef int PyMPI_Win_memory_del(MPI_Win w, int k, void *v, void *xs) nogil:
    if  v != NULL:
        if Py_IsInitialized():
            win_memory_pydecref(v)
    return 0

cdef int PyMPI_Win_set_attr_memory(MPI_Win win, object memory) except -1:
    if memory is None: return 0
    # create keyval for memory object
    global PyMPI_KEYVAL_WIN_MEMORY
    if PyMPI_KEYVAL_WIN_MEMORY == MPI_KEYVAL_INVALID:
        CHKERR( MPI_Win_create_keyval(MPI_WIN_NULL_COPY_FN,
                                      PyMPI_Win_memory_del,
                                      &PyMPI_KEYVAL_WIN_MEMORY, NULL) )
    # hold a reference to the object exposing windows memory
    CHKERR( MPI_Win_set_attr(win, PyMPI_KEYVAL_WIN_MEMORY, <void*>memory) )
    Py_INCREF(memory)
    return 0
