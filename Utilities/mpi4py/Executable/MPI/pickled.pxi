# --------------------------------------------------------------------

cdef extern from *:
    char*      PyMPIBytes_AsString(object) except NULL
    Py_ssize_t PyMPIBytes_Size(object) except -1
    object     PyMPIBytes_FromStringAndSize(char*,Py_ssize_t)

# --------------------------------------------------------------------

cdef object _Pickle_dumps = None
cdef object _Pickle_loads = None
cdef object _Pickle_PROTOCOL = -1

try:
    from cPickle import dumps as _Pickle_dumps
    from cPickle import loads as _Pickle_loads
    from cPickle import HIGHEST_PROTOCOL as _Pickle_PROTOCOL
except ImportError:
    from pickle import dumps as _Pickle_dumps
    from pickle import loads as _Pickle_loads
    from pickle import HIGHEST_PROTOCOL as _Pickle_PROTOCOL

cdef inline object PyMPI_Dump(object obj):
    return _Pickle_dumps(obj, _Pickle_PROTOCOL)

cdef inline object PyMPI_Load(object obj):
    return _Pickle_loads(obj)

# --------------------------------------------------------------------

cdef inline object _py_reduce(object op, object seq):
    if seq is None: return None
    cdef int i=0, n=len(seq)
    if op is __MAXLOC__ or op is __MINLOC__:
        seq = list(zip(seq, range(n)))
    res = seq[0]
    for i from 1 <= i < n:
        res = op(res, seq[i])
    return res


cdef inline object _py_scan(object op, object seq):
    if seq is None: return None
    cdef int i=0, n=len(seq)
    if op is MAXLOC or op is MINLOC:
        seq = list(zip(seq, range(n)))
    for i from 1 <= i < n:
        seq[i] = op(seq[i-1], seq[i])
    return seq


cdef inline object _py_exscan(object op, object seq):
    if seq is None: return None
    seq = _py_scan(op, seq)
    seq.pop(-1)
    seq.insert(0, None)
    return seq

# --------------------------------------------------------------------

cdef class _p_Pickler:

    cdef object dump(self, object obj, void **p, int *n):
        if obj is None:
            p[0] = NULL
            n[0] = 0
        else:
            obj  = PyMPI_Dump(obj)
            p[0] = PyMPIBytes_AsString(obj)
            n[0] = PyMPIBytes_Size(obj)
        return obj

    cdef object alloc(self, void **p, int n):
        if n == 0:
            obj  = None
            p[0] = NULL
        else:
            obj  = PyMPIBytes_FromStringAndSize(NULL, n)
            p[0] = PyMPIBytes_AsString(obj)
        return obj

    cdef object load(self, obj):
        if obj is None: return None
        return PyMPI_Load(obj)

    cdef object dumpv(self, object obj, void **p,
                      int n, int cnt[], int dsp[]):
        cdef int i=0, d=0, c=0
        cdef Py_ssize_t m = 0
        if obj is None:
            p[0] = NULL
            for i from 0 <= i < n:
                cnt[i] = 0
                dsp[i] = 0
        else:
            obj = list(obj)
            m = len(obj)
            if m != n: raise ValueError(mpistr(
                "expecting %d items, got %d") % (n, m))
            for i from 0 <= i < n:
                obj[i] = self.dump(obj[i], p, &c)
                if c == 0: obj[i] = b''
                cnt[i] = c
                dsp[i] = d
                d += c
            obj = b''.join(obj)
            p[0] = PyMPIBytes_AsString(obj)
        return obj

    cdef object allocv(self, void **p,
                       int n, int cnt[], int dsp[]):
        cdef int i=0, d=0
        for i from 0 <= i < n:
            dsp[i] = d
            d += cnt[i]
        return self.alloc(p, d)

    cdef object loadv(self, object obj,
                      int n, int cnt[], int dsp[]):
        cdef int i=0, d=0, c=0
        cdef object items = [None] * n
        if obj is None: return items
        for i from 0 <= i < n:
            c = cnt[i]
            d = dsp[i]
            if c == 0: continue
            items[i] = self.load(obj[d:d+c])
        return items


cdef _p_Pickler PyMPI_PICKLER = _p_Pickler()

cdef inline _p_Pickler PyMPI_pickler():
    return PyMPI_PICKLER

# --------------------------------------------------------------------

