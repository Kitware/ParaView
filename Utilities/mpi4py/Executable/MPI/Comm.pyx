# Communicator Comparisons
# ------------------------

IDENT     = MPI_IDENT      #: Groups are identical, communicator contexts are de same
CONGRUENT = MPI_CONGRUENT  #: Groups are identical, contexts are different
SIMILAR   = MPI_SIMILAR    #: Groups are similar, rank order differs
UNEQUAL   = MPI_UNEQUAL    #: Groups are different


# Communicator Topologies
# -----------------------

CART  = MPI_CART   #: Cartesian topology
GRAPH = MPI_GRAPH  #: Graph topology


cdef class Comm:

    """
    Communicator
    """

    def __cinit__(self):
        self.ob_mpi = MPI_COMM_NULL

    def __dealloc__(self):
        if not (self.flags & PyMPI_OWNED): return
        CHKERR( _del_Comm(&self.ob_mpi) )

    def __richcmp__(self, other, int op):
        if not isinstance(self,  Comm): return NotImplemented
        if not isinstance(other, Comm): return NotImplemented
        cdef Comm s = self, o = other
        if   op == 2: return (s.ob_mpi == o.ob_mpi)
        elif op == 3: return (s.ob_mpi != o.ob_mpi)
        else: raise TypeError(mpistr("only '==' and '!='"))

    def __nonzero__(self):
        return self.ob_mpi != MPI_COMM_NULL

    # Group
    # -----

    def Get_group(self):
        """
        Access the group associated with a communicator
        """
        cdef Group group = Group()
        CHKERR( MPI_Comm_group(self.ob_mpi, &group.ob_mpi) )
        return group

    property group:
        """communicator group"""
        def __get__(self):
            return self.Get_group()

    # Communicator Accessors
    # ----------------------

    def Get_size(self):
        """
        Return the size of a communicator
        """
        cdef int size = -1
        CHKERR( MPI_Comm_size(self.ob_mpi, &size) )
        return size

    property size:
        """number of processes in communicator"""
        def __get__(self):
            return self.Get_size()

    def Get_rank(self):
        """
        Return the rank of this process in a communicator
        """
        cdef int rank = MPI_PROC_NULL
        CHKERR( MPI_Comm_rank(self.ob_mpi, &rank) )
        return rank

    property rank:
        """rank of this process in communicator"""
        def __get__(self):
            return self.Get_rank()

    @classmethod
    def Compare(cls, Comm comm1 not None, Comm comm2 not None):
        """
        Compare two communicators
        """
        cdef int flag = MPI_UNEQUAL
        CHKERR( MPI_Comm_compare(comm1.ob_mpi, comm2.ob_mpi, &flag) )
        return flag

    # Communicator Constructors
    # -------------------------

    def Clone(self):
        """
        Clone an existing communicator
        """
        cdef Comm comm = type(self)()
        with nogil: CHKERR( MPI_Comm_dup(self.ob_mpi, &comm.ob_mpi) )
        return comm

    # Communicator Destructor
    # -----------------------

    def Free(self):
        """
        Free a communicator
        """
        with nogil: CHKERR( MPI_Comm_free(&self.ob_mpi) )

    # Point to Point communication
    # ----------------------------

    # Blocking Send and Receive Operations
    # ------------------------------------

    def Send(self, buf, int dest=0, int tag=0):
        """
        Blocking send

        .. note:: This function may block until the message is
           received. Whether or not `Send` blocks depends on
           several factors and is implementation dependent
        """
        cdef _p_msg_p2p smsg = message_p2p_send(buf, dest)
        with nogil: CHKERR( MPI_Send(
            smsg.buf, smsg.count, smsg.dtype,
            dest, tag, self.ob_mpi) )

    def Recv(self, buf, int source=0, int tag=0, Status status=None):
        """
        Blocking receive

        .. note:: This function blocks until the message is received
        """
        cdef _p_msg_p2p rmsg = message_p2p_recv(buf, source)
        cdef MPI_Status *statusp = _arg_Status(status)
        with nogil: CHKERR( MPI_Recv(
            rmsg.buf, rmsg.count, rmsg.dtype,
            source, tag, self.ob_mpi, statusp) )

    # Send-Receive
    # ------------

    def Sendrecv(self, sendbuf, int dest=0, int sendtag=0,
                 recvbuf=None, int source=0, int recvtag=0,
                 Status status=None):
        """
        Send and receive a message

        .. note:: This function is guaranteed not to deadlock in
           situations where pairs of blocking sends and receives may
           deadlock.

        .. caution:: A common mistake when using this function is to
           mismatch the tags with the source and destination ranks,
           which can result in deadlock.
        """
        cdef _p_msg_p2p smsg = message_p2p_send(sendbuf, dest)
        cdef _p_msg_p2p rmsg = message_p2p_recv(recvbuf, source)
        cdef MPI_Status *statusp = _arg_Status(status)
        with nogil: CHKERR( MPI_Sendrecv(
            smsg.buf, smsg.count, smsg.dtype, dest,   sendtag,
            rmsg.buf, rmsg.count, rmsg.dtype, source, recvtag,
            self.ob_mpi, statusp) )

    def Sendrecv_replace(self, buf,
                         int dest=0,  int sendtag=0,
                         int source=0, int recvtag=0,
                         Status status=None):
        """
        Send and receive a message

        .. note:: This function is guaranteed not to deadlock in
           situations where pairs of blocking sends and receives may
           deadlock.

        .. caution:: A common mistake when using this function is to
           mismatch the tags with the source and destination ranks,
           which can result in deadlock.
        """
        cdef int rank = MPI_PROC_NULL
        if dest   != MPI_PROC_NULL: rank = dest
        if source != MPI_PROC_NULL: rank = source
        cdef _p_msg_p2p rmsg = message_p2p_recv(buf, rank)
        cdef MPI_Status *statusp = _arg_Status(status)
        with nogil: CHKERR( MPI_Sendrecv_replace(
                rmsg.buf, rmsg.count, rmsg.dtype,
                dest, sendtag, source, recvtag,
                self.ob_mpi, statusp) )

    # Nonblocking Communications
    # --------------------------

    def Isend(self, buf, int dest=0, int tag=0):
        """
        Nonblocking send
        """
        cdef _p_msg_p2p smsg = message_p2p_send(buf, dest)
        cdef Request request = Request()
        with nogil: CHKERR( MPI_Isend(
            smsg.buf, smsg.count, smsg.dtype,
            dest, tag, self.ob_mpi, &request.ob_mpi) )
        return request

    def Irecv(self, buf, int source=0, int tag=0):
        """
        Nonblocking receive
        """
        cdef _p_msg_p2p rmsg = message_p2p_recv(buf, source)
        cdef Request request = Request()
        with nogil: CHKERR( MPI_Irecv(
            rmsg.buf, rmsg.count, rmsg.dtype,
            source, tag, self.ob_mpi, &request.ob_mpi) )
        return request

    # Probe
    # -----

    def Probe(self, int source=0, int tag=0, Status status=None):
        """
        Blocking test for a message

        .. note:: This function blocks until the message arrives.
        """
        cdef MPI_Status *statusp = _arg_Status(status)
        with nogil: CHKERR( MPI_Probe(
            source, tag, self.ob_mpi, statusp) )

    def Iprobe(self, source=0, tag=0, Status status=None):
        """
        Nonblocking test for a message
        """
        cdef bint flag = 0
        cdef MPI_Status *statusp = _arg_Status(status)
        with nogil: CHKERR( MPI_Iprobe(
            source, tag, self.ob_mpi, &flag, statusp) )
        return flag

    # Persistent Communication
    # ------------------------

    def Send_init(self, buf, int dest=0, int tag=0):
        """
        Create a persistent request for a standard send
        """
        cdef _p_msg_p2p smsg = message_p2p_send(buf, dest)
        cdef Prequest request = Prequest()
        with nogil: CHKERR( MPI_Send_init(
            smsg.buf, smsg.count, smsg.dtype,
            dest, tag, self.ob_mpi, &request.ob_mpi) )
        return request

    def Recv_init(self, buf, int source=0, int tag=0):
        """
        Create a persistent request for a receive
        """
        cdef _p_msg_p2p rmsg = message_p2p_recv(buf, source)
        cdef Prequest request = Prequest()
        with nogil: CHKERR( MPI_Recv_init(
            rmsg.buf, rmsg.count, rmsg.dtype,
            source, tag, self.ob_mpi, &request.ob_mpi) )
        return request

    # Communication Modes
    # -------------------

    # Blocking calls

    def Bsend(self, buf, int dest=0, int tag=0):
        """
        Blocking send in buffered mode
        """
        cdef _p_msg_p2p smsg = message_p2p_send(buf, dest)
        with nogil: CHKERR( MPI_Bsend(
            smsg.buf, smsg.count, smsg.dtype,
            dest, tag, self.ob_mpi) )

    def Ssend(self, buf, dest=0, tag=0):
        """
        Blocking send in synchronous mode
        """
        cdef _p_msg_p2p smsg = message_p2p_send(buf, dest)
        with nogil: CHKERR( MPI_Ssend(
            smsg.buf, smsg.count, smsg.dtype,
            dest, tag, self.ob_mpi) )

    def Rsend(self, buf, int dest=0, int tag=0):
        """
        Blocking send in ready mode
        """
        cdef _p_msg_p2p smsg = message_p2p_send(buf, dest)
        with nogil: CHKERR( MPI_Rsend(
            smsg.buf, smsg.count, smsg.dtype,
            dest, tag, self.ob_mpi) )

    # Nonblocking calls

    def Ibsend(self, buf, int dest=0, int tag=0):
        """
        Nonblocking send in buffered mode
        """
        cdef _p_msg_p2p smsg = message_p2p_send(buf, dest)
        cdef Request request = Request()
        with nogil: CHKERR( MPI_Ibsend(
            smsg.buf, smsg.count, smsg.dtype,
            dest, tag, self.ob_mpi, &request.ob_mpi) )
        return request

    def Issend(self, buf, int dest=0, int tag=0):
        """
        Nonblocking send in synchronous mode
        """
        cdef _p_msg_p2p smsg = message_p2p_send(buf, dest)
        cdef Request request = Request()
        with nogil: CHKERR( MPI_Issend(
            smsg.buf, smsg.count, smsg.dtype,
            dest, tag, self.ob_mpi, &request.ob_mpi) )
        return request

    def Irsend(self, buf, int dest=0, int tag=0):
        """
        Nonblocking send in ready mode
        """
        cdef _p_msg_p2p smsg = message_p2p_send(buf, dest)
        cdef Request request = Request()
        with nogil: CHKERR( MPI_Irsend(
            smsg.buf, smsg.count, smsg.dtype,
            dest, tag, self.ob_mpi, &request.ob_mpi) )
        return request

    # Persistent Requests

    def Bsend_init(self, buf, int dest=0, int tag=0):
        """
        Persistent request for a send in buffered mode
        """
        cdef _p_msg_p2p smsg = message_p2p_send(buf, dest)
        cdef Prequest request = Prequest()
        with nogil: CHKERR( MPI_Bsend_init(
            smsg.buf, smsg.count, smsg.dtype,
            dest, tag, self.ob_mpi, &request.ob_mpi) )
        return request

    def Ssend_init(self, buf, int dest=0, int tag=0):
        """
        Persistent request for a send in synchronous mode
        """
        cdef _p_msg_p2p smsg = message_p2p_send(buf, dest)
        cdef Prequest request = Prequest()
        with nogil: CHKERR( MPI_Ssend_init(
            smsg.buf, smsg.count, smsg.dtype,
            dest, tag, self.ob_mpi, &request.ob_mpi) )
        return request

    def Rsend_init(self, buf, int dest=0, int tag=0):
        """
        Persistent request for a send in ready mode
        """
        cdef _p_msg_p2p smsg = message_p2p_send(buf, dest)
        cdef Prequest request = Prequest()
        with nogil: CHKERR( MPI_Rsend_init(
            smsg.buf, smsg.count, smsg.dtype,
            dest, tag, self.ob_mpi, &request.ob_mpi) )
        return request

    # Collective Communications
    # -------------------------

    # Barrier Synchronization
    # -----------------------

    def Barrier(self):
        """
        Barrier synchronization
        """
        with nogil: CHKERR( MPI_Barrier(self.ob_mpi) )

    # Global Communication Functions
    # ------------------------------

    def Bcast(self, buf, int root=0):
        """
        Broadcast a message from one process
        to all other processes in a group
        """
        cdef _p_msg_cco m = message_cco()
        m.for_bcast(buf, root, self.ob_mpi)
        with nogil: CHKERR( MPI_Bcast(
            m.sbuf, m.scount, m.stype,
            root, self.ob_mpi) )

    def Gather(self, sendbuf, recvbuf, int root=0):
        """
        Gather together values from a group of processes
        """
        cdef _p_msg_cco m = message_cco()
        m.for_gather(0, sendbuf, recvbuf, root, self.ob_mpi)
        with nogil: CHKERR( MPI_Gather(
            m.sbuf, m.scount, m.stype,
            m.rbuf, m.rcount, m.rtype,
            root, self.ob_mpi) )

    def Gatherv(self, sendbuf, recvbuf, int root=0):
        """
        Gather Vector, gather data to one process from all other
        processes in a group providing different amount of data and
        displacements at the receiving sides
        """
        cdef _p_msg_cco m = message_cco()
        m.for_gather(1, sendbuf, recvbuf, root, self.ob_mpi)
        with nogil: CHKERR( MPI_Gatherv(
            m.sbuf, m.scount,             m.stype,
            m.rbuf, m.rcounts, m.rdispls, m.rtype,
            root, self.ob_mpi) )

    def Scatter(self, sendbuf, recvbuf, int root=0):
        """
        Scatter Vector, scatter data from one process
        to all other processes in a group
        """
        cdef _p_msg_cco m = message_cco()
        m.for_scatter(0, sendbuf, recvbuf, root, self.ob_mpi)
        with nogil: CHKERR( MPI_Scatter(
            m.sbuf, m.scount, m.stype,
            m.rbuf, m.rcount, m.rtype,
            root, self.ob_mpi) )

    def Scatterv(self, sendbuf, recvbuf, int root=0):
        """
        Scatter data from one process to all other processes in a
        group providing different amount of data and displacements at
        the sending side
        """
        cdef _p_msg_cco m = message_cco()
        m.for_scatter(1, sendbuf, recvbuf, root, self.ob_mpi)
        with nogil: CHKERR( MPI_Scatterv(
            m.sbuf, m.scounts, m.sdispls, m.stype,
            m.rbuf, m.rcount,             m.rtype,
            root, self.ob_mpi) )

    def Allgather(self, sendbuf, recvbuf):
        """
        Gather to All, gather data from all processes and
        distribute it to all other processes in a group
        """
        cdef _p_msg_cco m = message_cco()
        m.for_allgather(0, sendbuf, recvbuf, self.ob_mpi)
        with nogil: CHKERR( MPI_Allgather(
            m.sbuf, m.scount, m.stype,
            m.rbuf, m.rcount, m.rtype,
            self.ob_mpi) )

    def Allgatherv(self, sendbuf, recvbuf):
        """
        Gather to All Vector, gather data from all processes and
        distribute it to all other processes in a group providing
        different amount of data and displacements
        """
        cdef _p_msg_cco m = message_cco()
        m.for_allgather(1, sendbuf, recvbuf, self.ob_mpi)
        with nogil: CHKERR( MPI_Allgatherv(
            m.sbuf, m.scount,             m.stype,
            m.rbuf, m.rcounts, m.rdispls, m.rtype,
            self.ob_mpi) )

    def Alltoall(self, sendbuf, recvbuf):
        """
        All to All Scatter/Gather, send data from all to all
        processes in a group
        """
        cdef _p_msg_cco m = message_cco()
        m.for_alltoall(0, sendbuf, recvbuf, self.ob_mpi)
        with nogil: CHKERR( MPI_Alltoall(
            m.sbuf, m.scount, m.stype,
            m.rbuf, m.rcount, m.rtype,
            self.ob_mpi) )

    def Alltoallv(self, sendbuf, recvbuf):
        """
        All to All Scatter/Gather Vector, send data from all to all
        processes in a group providing different amount of data and
        displacements
        """
        cdef _p_msg_cco m = message_cco()
        m.for_alltoall(1, sendbuf, recvbuf, self.ob_mpi)
        with nogil: CHKERR( MPI_Alltoallv(
            m.sbuf, m.scounts, m.sdispls, m.stype,
            m.rbuf, m.rcounts, m.rdispls, m.rtype,
            self.ob_mpi) )

    def Alltoallw(self, sendbuf, recvbuf):
        """
        Generalized All-to-All communication allowing different
        counts, displacements and datatypes for each partner
        """
        raise NotImplementedError # XXX implement!
        cdef void *sbuf = NULL, *rbuf = NULL
        cdef int  *scounts = NULL, *rcounts = NULL
        cdef int  *sdispls = NULL, *rdispls = NULL
        cdef MPI_Datatype *stypes = NULL, *rtypes = NULL
        with nogil: CHKERR( MPI_Alltoallw(
            sbuf, scounts, sdispls, stypes,
            rbuf, rcounts, rdispls, rtypes,
            self.ob_mpi) )


    # Global Reduction Operations
    # ---------------------------

    def Reduce(self, sendbuf, recvbuf, Op op not None=SUM, int root=0):
        """
        Reduce
        """
        cdef _p_msg_cco m = message_cco()
        m.for_reduce(sendbuf, recvbuf, root, self.ob_mpi)
        with nogil: CHKERR( MPI_Reduce(
            m.sbuf, m.rbuf, m.rcount, m.rtype,
            op.ob_mpi, root, self.ob_mpi) )

    def Allreduce(self, sendbuf, recvbuf, Op op not None=SUM):
        """
        All Reduce
        """
        cdef _p_msg_cco m = message_cco()
        m.for_allreduce(sendbuf, recvbuf, self.ob_mpi)
        with nogil: CHKERR( MPI_Allreduce(
            m.sbuf, m.rbuf, m.rcount, m.rtype,
            op.ob_mpi, self.ob_mpi) )

    def Reduce_scatter(self, sendbuf, recvbuf, Op op not None=SUM):
        """
        Reduce-Scatter
        """
        raise NotImplementedError # XXX implement!
        cdef void *sbuf = NULL
        cdef void *rbuf = NULL
        cdef int *rcounts = NULL
        cdef MPI_Datatype rtype = MPI_DATATYPE_NULL
        with nogil: CHKERR( MPI_Reduce_scatter(
            sbuf, rbuf, rcounts, rtype,
            op.ob_mpi, self.ob_mpi) )

    # Tests
    # -----

    def Is_inter(self):
        """
        Test to see if a comm is an intercommunicator
        """
        cdef bint flag = 0
        CHKERR( MPI_Comm_test_inter(self.ob_mpi, &flag) )
        return flag

    property is_inter:
        """is intercommunicator"""
        def __get__(self):
            return self.Is_inter()

    def Is_intra(self):
        """
        Test to see if a comm is an intracommunicator
        """
        return not self.Is_inter()

    property is_intra:
        """is intracommunicator"""
        def __get__(self):
            return self.Is_intra()

    def Get_topology(self):
        """
        Determine the type of topology (if any)
        associated with a communicator
        """
        cdef int topo = MPI_UNDEFINED
        CHKERR( MPI_Topo_test(self.ob_mpi, &topo) )
        return topo

    property topology:
        """communicator topology type"""
        def __get__(self):
            return self.Get_topology()

    # Process Creation and Management
    # -------------------------------

    @classmethod
    def Get_parent(cls):
        """
        Return the parent intercommunicator for this process
        """
        cdef MPI_Comm comm = MPI_COMM_NULL
        with nogil: CHKERR( MPI_Comm_get_parent(&comm) )
        global __COMM_PARENT__
        cdef Intercomm parent = __COMM_PARENT__
        parent.ob_mpi = comm
        return parent

    def Disconnect(self):
        """
        Disconnect from a communicator
        """
        with nogil: CHKERR( MPI_Comm_disconnect(
            &self.ob_mpi) )

    @classmethod
    def Join(cls, int fd):
        """
        Create a intercommunicator by joining
        two processes connected by a socket
        """
        cdef Intercomm comm = Intercomm()
        with nogil: CHKERR( MPI_Comm_join(
            fd, &comm.ob_mpi) )
        return comm

    # Attributes
    # ----------

    def Get_attr(self, int keyval):
        """
        Retrieve attribute value by key
        """
        cdef void *attrval = NULL
        cdef int  flag = 0
        CHKERR(MPI_Comm_get_attr(self.ob_mpi, keyval, &attrval, &flag) )
        if not flag: return None
        if not attrval: return 0
        # MPI-1 predefined attribute keyvals
        if ((keyval == <int>MPI_TAG_UB) or
            (keyval == <int>MPI_HOST) or
            (keyval == <int>MPI_IO) or
            (keyval == <int>MPI_WTIME_IS_GLOBAL)):
            return (<int*>attrval)[0]
        # MPI-2 predefined attribute keyvals
        elif ((keyval == <int>MPI_UNIVERSE_SIZE) or
              (keyval == <int>MPI_APPNUM) or
              (keyval == <int>MPI_LASTUSEDCODE)):
            return (<int*>attrval)[0]
        # user-defined attribute keyval
        else:
            return PyLong_FromVoidPtr(attrval)

    # Error handling
    # --------------

    def Get_errhandler(self):
        """
        Get the error handler for a communicator
        """
        cdef Errhandler errhandler = Errhandler()
        CHKERR( MPI_Comm_get_errhandler(self.ob_mpi, &errhandler.ob_mpi) )
        return errhandler

    def Set_errhandler(self, Errhandler errhandler not None):
        """
        Set the error handler for a communicator
        """
        CHKERR( MPI_Comm_set_errhandler(self.ob_mpi, errhandler.ob_mpi) )

    def Call_errhandler(self, int errorcode):
        """
        Call the error handler installed on a communicator
        """
        CHKERR( MPI_Comm_call_errhandler(self.ob_mpi, errorcode) )


    def Abort(self, int errorcode=0):
        """
        Terminate MPI execution environment

        .. warning:: This is a direct call, use it with care!!!.
        """
        CHKERR( MPI_Abort(self.ob_mpi, errorcode) )

    # Naming Objects
    # --------------

    def Get_name(self):
        """
        Get the print name for this communicator
        """
        cdef char name[MPI_MAX_OBJECT_NAME+1]
        cdef int nlen = 0
        CHKERR( MPI_Comm_get_name(self.ob_mpi, name, &nlen) )
        return tompistr(name, nlen)

    def Set_name(self, name):
        """
        Set the print name for this communicator
        """
        cdef char *cname = NULL
        name = asmpistr(name, &cname, NULL)
        CHKERR( MPI_Comm_set_name(self.ob_mpi, cname) )

    property name:
        """communicator name"""
        def __get__(self):
            return self.Get_name()
        def __set__(self, value):
            self.Set_name(value)

    # Fortran Handle
    # --------------

    def py2f(self):
        """
        """
        return MPI_Comm_c2f(self.ob_mpi)

    @classmethod
    def f2py(cls, arg):
        """
        """
        cdef Comm comm = cls()
        comm.ob_mpi = MPI_Comm_f2c(arg)
        return comm

    # Python Communication
    # --------------------
    #
    def send(self, obj=None, int dest=0, int tag=0):
        """Send"""
        cdef MPI_Comm comm = self.ob_mpi
        return PyMPI_send(obj, dest, tag, comm)
    #
    def recv(self, obj=None, int source=0, int tag=0, Status status=None):
        """Receive"""
        cdef MPI_Comm comm = self.ob_mpi
        cdef MPI_Status *statusp = _arg_Status(status)
        return PyMPI_recv(obj, source, tag, comm, statusp)
    #
    def sendrecv(self,
                 sendobj=None, int dest=0,   int sendtag=0,
                 recvobj=None, int source=0, int recvtag=0,
                 Status status=None):
        """Send and Receive"""
        cdef MPI_Comm comm = self.ob_mpi
        cdef MPI_Status *statusp = _arg_Status(status)
        return PyMPI_sendrecv(sendobj, dest,   sendtag,
                              recvobj, source, recvtag,
                              comm, statusp)
    #
    def barrier(self):
        "Barrier"
        cdef MPI_Comm comm = self.ob_mpi
        return PyMPI_barrier(comm)
    #
    def bcast(self, obj=None, int root=0):
        """Broadcast"""
        cdef MPI_Comm comm = self.ob_mpi
        return PyMPI_bcast(obj, root, comm)
    #
    def gather(self, sendobj=None, recvobj=None, int root=0):
        """Gather"""
        cdef MPI_Comm comm = self.ob_mpi
        return PyMPI_gather(sendobj, recvobj, root, comm)
    #
    def scatter(self, sendobj=None, recvobj=None, int root=0):
        """Scatter"""
        cdef MPI_Comm comm = self.ob_mpi
        return PyMPI_scatter(sendobj, recvobj, root, comm)
    #
    def allgather(self, sendobj=None, recvobj=None):
        """Gather to All"""
        cdef MPI_Comm comm = self.ob_mpi
        return PyMPI_allgather(sendobj, recvobj, comm)
    #
    def alltoall(self, sendobj=None, recvobj=None):
        """All to All Scatter/Gather"""
        cdef MPI_Comm comm = self.ob_mpi
        return PyMPI_alltoall(sendobj, recvobj, comm)
    #
    def reduce(self, sendobj=None, recvobj=None, op=SUM, int root=0):
        """Reduce"""
        if op is None: op = SUM
        cdef MPI_Comm comm = self.ob_mpi
        return PyMPI_reduce(sendobj, recvobj, op, root, comm)
    #
    def allreduce(self, sendobj=None, recvobj=None, op=SUM):
        """Reduce to All"""
        if op is None: op = SUM
        cdef MPI_Comm comm = self.ob_mpi
        return PyMPI_allreduce(sendobj, recvobj, op, comm)


