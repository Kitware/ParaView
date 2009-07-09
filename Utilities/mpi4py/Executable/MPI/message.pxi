#---------------------------------------------------------------------

cdef extern from "Python.h":
    int is_int   "PyInt_Check"        (object)
    int is_list  "PyList_CheckExact"  (object)
    int is_tuple "PyTuple_CheckExact" (object)

cdef object __BOTTOM__   = <MPI_Aint>MPI_BOTTOM
cdef object __IN_PLACE__ = <MPI_Aint>MPI_IN_PLACE

cdef inline int is_BOTTOM(object msg):
    return (msg is None or msg is __BOTTOM__)

cdef inline int is_IN_PLACE(object msg):
    return (msg is None or msg is __IN_PLACE__)

cdef inline object message_simple(int readonly,
                                  object msg,
                                  int rank, int size,
                                  void **_buf,
                                  int *_count,
                                  MPI_Datatype *_dtype):
    # special case
    if rank == MPI_PROC_NULL:
        if msg is None:
            _buf[0]   = NULL
            _count[0] = 0
            _dtype[0] = MPI_BYTE # XXX explain
            return None
    # check argument containing message
    cdef Py_ssize_t n = 0
    if not is_list(msg) and not is_tuple(msg):
        raise TypeError(mpistr("message: expecting a list or tuple"))
    n = len(msg)
    if n < 2 or n > 3:
        raise ValueError(mpistr("message: expecting 2 or 3 items"))
    # unpack message list/tuple
    cdef object obuf, ocount, odtype
    if n == 2:
        obuf   = msg[0]
        ocount = None
        odtype = msg[1]
    else:
        obuf   = msg[0]
        ocount = msg[1]
        odtype = msg[2]
    # buffer pointer and length
    cdef void *bptr = NULL
    cdef MPI_Aint blen = 0
    if is_BOTTOM(obuf):
        bptr = MPI_BOTTOM; blen = 0
    elif readonly:
        asbuffer_r(obuf, &bptr, &blen)
    else:
        asbuffer_w(obuf, &bptr, &blen)
    # datatype
    cdef MPI_Datatype dtype = MPI_DATATYPE_NULL
    cdef MPI_Aint lb = 0, ex = 0
    dtype = (<Datatype?>odtype).ob_mpi
    # number of entries
    cdef int count = 0
    cdef MPI_Aint nitems = 0
    if ocount is not None: # user-provided
        count = <int> ocount
    elif blen > 0: # try to guess
        CHKERR( MPI_Type_get_extent(dtype, &lb, &ex) )
        if (blen % ex) != 0:
            raise ValueError(mpistr(
                "message: buffer length %d is not a multiple " \
                "of datatype extent %d (lb:%d, ub:%d)") %  \
                (blen, ex, lb, lb+ex))
        nitems = blen/ex
        if size <= 1:
            count = <int> nitems
        else:
            if (nitems % size) != 0:
                raise ValueError(mpistr(
                    "message: number of datatype items %d is not " \
                    "a multiple of the required number " \
                    "of blocks %d") %  (nitems, size))
            count = <int> (nitems/size)
    # return collected message data
    _buf[0]   = bptr
    _count[0] = count
    _dtype[0] = dtype
    return (obuf, ocount, odtype)