cdef object PyMPI_send(object obj, int dest, int tag,
                       MPI_Comm comm):
    cdef _p_Pickler pickler = PyMPI_pickler()
    #
    cdef void *sbuf = NULL
    cdef int scount = 0
    cdef MPI_Datatype stype = MPI_BYTE
    #
    cdef int dosend = (dest != MPI_PROC_NULL)
    #
    cdef object smsg = None
    if dosend: smsg = pickler.dump(obj, &sbuf, &scount)
    with nogil: CHKERR( MPI_Send(sbuf, scount, stype,
                                 dest, tag, comm) )
    return None


cdef object PyMPI_recv(object obj, int source, int tag,
                       MPI_Comm comm, MPI_Status *status):
    cdef _p_Pickler pickler = PyMPI_pickler()
    #
    cdef void *rbuf = NULL
    cdef int rcount = 0
    cdef MPI_Datatype rtype = MPI_BYTE
    #
    cdef int dorecv = (source != MPI_PROC_NULL)
    #
    cdef MPI_Status rsts
    with nogil: CHKERR( MPI_Probe(source, tag, comm, &rsts) )
    with nogil: CHKERR( MPI_Get_count(&rsts, rtype, &rcount) )
    source = rsts.MPI_SOURCE
    tag = rsts.MPI_TAG
    #
    cdef object rmsg = None
    if dorecv: rmsg = pickler.alloc(&rbuf, rcount)
    with nogil: CHKERR( MPI_Recv(rbuf, rcount, rtype,
                                 source, tag, comm, status) )
    if dorecv: rmsg = pickler.load(rmsg)
    return rmsg


cdef object PyMPI_sendrecv(object sobj, int dest,   int sendtag,
                           object robj, int source, int recvtag,
                           MPI_Comm comm, MPI_Status *status):
    cdef _p_Pickler pickler = PyMPI_pickler()
    #
    cdef void *sbuf = NULL
    cdef int scount = 0
    cdef MPI_Datatype stype = MPI_BYTE
    cdef void *rbuf = NULL
    cdef int rcount = 0
    cdef MPI_Datatype rtype = MPI_BYTE
    #
    cdef int dosend = (dest   != MPI_PROC_NULL)
    cdef int dorecv = (source != MPI_PROC_NULL)
    #
    cdef object smsg = None
    if dosend: smsg = pickler.dump(sobj, &sbuf, &scount)
    cdef MPI_Request sreq = MPI_REQUEST_NULL
    with nogil: CHKERR( MPI_Isend(sbuf, scount, stype,
                                  dest, sendtag, comm, &sreq) )
    #
    cdef MPI_Status rsts
    with nogil: CHKERR( MPI_Probe(source, recvtag, comm, &rsts) )
    with nogil: CHKERR( MPI_Get_count(&rsts, rtype, &rcount) )
    source  = rsts.MPI_SOURCE
    recvtag = rsts.MPI_TAG
    #
    cdef object rmsg = None
    if dorecv: rmsg = pickler.alloc(&rbuf, rcount)
    with nogil: CHKERR( MPI_Recv(rbuf, rcount, rtype,
                                 source, recvtag, comm, status) )
    #
    with nogil: CHKERR( MPI_Wait(&sreq, MPI_STATUS_IGNORE) )
    if dorecv: rmsg = pickler.load(rmsg)
    return rmsg

# --------------------------------------------------------------------

cdef object PyMPI_barrier(MPI_Comm comm):
    with nogil: CHKERR( MPI_Barrier(comm) )
    return None


cdef object PyMPI_bcast(object obj,
                        int root, MPI_Comm comm):
    cdef _p_Pickler pickler = PyMPI_pickler()
    #
    cdef void *buf = NULL
    cdef int count = 0
    cdef MPI_Datatype dtype = MPI_BYTE
    #
    cdef int dosend=0, dorecv=0
    cdef int inter=0, rank=0
    CHKERR( MPI_Comm_test_inter(comm, &inter) )
    if inter:
        if root == <int>MPI_PROC_NULL:
            dosend=0; dorecv=0;
        elif root == <int>MPI_ROOT:
            dosend=1; dorecv=0;
        else:
            dosend=0; dorecv=1;
    else:
        CHKERR( MPI_Comm_rank(comm, &rank) )
        if root == rank:
            dosend=1; dorecv=1;
        else:
            dosend=0; dorecv=1;
    #
    cdef object smsg = None
    if dosend: smsg = pickler.dump(obj, &buf, &count)
    with nogil: CHKERR( MPI_Bcast(&count, 1, MPI_INT,
                                  root, comm) )
    cdef object rmsg = None
    if dorecv and dosend: rmsg = smsg
    elif dorecv: rmsg = pickler.alloc(&buf, count)
    with nogil: CHKERR( MPI_Bcast(buf, count, MPI_BYTE,
                                  root, comm) )
    if dorecv: rmsg = pickler.load(rmsg)
    return rmsg