cdef class Intracomm(Comm):

    """
    Intracommunicator
    """

    # Communicator Constructors
    # -------------------------

    def Dup(self):
        """
        Duplicate an existing intracommunicator
        """
        cdef Intracomm comm = type(self)()
        with nogil: CHKERR( MPI_Comm_dup(self.ob_mpi, &comm.ob_mpi) )
        return comm

    def Create(self, Group group not None):
        """
        Create intracommunicator from group
        """
        cdef Intracomm comm = type(self)()
        with nogil: CHKERR( MPI_Comm_create(self.ob_mpi, group.ob_mpi, &comm.ob_mpi) )
        return comm

    def Split(self, int color=0, int key=0):
        """
        Split intracommunicator by color and key
        """
        cdef Intracomm comm = type(self)()
        with nogil: CHKERR( MPI_Comm_split(self.ob_mpi, color, key, &comm.ob_mpi) )
        return comm

    def Create_cart(self, dims, periods=None, bint reorder=False):
        """
        Create cartesian communicator
        """
        cdef int ndims = len(dims)
        cdef int *idims = NULL
        cdef tmp1 = asarray_int(dims, &idims, ndims)
        if periods is None: periods = [False] * ndims
        cdef int *iperiods = NULL
        cdef tmp2 = asarray_int(periods, &iperiods, ndims)
        #
        cdef Cartcomm comm = Cartcomm()
        with nogil: CHKERR( MPI_Cart_create(self.ob_mpi, ndims, idims, iperiods, reorder, &comm.ob_mpi) )
        return comm

    def Create_graph(self, index, edges, bint reorder=False):
        """
        Create graph communicator
        """
        cdef int nnodes = len(index)
        cdef int *iindex = NULL
        cdef tmp1 = asarray_int(index, &iindex, nnodes)
        cdef int nedges = len(edges)
        cdef int *iedges = NULL
        cdef tmp2 = asarray_int(edges, &iedges, nedges)
        # extension: 'standard' adjacency arrays
        if iindex[0]==0 and iindex[nnodes-1]==nedges:
            nnodes -= 1; iindex += 1;
        #
        cdef Graphcomm comm = Graphcomm()
        with nogil: CHKERR( MPI_Graph_create(self.ob_mpi, nnodes, iindex, iedges, reorder, &comm.ob_mpi) )
        return comm

    def Create_intercomm(self,
                         int local_leader,
                         Intracomm peer_comm not None,
                         int remote_leader,
                         int tag=0):
        """
        Create intercommunicator
        """
        cdef Intercomm comm = Intercomm()
        with nogil: CHKERR( MPI_Intercomm_create(
            self.ob_mpi, local_leader,
            peer_comm.ob_mpi, remote_leader,
            tag, &comm.ob_mpi) )
        return comm

    # Global Reduction Operations
    # ---------------------------

    # Inclusive Scan

    def Scan(self, sendbuf, recvbuf, Op op not None=SUM):
        """
        Inclusive Scan
        """
        cdef _p_msg_cco m = message_cco()
        m.for_scan(sendbuf, recvbuf, self.ob_mpi)
        with nogil: CHKERR( MPI_Scan(
            m.sbuf, m.rbuf, m.rcount, m.rtype,
            op.ob_mpi, self.ob_mpi) )

    # Exclusive Scan

    def Exscan(self, sendbuf, recvbuf, Op op not None=SUM):
        """
        Exclusive Scan
        """
        cdef _p_msg_cco m = message_cco()
        m.for_exscan(sendbuf, recvbuf, self.ob_mpi)
        with nogil: CHKERR( MPI_Exscan(
            m.sbuf, m.rbuf, m.rcount, m.rtype,
            op.ob_mpi, self.ob_mpi) )

    # Python Communication
    #
    def scan(self, sendobj=None, recvobj=None, op=SUM):
        """Inclusive Scan"""
        if op is None: op = SUM
        cdef MPI_Comm comm = self.ob_mpi
        return PyMPI_scan(sendobj, recvobj, op, comm)
    #
    def exscan(self, sendobj=None, recvobj=None, op=SUM):
        """Exclusive Scan"""
        if op is None: op = SUM
        cdef MPI_Comm comm = self.ob_mpi
        return PyMPI_exscan(sendobj, recvobj, op, comm)


    # Establishing Communication
    # --------------------------

    # Starting Processes

    def Spawn(self, command, args, int maxprocs=1,
              Info info=INFO_NULL, int root=0, errcodes=None):
        """
        Spawn instances of a single MPI application
        """
        cdef char *cmd = NULL
        cdef char **argv = MPI_ARGV_NULL
        cdef MPI_Info cinfo = _arg_Info(info)
        cdef int *ierrcodes = MPI_ERRCODES_IGNORE
        #
        cdef int rank = MPI_UNDEFINED
        CHKERR( MPI_Comm_rank(self.ob_mpi, &rank) )
        cdef object tmp1 = None, tmp2 = None
        if root == rank:
            command = asmpistr(command, &cmd, NULL)
            if args is not None:
                tmp1 = asarray_argv(args, &argv)
            if errcodes is not None:
                tmp2 = newarray_int(maxprocs, &ierrcodes)
        #
        cdef Intercomm comm = Intercomm()
        with nogil: CHKERR( MPI_Comm_spawn(
            cmd, argv, maxprocs, cinfo, root,
            self.ob_mpi, &comm.ob_mpi,
            ierrcodes) )
        #
        cdef int i = 0
        if root == rank and (errcodes is not None):
            errcodes[:] = [ierrcodes[i] for i from 0<=i<maxprocs]
        #
        return comm

    # Server Routines

    def Accept(self, port_name, Info info=INFO_NULL, int root=0):
        """
        Accept a request to form a new intercommunicator
        """
        cdef char *cportname = NULL
        port_name = asmpistr(port_name, &cportname, NULL)
        cdef MPI_Info cinfo = _arg_Info(info)
        cdef Intercomm comm = Intercomm()
        with nogil: CHKERR( MPI_Comm_accept(
            cportname, cinfo, root,
            self.ob_mpi, &comm.ob_mpi) )
        return comm

    # Client Routines

    def Connect(self, port_name, Info info=INFO_NULL, int root=0):
        """
        Make a request to form a new intercommunicator
        """
        cdef char *cportname = NULL
        port_name = asmpistr(port_name, &cportname, NULL)
        cdef MPI_Info cinfo = _arg_Info(info)
        cdef Intercomm comm = Intercomm()
        with nogil: CHKERR( MPI_Comm_connect(
            cportname, cinfo, root,
            self.ob_mpi, &comm.ob_mpi) )
        return comm