cdef inline object message_vector(int readonly,
                                  object msg,
                                  int rank, int size,
                                  void **_buf,
                                  int **_counts,
                                  int **_displs,
                                  MPI_Datatype *_dtype):
    # special case
    if rank == MPI_PROC_NULL:
        if msg is None:
            _buf[0]    = NULL
            _counts[0] = NULL
            _displs[0] = NULL
            _dtype[0]  = MPI_BYTE # XXX explain
            return None
    #
    cdef void *buf = NULL
    cdef int *counts = NULL
    cdef int *displs = NULL
    cdef MPI_Datatype dtype = MPI_DATATYPE_NULL
    cdef object obuf, ocounts, odispls, odtype
    # check argument containing message
    cdef Py_ssize_t n = 0
    if not is_list(msg) and not is_tuple(msg):
        raise TypeError(mpistr("message: expecting a list or tuple"))
    n = len(msg)
    if n < 3 or n > 4:
        raise ValueError(mpistr("message: expecting 3 or 4 items"))
    # unpack message list/tuple
    if n == 4:
        obuf    = msg[0]
        ocounts = msg[1]
        odispls = msg[2]
        odtype  = msg[3]
    else:
        obuf    = msg[0]
        ocounts = msg[1][0]
        odispls = msg[1][1]
        odtype  = msg[2]
    # buffer
    if is_BOTTOM(obuf):
        buf = MPI_BOTTOM
    elif readonly:
        asbuffer_r(obuf, &buf, NULL)
    else:
        asbuffer_w(obuf, &buf, NULL)
    # counts
    cdef int i=0, val=0
    if is_int(ocounts):
        val = <int> ocounts
        ocounts = newarray_int(size, &counts)
        for i from 0 <= i < size:
            counts[i] = val
    else:
        ocounts = asarray_int(ocounts, &counts, size)
    # displacements
    if odispls is None: # contiguous
        val = 0
        odispls = newarray_int(size, &displs)
        for i from 0 <= i < size:
            displs[i] = val; val += counts[i]
    elif is_int(odispls): # strided
        val = <int> odispls
        odispls = newarray_int(size, &displs)
        for i from 0 <= i < size:
            displs[i] = val * i
    else: # general
        odispls = asarray_int(odispls, &displs, size)
    # datatype
    dtype = (<Datatype?>odtype).ob_mpi
    # return collected message data
    _buf[0]    = buf
    _counts[0] = counts
    _displs[0] = displs
    _dtype[0]  = dtype
    return (obuf, (ocounts, odispls), odtype)

#---------------------------------------------------------------------

cdef class _p_msg_p2p:

    # raw C-side arguments
    cdef void         *buf
    cdef int          count
    cdef MPI_Datatype dtype
    # python-side argument
    cdef object _msg

    def __cinit__(self):
        self.buf   = NULL
        self.count = 0
        self.dtype = MPI_DATATYPE_NULL

    cdef for_send(self, object msg, int rank):
        self._msg = message_simple(1, msg, # readonly
                                   rank, 0,
                                   &self.buf,
                                   &self.count,
                                   &self.dtype)

    cdef for_recv(self, object msg, int rank):
        self._msg = message_simple(0, msg, # writable
                                   rank, 0,
                                   &self.buf,
                                   &self.count,
                                   &self.dtype)

cdef inline _p_msg_p2p message_p2p_send(object sendbuf, int dest):
    cdef _p_msg_p2p msg = <_p_msg_p2p>_p_msg_p2p()
    msg.for_send(sendbuf, dest)
    return msg

cdef inline _p_msg_p2p message_p2p_recv(object recvbuf, int source):
    cdef _p_msg_p2p msg = <_p_msg_p2p>_p_msg_p2p()
    msg.for_recv(recvbuf, source)
    return msg

#---------------------------------------------------------------------