cdef object PyMPI_gather(object sendobj, object recvobj,
                         int root, MPI_Comm comm):
    cdef _p_Pickler pickler = PyMPI_pickler()
    #
    cdef void *sbuf = NULL
    cdef int scount = 0
    cdef MPI_Datatype stype = MPI_BYTE
    cdef void *rbuf = NULL
    cdef int *rcounts = NULL
    cdef int *rdispls = NULL
    cdef MPI_Datatype rtype = MPI_BYTE
    #
    cdef int dosend=0, dorecv=0
    cdef int inter=0, size=0, rank=0
    CHKERR( MPI_Comm_test_inter(comm, &inter) )
    if inter:
        CHKERR( MPI_Comm_remote_size(comm, &size) )
        if root == <int>MPI_PROC_NULL:
            dosend=0; dorecv=0;
        elif root == <int>MPI_ROOT:
            dosend=0; dorecv=1;
        else:
            dosend=1; dorecv=0;
    else:
        CHKERR( MPI_Comm_size(comm, &size) )
        CHKERR( MPI_Comm_rank(comm, &rank) )
        if root == rank:
            dosend=1; dorecv=1;
        else:
            dosend=1; dorecv=0;
    #
    cdef object tmp1=None, tmp2=None
    if dorecv: tmp1 = newarray_int(size, &rcounts)
    if dorecv: tmp2 = newarray_int(size, &rdispls)
    #
    cdef object smsg = None
    if dosend: smsg = pickler.dump(sendobj, &sbuf, &scount)
    with nogil: CHKERR( MPI_Gather(&scount, 1, MPI_INT,
                                   rcounts, 1, MPI_INT,
                                   root, comm) )
    cdef object rmsg = None
    if dorecv: rmsg = pickler.allocv(&rbuf, size, rcounts, rdispls)
    with nogil: CHKERR( MPI_Gatherv(sbuf, scount,           stype,
                                    rbuf, rcounts, rdispls, rtype,
                                    root, comm) )
    if dorecv: rmsg = pickler.loadv(rmsg, size, rcounts, rdispls)
    return rmsg


cdef object PyMPI_scatter(object sendobj, object recvobj,
                          int root, MPI_Comm comm):
    cdef _p_Pickler pickler = PyMPI_pickler()
    #
    cdef void *sbuf = NULL
    cdef int *scounts = NULL
    cdef int *sdispls = NULL
    cdef MPI_Datatype stype = MPI_BYTE
    cdef void *rbuf = NULL
    cdef int rcount = 0
    cdef MPI_Datatype rtype = MPI_BYTE
    #
    cdef int dosend=0, dorecv=0
    cdef int inter=0, size=0, rank=0
    CHKERR( MPI_Comm_test_inter(comm, &inter) )
    if inter:
        CHKERR( MPI_Comm_remote_size(comm, &size) )
        if root == <int>MPI_PROC_NULL:
            dosend=1; dorecv=0;
        elif root == <int>MPI_ROOT:
            dosend=1; dorecv=0;
        else:
            dosend=0; dorecv=1;
    else:
        CHKERR( MPI_Comm_size(comm, &size) )
        CHKERR( MPI_Comm_rank(comm, &rank) )
        if root == rank:
            dosend=1; dorecv=1;
        else:
            dosend=0; dorecv=1;
    #
    cdef object tmp1=None, tmp2=None
    if dosend: tmp1 = newarray_int(size, &scounts)
    if dosend: tmp2 = newarray_int(size, &sdispls)
    #
    cdef object smsg = None
    if dosend: smsg = pickler.dumpv(sendobj, &sbuf, size, scounts, sdispls)
    with nogil: CHKERR( MPI_Scatter(scounts, 1, MPI_INT,
                                    &rcount, 1, MPI_INT,
                                    root, comm) )
    cdef object rmsg = None
    if dorecv: rmsg = pickler.alloc(&rbuf, rcount)
    with nogil: CHKERR( MPI_Scatterv(sbuf, scounts, sdispls, stype,
                                     rbuf, rcount,           rtype,
                                     root, comm) )
    if dorecv: rmsg = pickler.load(rmsg)
    return rmsg