cdef class Cartcomm(Intracomm):

    """
    Cartesian topology intracommunicator
    """

    # Communicator Constructors
    # -------------------------

    def Dup(self):
        """
        Duplicate an existing communicator
        """
        cdef Intracomm comm = type(self)()
        with nogil: CHKERR( MPI_Comm_dup(self.ob_mpi, &comm.ob_mpi) )
        return comm

    # Cartesian Inquiry Functions
    # ---------------------------

    def Get_dim(self):
        """
        Return number of dimensions
        """
        cdef int dim = 0
        CHKERR( MPI_Cartdim_get(self.ob_mpi, &dim) )
        return dim

    property dim:
        """number of dimensions"""
        def __get__(self):
            return self.Get_dim()

    property ndim:
        """number of dimensions"""
        def __get__(self):
            return self.Get_dim()

    def Get_topo(self):
        """
        Return information on the cartesian topology
        """
        cdef int ndim = 0
        CHKERR( MPI_Cartdim_get(self.ob_mpi, &ndim) )
        cdef int *idims = NULL
        cdef tmp1 = newarray_int(ndim, &idims)
        cdef int *iperiods = NULL
        cdef tmp2 = newarray_int(ndim, &iperiods)
        cdef int *icoords = NULL
        cdef tmp3 = newarray_int(ndim, &icoords)
        CHKERR( MPI_Cart_get(self.ob_mpi, ndim, idims, iperiods, icoords) )
        cdef int i = 0
        dims    = [idims[i]    for i from 0 <= i < ndim]
        periods = [iperiods[i] for i from 0 <= i < ndim]
        coords  = [icoords[i]  for i from 0 <= i < ndim]
        return (dims, periods, coords)

    property topo:
        """topology information"""
        def __get__(self):
            return self.Get_topo()

    property dims:
        """dimensions"""
        def __get__(self):
            return self.Get_topo()[0]

    property periods:
        """periodicity"""
        def __get__(self):
            return self.Get_topo()[1]

    property coords:
        """coordinates"""
        def __get__(self):
            return self.Get_topo()[2]


    # Cartesian Translator Functions
    # ------------------------------

    def Get_cart_rank(self, coords):
        """
        Translate logical coordinates to ranks
        """
        cdef int ndim = 0
        CHKERR( MPI_Cartdim_get( self.ob_mpi, &ndim) )
        #
        cdef int *icoords = NULL
        cdef tmp = asarray_int(coords, &icoords, ndim)
        cdef int rank = MPI_PROC_NULL
        CHKERR( MPI_Cart_rank(self.ob_mpi, icoords, &rank) )
        return rank

    def Get_coords(self, int rank):
        """
        Translate ranks to logical coordinates
        """
        cdef int ndim = 0
        CHKERR( MPI_Cartdim_get(self.ob_mpi, &ndim) )
        cdef int *icoords = NULL
        cdef tmp = newarray_int(ndim, &icoords)
        CHKERR( MPI_Cart_coords(self.ob_mpi, rank, ndim, icoords) )
        cdef int i = 0
        coords  = [icoords[i] for i from 0 <= i < ndim]
        return coords

    # Cartesian Shift Function
    # ------------------------

    def Shift(self, int direction, int disp):
        """
        Return a tuple (source,dest) of process ranks
        for data shifting with Comm.Sendrecv()
        """
        cdef int source = MPI_PROC_NULL, dest = MPI_PROC_NULL
        CHKERR( MPI_Cart_shift(self.ob_mpi, direction, disp, &source, &dest) )
        return (source, dest)

    # Cartesian Partition Function
    # ----------------------------

    def Sub(self, remain_dims):
        """
        Return cartesian communicators
        that form lower-dimensional subgrids
        """
        cdef int ndim = 0
        CHKERR( MPI_Cartdim_get(self.ob_mpi, &ndim) )
        cdef int *iremdims = NULL
        cdef tmp = asarray_int(remain_dims, &iremdims, ndim)
        cdef Cartcomm comm = Cartcomm()
        with nogil: CHKERR( MPI_Cart_sub(self.ob_mpi, iremdims, &comm.ob_mpi) )
        return comm


    # Cartesian Low-Level Functions
    # -----------------------------

    def Map(self, dims, periods=None):
        """
        Return an optimal placement for the
        calling process on the physical machine
        """
        cdef int ndims = len(dims)
        cdef int *idims = NULL
        cdef tmp1 = asarray_int(dims, &idims, ndims)
        if periods is None: periods = [False] * ndims
        cdef int *iperiods = NULL
        cdef tmp2 = asarray_int(periods, &iperiods, ndims)
        cdef int rank = MPI_PROC_NULL
        CHKERR( MPI_Cart_map(self.ob_mpi, ndims, idims, iperiods, &rank) )
        return rank