cdef class _p_msg_cco:

    # raw C-side arguments
    cdef void *sbuf, *rbuf
    cdef int  scount, rcount
    cdef int *scounts, *rcounts
    cdef int *sdispls, *rdispls
    cdef MPI_Datatype stype, rtype
    # python-side arguments
    cdef object _smsg, _rmsg

    def __cinit__(self):
        self.sbuf    = self.rbuf    = NULL
        self.scount  = self.rcount  = 0
        self.scounts = self.rcounts = NULL
        self.sdispls = self.rdispls = NULL
        self.stype   = self.rtype = MPI_DATATYPE_NULL

    # Collective Communication Operations
    # -----------------------------------

    # sendbuf arguments
    cdef for_cco_send(self, int v, object amsg, int root, int size):
        if not v:
            self._smsg = message_simple(
                1, amsg, root, size,
                &self.sbuf, &self.scount, &self.stype)
        else: # vector variant
            self._smsg = message_vector(
                1, amsg, root, size,
                &self.sbuf, &self.scounts, &self.sdispls, &self.stype)

    # recvbuf arguments
    cdef for_cco_recv(self, int v, object amsg, int root, int size):
        if not v:
            self._rmsg = message_simple(
                0, amsg, root, size,
                &self.rbuf, &self.rcount, &self.rtype)
        else: # vector variant
            self._rmsg = message_vector(
                0, amsg, root, size,
                &self.rbuf, &self.rcounts, &self.rdispls, &self.rtype)

    # bcast
    cdef for_bcast(self, object msg, int root, MPI_Comm comm):
        if comm == MPI_COMM_NULL: return
        cdef int inter=0, rank=0, sending=0
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        if not inter: # intra-communication
            CHKERR( MPI_Comm_rank(comm, &rank) )
            if root == rank:
                self.for_cco_send(0, msg, root, 0)
                sending = 1
            else:
                self.for_cco_recv(0, msg, root, 0)
        else: # inter-communication
            if ((root == <int>MPI_ROOT) or
                (root == <int>MPI_PROC_NULL)):
                self.for_cco_send(0, msg, root, 0)
                sending = 1
            else:
                self.for_cco_recv(0, msg, root, 0)
        if sending:
            self.rbuf   = self.sbuf
            self.rcount = self.scount
            self.rtype  = self.stype
        else:
            self.sbuf   = self.rbuf
            self.scount = self.rcount
            self.stype  = self.rtype

    # gather/gatherv
    cdef for_gather(self, int v,
                    object smsg, object rmsg,
                    int root, MPI_Comm comm):
        if comm == MPI_COMM_NULL: return
        cdef int inter=0, size=0, rank=0, null = MPI_PROC_NULL
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        if not inter: # intra-communication
            CHKERR( MPI_Comm_size(comm, &size) )
            CHKERR( MPI_Comm_rank(comm, &rank) )
            if root == rank:
                self.for_cco_recv(v, rmsg, root, size)
                if is_IN_PLACE(smsg):
                    self.sbuf   = MPI_IN_PLACE
                    self.scount = self.rcount
                    self.stype  = self.rtype
                else:
                    self.for_cco_send(0, smsg, 0, 0)
            else:
                self.for_cco_recv(v, rmsg, null, 0)
                self.for_cco_send(0, smsg, root, 0)
        else: # inter-communication
            CHKERR( MPI_Comm_remote_size(comm, &size) )
            if ((root == <int>MPI_ROOT) or
                (root == <int>MPI_PROC_NULL)):
                self.for_cco_recv(v, rmsg, root, size)
                self.for_cco_send(0, smsg, null, 0)
            else:
                self.for_cco_recv(v, rmsg, null, 0)
                self.for_cco_send(0, smsg, root, 0)

    # scatter/scatterv
    cdef for_scatter(self, int v,
                     object smsg, object rmsg,
                     int root, MPI_Comm comm):
        if comm == MPI_COMM_NULL: return
        cdef int inter=0, size=0, rank=0, null = MPI_PROC_NULL
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        if not inter: # intra-communication
            CHKERR( MPI_Comm_size(comm, &size) )
            CHKERR( MPI_Comm_rank(comm, &rank) )
            if root == rank:
                self.for_cco_send(v, smsg, root, size)
                if is_IN_PLACE(rmsg):
                    self.rbuf   = MPI_IN_PLACE
                    self.rcount = self.scount
                    self.rtype  = self.stype
                else:
                    self.for_cco_recv(0, rmsg, root, 0)
            else:
                self.for_cco_send(v, smsg, null, 0)
                self.for_cco_recv(0, rmsg, root, 0)
        else: # inter-communication
            CHKERR( MPI_Comm_remote_size(comm, &size) )
            if ((root == <int>MPI_ROOT) or
                (root == <int>MPI_PROC_NULL)):
                self.for_cco_send(v, smsg, root, size)
                self.for_cco_recv(0, rmsg, null,  0)
            else:
                self.for_cco_send(v, smsg, null, 0)
                self.for_cco_recv(0, rmsg, root, 0)

    # allgather/allgatherv
    cdef for_allgather(self, int v,
                       object smsg, object rmsg,
                       MPI_Comm comm):
        if comm == MPI_COMM_NULL: return
        cdef int inter=0, size=0
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        if not inter: # intra-communication
            CHKERR( MPI_Comm_size(comm, &size) )
        else: # inter-communication
            CHKERR( MPI_Comm_remote_size(comm, &size) )
        #
        self.for_cco_recv(v, rmsg, 0, size)
        if not inter and is_IN_PLACE(smsg):
            self.sbuf   = MPI_IN_PLACE
            self.scount = self.rcount
            self.stype  = self.rtype
        else:
            self.for_cco_send(0, smsg, 0, 0)

    # alltoall/alltoallv
    cdef for_alltoall(self, int v,
                      object smsg, object rmsg,
                      MPI_Comm comm):
        if comm == MPI_COMM_NULL: return
        cdef int inter=0, size=0
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        if not inter: # intra-communication
            CHKERR( MPI_Comm_size(comm, &size) )
        else: # inter-communication
            CHKERR( MPI_Comm_remote_size(comm, &size) )
        #
        self.for_cco_recv(v, rmsg, 0, size)
        self.for_cco_send(v, smsg, 0, size)


    # Collective Reductions Operations
    # --------------------------------

    # sendbuf
    cdef for_cro_send(self, object amsg, int root):
        self._smsg = message_simple(1, amsg, # readonly
                                    root, 0,
                                    &self.sbuf,
                                    &self.scount,
                                    &self.stype)
    # recvbuf
    cdef for_cro_recv(self, object amsg, int root):
        self._rmsg = message_simple(0, amsg, # writable
                                    root, 0,
                                    &self.rbuf,
                                    &self.rcount,
                                    &self.rtype)

    cdef for_reduce(self, object smsg, object rmsg,
                    int root, MPI_Comm comm):
        if comm == MPI_COMM_NULL: return
        cdef int inter=0, rank=0
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        if not inter: # intra-communication
            self.for_cro_recv(rmsg, root)
            CHKERR( MPI_Comm_rank(comm, &rank) )
            if root == rank and is_IN_PLACE(smsg):
                self.sbuf   = MPI_IN_PLACE
                self.scount = self.rcount
                self.stype  = self.rtype
            else:
                self.for_cro_send(smsg, root)
        else: # inter-communication
            if ((root == <int>MPI_ROOT) or
                (root == <int>MPI_PROC_NULL)):
                self.for_cro_recv(rmsg, root)
                self.scount = self.rcount
                self.stype  = self.rtype
            else:
                self.for_cro_send(smsg, root)
                self.rcount = self.scount
                self.rtype  = self.stype

    cdef for_allreduce(self, object smsg, object rmsg,
                       MPI_Comm comm):
        if comm == MPI_COMM_NULL: return
        cdef int inter=0
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        #
        self.for_cro_recv(rmsg, 0)
        if not inter and is_IN_PLACE(smsg):
            self.sbuf   = MPI_IN_PLACE
            self.scount = self.rcount
            self.stype  = self.rtype
        else:
            self.for_cro_send(smsg, 0)
            assert self.scount == self.rcount
            assert self.stype  == self.rtype

    cdef for_scan(self, object smsg, object rmsg,
                  MPI_Comm comm):
        self.for_cro_recv(rmsg, 0)
        if is_IN_PLACE(smsg):
            self.sbuf   = MPI_IN_PLACE
            self.scount = self.rcount
            self.stype  = self.rtype
        else:
            self.for_cro_send(smsg, 0)
            assert self.scount == self.rcount
            assert self.stype  == self.rtype

    cdef for_exscan(self, object smsg, object rmsg,
                    MPI_Comm comm):
        self.for_cro_recv(rmsg, 0)
        self.for_cro_send(smsg, 0)
        assert self.scount == self.rcount
        assert self.stype  == self.rtype


