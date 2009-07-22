cdef class Request:

    """
    Request
    """

    def __cinit__(self):
        self.ob_mpi = MPI_REQUEST_NULL

    def __dealloc__(self):
        if not (self.flags & PyMPI_OWNED): return
        CHKERR( _del_Request(&self.ob_mpi) )

    def __richcmp__(self, other, int op):
        if not isinstance(self,  Request): return NotImplemented
        if not isinstance(other, Request): return NotImplemented
        cdef Request s = self, o = other
        if   op == 2: return (s.ob_mpi == o.ob_mpi)
        elif op == 3: return (s.ob_mpi != o.ob_mpi)
        else: raise TypeError(mpistr("only '==' and '!='"))

    def __nonzero__(self):
        return self.ob_mpi != MPI_REQUEST_NULL

    # Completion Operations
    # ---------------------

    def Wait(self, Status status=None):
        """
        Wait for an MPI send or receive to complete.
        """
        cdef MPI_Status *statusp = _arg_Status(status)
        with nogil: CHKERR( MPI_Wait(&self.ob_mpi, statusp) )

    def Test(self, Status status=None):
        """
        Test for the completion of a send or receive.
        """
        cdef bint flag = 0
        cdef MPI_Status *statusp = _arg_Status(status)
        with nogil: CHKERR( MPI_Test(&self.ob_mpi, &flag, statusp) )
        return flag

    def Free(self):
        """
        Free a communication request
        """
        with nogil: CHKERR( MPI_Request_free(&self.ob_mpi) )

    def Get_status(self, Status status=None):
        """
        Non-destructive test for the
        completion of a request
        """
        cdef bint flag = 0
        cdef MPI_Status *statusp = _arg_Status(status)
        with nogil: CHKERR( MPI_Request_get_status(self.ob_mpi, &flag, statusp) )
        return flag

    # Multiple Completions
    # --------------------

    @classmethod
    def Waitany(cls, requests, Status status=None):
        """
        Wait for any previously initiated request to complete
        """
        cdef int count = len(requests)
        cdef MPI_Request *irequests = NULL
        cdef tmp1 = asarray_Request(requests, &irequests, count)
        cdef MPI_Status *statusp = _arg_Status(status)
        cdef int index = MPI_UNDEFINED
        #
        try:
            with nogil: CHKERR( MPI_Waitany(count, irequests, &index, statusp) )
        finally:
            restore_Request(requests, &irequests, count);
        #
        return index

    @classmethod
    def Testany(cls, requests, Status status=None):
        """
        Test for completion of any previously initiated request
        """
        cdef int count = len(requests)
        cdef MPI_Request *irequests = NULL
        cdef tmp1 = asarray_Request(requests, &irequests, count)
        cdef int index = MPI_UNDEFINED
        cdef bint flag = 0
        cdef MPI_Status *statusp = _arg_Status(status)
        #
        try:
            with nogil: CHKERR( MPI_Testany(count, irequests, &index, &flag, statusp) )
        finally:
            restore_Request(requests, &irequests, count)
        #
        return (index, flag)

    @classmethod
    def Waitall(cls, requests, statuses=None):
        """
        Wait for all previously initiated requests to complete
        """
        cdef int count = len(requests)
        cdef MPI_Request *irequests = NULL
        cdef tmp1 = asarray_Request(requests, &irequests, count)
        cdef MPI_Status *istatuses = MPI_STATUSES_IGNORE
        cdef tmp2 = asarray_Status(statuses, &istatuses, count)
        #
        try:
            with nogil: CHKERR( MPI_Waitall(count, irequests, istatuses) )
        finally:
            restore_Request(requests, &irequests, count)
            restore_Status(statuses, &istatuses, count)
        #
        return None

    @classmethod
    def Testall(cls, requests, statuses=None):
        """
        Test for completion of all previously initiated requests
        """
        cdef int count = len(requests)
        cdef MPI_Request *irequests = NULL
        cdef tmp1 = asarray_Request(requests, &irequests, count)
        cdef MPI_Status *istatuses = MPI_STATUSES_IGNORE
        cdef tmp2 = asarray_Status(statuses, &istatuses, count)
        cdef bint flag = 0
        #
        try:
            with nogil: CHKERR( MPI_Testall(count, irequests, &flag, istatuses) )
        finally:
            restore_Request(requests, &irequests, count)
            restore_Status(statuses, &istatuses, count)
        #
        return flag

    @classmethod
    def Waitsome(cls, requests, statuses=None):
        """
        Wait for some previously initiated requests to complete
        """
        cdef int incount = len(requests)
        cdef MPI_Request *irequests = NULL
        cdef tmp1 = asarray_Request(requests, &irequests, incount)
        cdef MPI_Status *istatuses = MPI_STATUSES_IGNORE
        cdef tmp2 = asarray_Status(statuses, &istatuses, incount)
        cdef int outcount = MPI_UNDEFINED
        cdef int *iindices = NULL
        cdef tmp3 = newarray_int(incount, &iindices)
        #
        try:
            with nogil: CHKERR( MPI_Waitsome(incount, irequests, &outcount, iindices, istatuses) )
        finally:
            restore_Request(requests, &irequests, incount)
            restore_Status(statuses, &istatuses, incount)
        #
        cdef int i = 0
        indices = []
        if outcount != 0 and outcount != MPI_UNDEFINED:
            indices = [iindices[i] for i from 0 <= i < outcount]
        return (outcount, indices)

    @classmethod
    def Testsome(cls, requests, statuses=None):
        """
        Test for completion of some previously initiated requests
        """
        cdef int incount = len(requests)
        cdef MPI_Request *irequests = NULL
        cdef tmp1 = asarray_Request(requests, &irequests, incount)
        cdef MPI_Status *istatuses = MPI_STATUSES_IGNORE
        cdef tmp2 = asarray_Status(statuses, &istatuses, incount)
        cdef int outcount = MPI_UNDEFINED
        cdef int *iindices = NULL
        cdef tmp3 = newarray_int(incount, &iindices)
        #
        try:
            with nogil: CHKERR( MPI_Waitsome(incount, irequests, &outcount, iindices, istatuses) )
        finally:
            restore_Request(requests, &irequests, incount)
            restore_Status(statuses, &istatuses, incount)
        #
        cdef int i = 0
        indices = []
        if outcount != 0 and outcount != MPI_UNDEFINED:
            indices = [iindices[i] for i from 0 <= i < outcount]
        return (outcount, indices)

    # Cancel
    # ------

    def Cancel(self):
        """
        Cancel a communication request
        """
        with nogil: CHKERR( MPI_Cancel(&self.ob_mpi) )

    # Fortran Handle
    # --------------

    def py2f(self):
        """
        """
        return MPI_Request_c2f(self.ob_mpi)

    @classmethod
    def f2py(cls, arg):
        """
        """
        cdef Request request = cls()
        request.ob_mpi = MPI_Request_f2c(arg)
        return request