# Cartesian Convenience Function

def Compute_dims(int nnodes, dims):
    """
    Return a balanced distribution of
    processes per coordinate direction
    """
    cdef int ndims, *idims = NULL
    try: ndims = len(dims)
    except: ndims = dims; dims = [0] * ndims
    cdef tmp = asarray_int(dims, &idims, ndims)
    CHKERR( MPI_Dims_create(nnodes, ndims, idims) )
    cdef int i = 0
    return [idims[i] for i from 0 <= i < ndims]


cdef class Graphcomm(Intracomm):

    """
    Graph topology intracommunicator
    """

    # Communicator Constructors
    # -------------------------

    def Dup(self):
        """
        Duplicate an existing communicator
        """
        cdef Intracomm comm = type(self)()
        with nogil: CHKERR( MPI_Comm_dup(
            self.ob_mpi, &comm.ob_mpi) )
        return comm

    # Graph Inquiry Functions
    # -----------------------

    def Get_dims(self):
        """
        Return the number of nodes and edges
        """
        cdef int nnodes = 0, nedges = 0
        CHKERR( MPI_Graphdims_get(self.ob_mpi, &nnodes, &nedges) )
        return (nnodes, nedges)

    property dims:
        """number of nodes and edges"""
        def __get__(self):
            return self.Get_topo()

    property nnodes:
        """number of nodes"""
        def __get__(self):
            return self.Get_topo()[0]

    property nedges:
        """number of edges"""
        def __get__(self):
            return self.Get_topo()[1]

    def Get_topo(self):
        """
        Return index and edges
        """
        cdef int nindex = 0, nedges = 0
        CHKERR( MPI_Graphdims_get( self.ob_mpi, &nindex, &nedges) )
        cdef int *iindex = NULL
        cdef tmp1 = newarray_int(nindex, &iindex)
        cdef int *iedges = NULL
        cdef tmp2 = newarray_int(nedges, &iedges)
        CHKERR( MPI_Graph_get(self.ob_mpi, nindex, nedges, iindex, iedges) )
        cdef int i = 0
        index = [iindex[i] for i from 0 <= i < nindex]
        edges = [iedges[i] for i from 0 <= i < nedges]
        return (index, edges)

    property topo:
        """topology information"""
        def __get__(self):
            return self.Get_topo()

    property index:
        """index"""
        def __get__(self):
            return self.Get_topo()[0]

    property edges:
        """edges"""
        def __get__(self):
            return self.Get_topo()

    # Graph Information Functions
    # ---------------------------

    def Get_neighbors_count(self, int rank):
        """
        Return number of neighbors of a process
        """
        cdef int nneighbors = 0
        CHKERR( MPI_Graph_neighbors_count(self.ob_mpi, rank, &nneighbors) )
        return nneighbors

    property nneighbors:
        """number of neighbors"""
        def __get__(self):
            cdef int rank = self.Get_rank()
            return self.Get_neighbors_count(rank)

    def Get_neighbors(self, int rank):
        """
        Return list of neighbors of a process
        """
        cdef int nneighbors = 0
        with nogil: CHKERR( MPI_Graph_neighbors_count(
            self.ob_mpi, rank, &nneighbors) )
        cdef int *ineighbors = NULL
        cdef tmp = newarray_int(nneighbors, &ineighbors)
        CHKERR( MPI_Graph_neighbors(self.ob_mpi, rank, nneighbors, ineighbors) )
        cdef int i = 0
        neighbors = [ineighbors[i] for i from 0 <= i < nneighbors]
        return neighbors

    property neighbors:
        """neighbors"""
        def __get__(self):
            cdef int rank = self.Get_rank()
            return self.Get_neighbors(rank)

    # Graph Low-Level Functions
    # -------------------------

    def Map(self, index, edges):
        """
        Return an optimal placement for the
        calling process on the physical machine
        """
        cdef int nnodes = len(index)
        cdef int *iindex = NULL
        cdef tmp1 = asarray_int(index, &iindex, nnodes)
        cdef int nedges = len(edges)
        cdef int *iedges = NULL
        cdef tmp2 = asarray_int(edges, &iedges, nedges)
        # extension: accept more 'standard' adjacency arrays
        if iindex[0]==0 and iindex[nnodes-1]==nedges:
            nnodes -= 1; iindex += 1;
        cdef int rank = MPI_PROC_NULL
        CHKERR( MPI_Graph_map(self.ob_mpi, nnodes, iindex, iedges, &rank) )
        return rank