cdef inline _p_msg_cco message_cco():
    cdef _p_msg_cco msg = <_p_msg_cco>_p_msg_cco()
    return msg

#---------------------------------------------------------------------

cdef class _p_msg_rma:

    # raw origin arguments
    cdef void*        oaddr
    cdef int          ocount
    cdef MPI_Datatype otype
    # raw target arguments
    cdef MPI_Aint     tdisp
    cdef int          tcount
    cdef MPI_Datatype ttype
    # python-side arguments
    cdef object _origin
    cdef object _target

    def __cinit__(self):
        self.oaddr  = NULL
        self.ocount = 0
        self.otype  = MPI_DATATYPE_NULL
        self.tdisp  = 0
        self.tcount = 0
        self.ttype  = MPI_DATATYPE_NULL

    cdef for_rma(self, int readonly, object origin,
                 int rank, object target):
        # check arguments
        cdef Py_ssize_t no = 0, nt = 0
        if origin is not None:
            if not is_list(origin) and not is_tuple(origin):
                raise ValueError(mpistr("origin: expecting a list or tuple"))
            no = len(origin)
            if no < 2 or no > 3:
                raise ValueError(mpistr("origin: expecting 2 or 3 items"))
        if target is not None:
            if not is_list(target) and not is_tuple(target):
                raise ValueError(mpistr("target: expecting a list or tuple"))
            nt = len(target)
            if nt > 3:
                raise ValueError(mpistr("target: expecting at most 3 items"))
        # ORIGIN
        cdef void *oaddr = NULL
        cdef int ocount = 0
        cdef MPI_Datatype otype = MPI_DATATYPE_NULL
        origin = message_simple(readonly, origin, rank, 0,
                                &oaddr, &ocount, &otype)
        # TARGET
        cdef MPI_Aint     tdisp  = 0
        cdef int          tcount = ocount
        cdef MPI_Datatype ttype  = otype
        if nt > 0:
            tdisp  = <MPI_Aint>target[0]
        if nt > 1:
            tcount = <int>target[1]
        if nt > 2:
            ttype  = (<Datatype?>target[2]).ob_mpi
        # save collected data
        self.oaddr  = oaddr
        self.ocount = ocount
        self.otype  = otype
        self.tdisp  = tdisp
        self.tcount = tcount
        self.ttype  = ttype
        self._origin = origin
        self._target = target

    cdef for_put(self, object origin, int rank, object target):
        self.for_rma(1, origin, rank, target)

    cdef for_get(self, object origin, int rank, object target):
        self.for_rma(0, origin, rank, target)

    cdef for_acc(self, object origin, int rank, object target):
        self.for_rma(1, origin, rank, target)

