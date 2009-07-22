
# --------------------------------------------------------------------

cdef extern from "Python.h":
    ctypedef struct PyObject
    Py_ssize_t Py_REFCNT(object)
    void Py_INCREF(object) except *
    void Py_DECREF(object) except *
    int Py_IsInitialized() nogil
    void PySys_WriteStderr(char*,...)
    int Py_AtExit(void (*)())

# --------------------------------------------------------------------

cdef extern from "atimport.h":
    int PyMPI_KEYVAL_ATEXIT_MPI

# --------------------------------------------------------------------

cdef int inited_atimport = 0
cdef int finalize_atexit = 0
cdef int startup_done = 0
cdef int cleanup_done = 0

cdef MPI_Errhandler comm_self_eh  = MPI_ERRHANDLER_NULL
cdef MPI_Errhandler comm_world_eh = MPI_ERRHANDLER_NULL
cdef int PyMPI_KEYVAL_WIN_MEMORY  = MPI_KEYVAL_INVALID

ctypedef struct RCParams:
    int initialize
    int threaded
    int thread_level
    int finalize

cdef int warnRC(object attr, object value) except -1:
    from warnings import warn
    warn(u"mpi4py.rc: '%s': unexpected value '%r'" % (attr, value))

cdef int getRCParams(RCParams* rc) except -1:
    rc.initialize = 1
    rc.threaded = 1
    rc.thread_level = MPI_THREAD_MULTIPLE
    rc.finalize = 1
    #
    try: from mpi4py import rc as rcmod
    except: return 0
    #
    cdef object initialize = True
    cdef object threaded = True
    cdef object thread_level = u'multiple'
    cdef object finalize = True
    try: initialize = rcmod.initialize
    except: pass
    try: threaded = rcmod.threaded
    except: pass
    try: thread_level = rcmod.thread_level
    except: pass
    try: finalize = rcmod.finalize
    except: pass
    #
    if initialize in (True, u'yes'):
        rc.initialize = 1
    elif initialize in (False, u'no'):
        rc.initialize = 0
    else:
        warnRC(u"initialize", initialize)
    #
    if threaded in (True, u'yes'):
        rc.threaded = 1
    elif threaded in (False, u'no'):
        rc.threaded = 0
    else:
        warnRC(u"threaded", threaded)
    #
    if thread_level == u'single':
        rc.thread_level = MPI_THREAD_SINGLE
    elif thread_level == u'funneled':
        rc.thread_level = MPI_THREAD_FUNNELED
    elif thread_level == u'serialized':
        rc.thread_level = MPI_THREAD_SERIALIZED
    elif thread_level == u'multiple':
        rc.thread_level = MPI_THREAD_MULTIPLE
    else:
        warnRC(u"thread_level", thread_level)
    #
    if finalize in (True, u'yes'):
        rc.finalize = 1
    elif finalize in (False, u'no'):
        rc.finalize = 0
    else:
        warnRC(u"finalize", finalize)
    #
    return 0

cdef int initialize() except -1:
    global inited_atimport
    global finalize_atexit
    cdef int ierr = MPI_SUCCESS
    # MPI initialized ?
    cdef int initialized = 1
    ierr = MPI_Initialized(&initialized)
    # MPI finalized ?
    cdef int finalized = 1
    ierr = MPI_Finalized(&finalized)
    # Do we have to initialize MPI?
    if initialized:
        if not finalized:
            # cleanup at (the very end of) Python exit
            if Py_AtExit(atexit_py) < 0:
                PySys_WriteStderr("warning: could not register"
                                  "cleanup with Py_AtExit()")
        return 0
    # Use user parameters from 'mpi4py.rc' module
    cdef RCParams rc
    getRCParams(&rc)
    cdef int required = MPI_THREAD_SINGLE
    cdef int provided = MPI_THREAD_SINGLE
    if rc.initialize: # We have to initialize MPI
        if rc.threaded:
            required = rc.thread_level
            ierr = MPI_Init_thread(NULL, NULL, required, &provided)
            if ierr != MPI_SUCCESS: raise RuntimeError(
                u"MPI_Init_thread() failed [error code: %d]" % ierr)
        else:
            ierr = MPI_Init(NULL, NULL)
            if ierr != MPI_SUCCESS: raise RuntimeError(
                u"MPI_Init() failed [error code: %d]" % ierr)
        inited_atimport = 1 # We initialized MPI
        if rc.finalize:     # We have to finalize MPI
            finalize_atexit = 1
    # Cleanup at (the very end of) Python exit
    if Py_AtExit(atexit_py) < 0:
        PySys_WriteStderr("warning: could not register"
                          "cleanup with Py_AtExit()")
    return 0

cdef inline int mpi_active() nogil:
    cdef int ierr = MPI_SUCCESS
    # MPI initialized ?
    cdef int initialized = 0
    ierr = MPI_Initialized(&initialized)
    if not initialized: return 0
    # MPI finalized ?
    cdef int finalized = 1
    ierr = MPI_Finalized(&finalized)
    if finalized: return 0
    # MPI should be active ...
    return 1