cdef class Intercomm(Comm):

    """
    Intercommunicator
    """

    # Intercommunicator Accessors
    # ---------------------------

    def Get_remote_group(self):
        """
        Access the remote group associated
        with the inter-communicator
        """
        cdef Group group = Group()
        CHKERR( MPI_Comm_remote_group(self.ob_mpi, &group.ob_mpi) )
        return group

    property remote_group:
        """remote group"""
        def __get__(self):
            return self.Get_remote_group()

    def Get_remote_size(self):
        """
        Intercommunicator remote size
        """
        cdef int size = -1
        CHKERR( MPI_Comm_remote_size(self.ob_mpi, &size) )
        return size

    property remote_size:
        """number of remote processes"""
        def __get__(self):
            return self.Get_remote_size()

    # Communicator Constructors
    # -------------------------

    def Dup(self):
        """
        Duplicate an existing intercommunicator
        """
        cdef Intercomm comm = type(self)()
        with nogil: CHKERR( MPI_Comm_dup(self.ob_mpi, &comm.ob_mpi) )
        return comm

    def Create(self, Group group not None):
        """
        Create intercommunicator from group
        """
        cdef Intercomm comm = type(self)()
        with nogil: CHKERR( MPI_Comm_create(self.ob_mpi, group.ob_mpi, &comm.ob_mpi) )
        return comm

    def Split(self, int color=0, int key=0):
        """
        Split intercommunicator by color and key
        """
        cdef Intercomm comm = type(self)()
        with nogil: CHKERR( MPI_Comm_split(self.ob_mpi, color, key, &comm.ob_mpi) )
        return comm

    def Merge(self, bint high=False):
        """
        Merge intercommunicator
        """
        cdef Intracomm comm = Intracomm()
        with nogil: CHKERR( MPI_Intercomm_merge(self.ob_mpi, high, &comm.ob_mpi) )
        return comm