cdef inline _p_msg_rma message_rma_put(object origin, int rank, object target):
    cdef _p_msg_rma msg = <_p_msg_rma>_p_msg_rma()
    msg.for_put(origin, rank, target)
    return msg

cdef inline _p_msg_rma message_rma_get(object origin, int rank, object target):
    cdef _p_msg_rma msg = <_p_msg_rma>_p_msg_rma()
    msg.for_get(origin, rank, target)
    return msg

cdef inline _p_msg_rma message_rma_acc(object origin, int rank, object target):
    cdef _p_msg_rma msg = <_p_msg_rma>_p_msg_rma()
    msg.for_acc(origin, rank, target)
    return msg

#---------------------------------------------------------------------

cdef class _p_msg_io:

    # raw C-side data
    cdef void         *buf
    cdef int          count
    cdef MPI_Datatype dtype
    # python-side data
    cdef object _msg

    def __cinit__(self):
        self.buf   = NULL
        self.count = 0
        self.dtype = MPI_DATATYPE_NULL

    cdef for_read(self, msg):
        self._msg = message_simple(0, msg, # writable
                                   0, 0,
                                   &self.buf,
                                   &self.count,
                                   &self.dtype)

    cdef for_write(self, msg):
        self._msg = message_simple(1, msg,# readonly
                                   0, 0,
                                   &self.buf,
                                   &self.count,
                                   &self.dtype)

cdef inline _p_msg_io message_io_read(object buf):
    cdef _p_msg_io msg = <_p_msg_io>_p_msg_io()
    msg.for_read(buf)
    return msg

cdef inline _p_msg_io message_io_write(object buf):
    cdef _p_msg_io msg = <_p_msg_io>_p_msg_io()
    msg.for_write(buf)
    return msg

#---------------------------------------------------------------------