cdef class Prequest(Request):

    """
    Persistent request
    """

    def Start(self):
        """
        Initiate a communication with a persistent request
        """
        with nogil: CHKERR( MPI_Start(&self.ob_mpi) )

    @classmethod
    def Startall(cls, requests):
        """
        Start a collection of persistent requests
        """
        cdef int count = len(requests)
        cdef MPI_Request *irequests = NULL
        cdef tmp = asarray_Request(requests, &irequests, count)
        #
        try:
            with nogil: CHKERR( MPI_Startall(count, irequests) )
        finally:
            restore_Request(requests, &irequests, count)



cdef class Grequest(Request):

    """
    Generalized request
    """

    def __cinit__(self):
        self.ob_grequest = MPI_REQUEST_NULL

    @classmethod
    def Start(cls, query_fn, free_fn, cancel_fn,
              args=None, kargs=None):
        """
        Create and return a user-defined request
        """
        #
        cdef Grequest request = cls()
        cdef _p_greq state = \
             _p_greq(query_fn, free_fn, cancel_fn,
                     args, kargs)
        with nogil: CHKERR( MPI_Grequest_start(
            greq_query_fn, greq_free_fn, greq_cancel_fn,
            <void*>state, &request.ob_mpi) )
        Py_INCREF(state)
        request.ob_grequest = request.ob_mpi
        return request

    def Complete(self):
        """
        Notify that a user-defined request is complete
        """
        if self.ob_mpi != MPI_REQUEST_NULL:
            if self.ob_mpi != self.ob_grequest:
                raise Exception(MPI_ERR_REQUEST)
        cdef MPI_Request grequest = self.ob_grequest
        self.ob_grequest = self.ob_mpi # sync handles
        with nogil: CHKERR( MPI_Grequest_complete(grequest) )



cdef Request __REQUEST_NULL__ = _new_Request(MPI_REQUEST_NULL)


# Predefined request handles
# --------------------------

REQUEST_NULL = __REQUEST_NULL__  #: Null request handle