cdef void startup() nogil:
    cdef int ierr = MPI_SUCCESS
    if not mpi_active(): return
    #
    global startup_done
    if startup_done: return
    startup_done = 1
    #DBG# fprintf(stderr, "statup: BEGIN\n"); fflush(stderr)
    # change error handlers for predefined communicators
    global comm_world_eh
    if comm_world_eh == MPI_ERRHANDLER_NULL:
        ierr = MPI_Comm_get_errhandler(MPI_COMM_WORLD, &comm_world_eh)
        ierr = MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN)
    global comm_self_eh
    if comm_self_eh == MPI_ERRHANDLER_NULL:
        ierr = MPI_Comm_get_errhandler(MPI_COMM_SELF,  &comm_self_eh)
        ierr = MPI_Comm_set_errhandler(MPI_COMM_SELF,  MPI_ERRORS_RETURN)
    # make the call to MPI_Finalize() run a cleanup function
    global PyMPI_KEYVAL_ATEXIT_MPI
    cdef int* keyval = &PyMPI_KEYVAL_ATEXIT_MPI
    if keyval[0] == MPI_KEYVAL_INVALID:
        ierr = MPI_Comm_create_keyval(MPI_COMM_NULL_COPY_FN,
                                      atexit_mpi, keyval, NULL)
        ierr = MPI_Comm_set_attr(MPI_COMM_SELF, keyval[0], NULL)
    #DBG# fprintf(stderr, "statup: END\n"); fflush(stderr)

cdef void cleanup() nogil:
    cdef int ierr = MPI_SUCCESS
    if not mpi_active(): return
    #
    global cleanup_done
    if cleanup_done: return
    cleanup_done = 1
    #
    #DBG# fprintf(stderr, "cleanup: BEGIN\n"); fflush(stderr)
    # free cleanup keyval
    global PyMPI_KEYVAL_ATEXIT_MPI
    if PyMPI_KEYVAL_ATEXIT_MPI != MPI_KEYVAL_INVALID:
        ierr = MPI_Comm_free_keyval(&PyMPI_KEYVAL_ATEXIT_MPI)
        PyMPI_KEYVAL_ATEXIT_MPI = MPI_KEYVAL_INVALID
    # free windows keyval
    global PyMPI_KEYVAL_WIN_MEMORY
    if PyMPI_KEYVAL_WIN_MEMORY != MPI_KEYVAL_INVALID:
        ierr = MPI_Win_free_keyval(&PyMPI_KEYVAL_WIN_MEMORY)
        PyMPI_KEYVAL_WIN_MEMORY = MPI_KEYVAL_INVALID
    # restore default error handlers for predefined communicators
    global comm_self_eh
    if comm_self_eh != MPI_ERRHANDLER_NULL:
        ierr = MPI_Comm_set_errhandler(MPI_COMM_SELF, comm_self_eh)
        ierr = MPI_Errhandler_free(&comm_self_eh)
        comm_self_eh = MPI_ERRHANDLER_NULL
    global comm_world_eh
    if comm_world_eh != MPI_ERRHANDLER_NULL:
        ierr = MPI_Comm_set_errhandler(MPI_COMM_WORLD, comm_world_eh)
        ierr = MPI_Errhandler_free(&comm_world_eh)
        comm_world_eh = MPI_ERRHANDLER_NULL
    #DBG# fprintf(stderr, "cleanup: END\n"); fflush(stderr)

cdef int atexit_mpi(MPI_Comm c,int k, void *v, void *xs) nogil:
    #DBG# fprintf(stderr, "atexit_mpi: BEGIN\n"); fflush(stderr)
    cleanup()
    #DBG# fprintf(stderr, "atexit_mpi: END\n"); fflush(stderr)
    return MPI_SUCCESS

cdef void atexit_py() nogil:
    global cleanup_done
    global finalize_atexit
    cdef int ierr = MPI_SUCCESS
    if not mpi_active(): return
    #DBG# fprintf(stderr, "atexit_py: BEGIN\n"); fflush(stderr)
    if not cleanup_done:
        cleanup()
    if finalize_atexit:
        ierr = MPI_Finalize()
    #DBG# fprintf(stderr, "atexit_py: END\n"); fflush(stderr)

# --------------------------------------------------------------------

# Vile hack for raising a exception and not contaminate the traceback

cdef extern from *:
    void __Pyx_Raise(object, object, void*)

cdef extern from *:
    PyObject *PyExc_RuntimeError
    PyObject *PyExc_NotImplementedError

cdef object MPIException = <object>PyExc_RuntimeError

cdef int PyMPI_Raise(int ierr) except -1 with gil:
    if ierr != -1:
        if (<void*>MPIException) != NULL:
            __Pyx_Raise(MPIException, ierr, NULL)
        else:
            __Pyx_Raise(<object>PyExc_RuntimeError, ierr, NULL)
    else:
        __Pyx_Raise(<object>PyExc_NotImplementedError, None, NULL)
    return 0

cdef inline int CHKERR(int ierr) nogil except -1:
    if ierr == 0: return 0
    PyMPI_Raise(ierr)
    return -1

# --------------------------------------------------------------------