cdef Comm      __COMM_NULL__   = _new_Comm      ( MPI_COMM_NULL  )
cdef Intracomm __COMM_SELF__   = _new_Intracomm ( MPI_COMM_SELF  )
cdef Intracomm __COMM_WORLD__  = _new_Intracomm ( MPI_COMM_WORLD )
cdef Intercomm __COMM_PARENT__ = _new_Intercomm ( MPI_COMM_NULL  )


# Predefined communicators
# ------------------------

COMM_NULL =  __COMM_NULL__   #: Null communicator handle
COMM_SELF  = __COMM_SELF__   #: Self communicator handle
COMM_WORLD = __COMM_WORLD__  #: World communicator handle


# Buffer Allocation and Usage
# ---------------------------

BSEND_OVERHEAD = MPI_BSEND_OVERHEAD
#: Upper bound of memory overhead for sending in buffered mode

def Attach_buffer(memory):
    """
    Attach a user-provided buffer for
    sending in buffered mode
    """
    cdef void *base = NULL
    cdef MPI_Aint size = 0
    asmemory(memory, &base, &size)
    with nogil: CHKERR( MPI_Buffer_attach(base, <int>size) )

def Detach_buffer():
    """
    Remove an existing attached buffer
    """
    cdef void *base = NULL
    cdef int size = 0
    with nogil: CHKERR( MPI_Buffer_detach(&base, &size) )
    return tomemory(base, <MPI_Aint>size)