cdef object PyMPI_allgather(object sendobj, object recvobj,
                            MPI_Comm comm):
    cdef _p_Pickler pickler = PyMPI_pickler()
    #
    cdef void *sbuf = NULL
    cdef int scount = 0
    cdef MPI_Datatype stype = MPI_BYTE
    cdef void *rbuf = NULL
    cdef int *rcounts = NULL
    cdef int *rdispls = NULL
    cdef MPI_Datatype rtype = MPI_BYTE
    #
    cdef int inter=0, size=0
    CHKERR( MPI_Comm_test_inter(comm, &inter) )
    if inter:
        CHKERR( MPI_Comm_remote_size(comm, &size) )
    else:
        CHKERR( MPI_Comm_size(comm, &size) )
    #
    cdef object tmp1 = newarray_int(size, &rcounts)
    cdef object tmp2 = newarray_int(size, &rdispls)
    #
    cdef object smsg = pickler.dump(sendobj, &sbuf, &scount)
    with nogil: CHKERR( MPI_Allgather(&scount, 1, MPI_INT,
                                      rcounts, 1, MPI_INT,
                                      comm) )
    cdef object rmsg = pickler.allocv(&rbuf, size, rcounts, rdispls)
    with nogil: CHKERR( MPI_Allgatherv(sbuf, scount,           stype,
                                       rbuf, rcounts, rdispls, rtype,
                                       comm) )
    rmsg = pickler.loadv(rmsg, size, rcounts, rdispls)
    return rmsg


cdef object PyMPI_alltoall(object sendobj, object recvobj,
                           MPI_Comm comm):
    cdef _p_Pickler pickler = PyMPI_pickler()
    #
    cdef void *sbuf = NULL
    cdef int *scounts = NULL
    cdef int *sdispls = NULL
    cdef MPI_Datatype stype = MPI_BYTE
    cdef void *rbuf = NULL
    cdef int *rcounts = NULL
    cdef int *rdispls = NULL
    cdef MPI_Datatype rtype = MPI_BYTE
    #
    cdef int inter=0, size=0
    CHKERR( MPI_Comm_test_inter(comm, &inter) )
    if inter:
        CHKERR( MPI_Comm_remote_size(comm, &size) )
    else:
        CHKERR( MPI_Comm_size(comm, &size) )
    #
    cdef object stmp1 = newarray_int(size, &scounts)
    cdef object stmp2 = newarray_int(size, &sdispls)
    cdef object rtmp1 = newarray_int(size, &rcounts)
    cdef object rtmp2 = newarray_int(size, &rdispls)
    #
    cdef object smsg = pickler.dumpv(sendobj, &sbuf, size, scounts, sdispls)
    with nogil: CHKERR( MPI_Alltoall(scounts, 1, MPI_INT,
                                     rcounts, 1, MPI_INT,
                                     comm) )
    cdef object rmsg = pickler.allocv(&rbuf, size, rcounts, rdispls)
    with nogil: CHKERR( MPI_Alltoallv(sbuf, scounts, sdispls, stype,
                                      rbuf, rcounts, rdispls, rtype,
                                      comm) )
    rmsg = pickler.loadv(rmsg, size, rcounts, rdispls)
    return rmsg

# --------------------------------------------------------------------

cdef object PyMPI_reduce(object sendobj, object recvobj,
                         object op, int root, MPI_Comm comm):
    items = PyMPI_gather(sendobj, recvobj, root, comm)
    return _py_reduce(op, items)


cdef object PyMPI_allreduce(object sendobj, object recvobj,
                            object op, MPI_Comm comm):
    items = PyMPI_allgather(sendobj, recvobj, comm)
    return _py_reduce(op, items)


cdef object PyMPI_scan(object sendobj, object recvobj,
                       object op, MPI_Comm comm):
    items = PyMPI_gather(sendobj, None, 0, comm)
    items = _py_scan(op, items)
    return PyMPI_scatter(items, None, 0, comm)


cdef object PyMPI_exscan(object sendobj, object recvobj,
                         object op, MPI_Comm comm):
    items = PyMPI_gather(sendobj, None, 0, comm)
    items = _py_exscan(op, items)
    return PyMPI_scatter(items, None, 0, comm)

# --------------------------------------------------------------------