# --------------------------------------------------------------------
# [5] Process Creation and Management
# --------------------------------------------------------------------

# [5.4.2] Server Routines
# -----------------------

def Open_port(Info info=INFO_NULL):
    """
    Return an address that can be used to establish
    connections between groups of MPI processes
    """
    cdef MPI_Info cinfo = _arg_Info(info)
    cdef char cportname[MPI_MAX_PORT_NAME+1]
    with nogil: CHKERR( MPI_Open_port(cinfo, cportname) )
    return mpistr(cportname)

def Close_port(port_name, Info info=INFO_NULL):
    """
    Close a port
    """
    cdef char *cportname = NULL
    port_name = asmpistr(port_name, &cportname, NULL)
    cdef MPI_Info cinfo = _arg_Info(info)
    with nogil: CHKERR( MPI_Close_port(cportname) )

# [5.4.4] Name Publishing
# -----------------------

def Publish_name(service_name, Info info, port_name):
    """
    Publish a service name
    """
    cdef char *csrvcname = NULL
    service_name = asmpistr(service_name, &csrvcname, NULL)
    cdef char *cportname = NULL
    port_name = asmpistr(port_name, &cportname, NULL)
    cdef MPI_Info cinfo = _arg_Info(info)
    with nogil: CHKERR( MPI_Publish_name(csrvcname, cinfo, cportname) )

def Unpublish_name(service_name, Info info, port_name):
    """
    Unpublish a service name
    """
    cdef char *csrvcname = NULL
    service_name = asmpistr(service_name, &csrvcname, NULL)
    cdef char *cportname = NULL
    port_name = asmpistr(port_name, &cportname, NULL)
    cdef MPI_Info cinfo = _arg_Info(info)
    with nogil: CHKERR( MPI_Unpublish_name(csrvcname, cinfo, cportname) )

def Lookup_name(service_name, Info info=INFO_NULL):
    """
    Lookup a port name given a service name
    """
    cdef char *csrvcname = NULL
    service_name = asmpistr(service_name, &csrvcname, NULL)
    cdef MPI_Info cinfo = _arg_Info(info)
    cdef char cportname[MPI_MAX_PORT_NAME+1]
    with nogil: CHKERR( MPI_Lookup_name(csrvcname, cinfo, cportname) )
    return mpistr(cportname)
