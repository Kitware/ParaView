# Storage order for arrays
# ------------------------

ORDER_C       = MPI_ORDER_C        #: C order (a.k.a. row major)
ORDER_FORTRAN = MPI_ORDER_FORTRAN  #: Fortran order (a.k.a. column major)
ORDER_F       = MPI_ORDER_FORTRAN  #: Convenience alias for ORDER_FORTRAN


# Type classes for Fortran datatype matching
# ------------------------------------------
TYPECLASS_INTEGER = MPI_TYPECLASS_INTEGER
TYPECLASS_REAL    = MPI_TYPECLASS_REAL
TYPECLASS_COMPLEX = MPI_TYPECLASS_COMPLEX


# Type of distributions (HPF-like arrays)
# ---------------------------------------

DISTRIBUTE_NONE      = MPI_DISTRIBUTE_NONE       #: Dimension not distributed
DISTRIBUTE_BLOCK     = MPI_DISTRIBUTE_BLOCK      #: Block distribution
DISTRIBUTE_CYCLIC    = MPI_DISTRIBUTE_CYCLIC     #: Cyclic distribution
DISTRIBUTE_DFLT_DARG = MPI_DISTRIBUTE_DFLT_DARG  #: Default distribution argument


cdef class Datatype:

    """
    Datatype
    """

    def __cinit__(self):
        self.ob_mpi = MPI_DATATYPE_NULL

    def __dealloc__(self):
        if not (self.flags & PyMPI_OWNED): return
        CHKERR( _del_Datatype(&self.ob_mpi) )

    def __richcmp__(self, other, int op):
        if not isinstance(self,  Datatype): return NotImplemented
        if not isinstance(other, Datatype): return NotImplemented
        cdef Datatype s = self, o = other
        if   op == 2: return (s.ob_mpi == o.ob_mpi)
        elif op == 3: return (s.ob_mpi != o.ob_mpi)
        else: raise TypeError(mpistr("only '==' and '!='"))

    def __nonzero__(self):
        return self.ob_mpi != MPI_DATATYPE_NULL

    # Datatype Accessors
    # ------------------

    def Get_extent(self):
        """
        Return lower bound and extent of datatype
        """
        cdef MPI_Aint lb = 0, extent = 0
        CHKERR( MPI_Type_get_extent(self.ob_mpi, &lb, &extent) )
        return (lb, extent)

    property extent:
        """extent"""
        def __get__(self):
            cdef MPI_Aint lb = 0, extent = 0
            CHKERR( MPI_Type_get_extent(self.ob_mpi, &lb, &extent) )
            return extent

    property lb:
        """lower bound"""
        def __get__(self):
            cdef MPI_Aint lb = 0, extent = 0
            CHKERR( MPI_Type_get_extent(self.ob_mpi, &lb, &extent) )
            return lb

    property ub:
        """upper bound"""
        def __get__(self):
            cdef MPI_Aint lb = 0, extent = 0
            CHKERR( MPI_Type_get_extent(self.ob_mpi, &lb, &extent) )
            return lb + extent

    def Get_size(self):
        """
        Return the number of bytes occupied
        by entries in the datatype
        """
        cdef int size = 0
        CHKERR( MPI_Type_size(self.ob_mpi, &size) )
        return size

    property size:
        """size (in bytes)"""
        def __get__(self):
            cdef int size = 0
            CHKERR( MPI_Type_size(self.ob_mpi, &size) )
            return size

    # Datatype Constructors
    # ---------------------

    def Dup(self):
        """
        Duplicate a datatype
        """
        cdef Datatype datatype = type(self)()
        CHKERR( MPI_Type_dup(self.ob_mpi, &datatype.ob_mpi) )
        return datatype

    def Create_contiguous(self, int count):
        """
        Create a contiguous datatype
        """
        cdef Datatype datatype = type(self)()
        CHKERR( MPI_Type_contiguous(count, self.ob_mpi,
                                    &datatype.ob_mpi) )
        return datatype

    def Create_vector(self, int count, int blocklength, int stride):
        """
        Create a vector (strided) datatype
        """
        cdef Datatype datatype = type(self)()
        CHKERR( MPI_Type_vector(count, blocklength, stride,
                                self.ob_mpi, &datatype.ob_mpi) )
        return datatype

    def Create_hvector(self, int count, int blocklength, Aint stride):
        """
        Create a vector (strided) datatype
        """
        cdef Datatype datatype = type(self)()
        CHKERR( MPI_Type_create_hvector(count, blocklength, stride,
                                        self.ob_mpi,
                                        &datatype.ob_mpi) )
        return datatype

    def Create_indexed(self, blocklengths, displacements):
        """
        Create an indexed datatype
        """
        cdef int count = len(displacements)
        cdef int *iblen=NULL
        tmp1 = asarray_int(blocklengths,  &iblen, count)
        cdef int *idisp=NULL
        tmp2 = asarray_int(displacements, &idisp, count)
        #
        cdef Datatype datatype = type(self)()
        CHKERR( MPI_Type_indexed(count, iblen, idisp,
                                 self.ob_mpi, &datatype.ob_mpi) )
        return datatype

    def Create_indexed_block(self, int blocklength, displacements):
        """
        Create an indexed datatype
        with constant-sized blocks
        """
        cdef int count = len(displacements)
        cdef int *idisp=NULL
        tmp2 = asarray_int(displacements, &idisp, count)
        #
        cdef Datatype datatype = type(self)()
        CHKERR( MPI_Type_create_indexed_block(count, blocklength,
                                              idisp, self.ob_mpi,
                                              &datatype.ob_mpi) )
        return datatype

    def Create_hindexed(self, blocklengths, displacements):
        """
        Create an indexed datatype
        with displacements in bytes
        """
        cdef int count = len(displacements)
        cdef int *iblen=NULL
        tmp1 = asarray_int(blocklengths, &iblen, count)
        cdef MPI_Aint *idisp=NULL
        tmp2 = asarray_Aint(displacements, &idisp, count)
        #
        cdef Datatype datatype = type(self)()
        CHKERR( MPI_Type_create_hindexed(count, iblen, idisp,
                                         self.ob_mpi,
                                         &datatype.ob_mpi) )
        return datatype

    def Create_subarray(self, sizes, subsizes, starts,
                        int order=ORDER_C):
        """
        Create a datatype for a subarray of
        a regular, multidimensional array
        """
        cdef int ndims = len(sizes)
        cdef int *isizes = NULL
        tmp1 = asarray_int(sizes, &isizes, ndims)
        cdef int *isubsizes = NULL
        tmp2 = asarray_int(subsizes, &isubsizes, ndims)
        cdef int *istarts = NULL
        tmp3 = asarray_int(starts, &istarts, ndims)
        cdef int iorder = MPI_ORDER_C
        if order is not None: iorder = order
        #
        cdef Datatype datatype = type(self)()
        CHKERR( MPI_Type_create_subarray(ndims, isizes,
                                         isubsizes, istarts,
                                         iorder, self.ob_mpi,
                                         &datatype.ob_mpi) )
        return datatype

    def Create_darray(self, int size, int rank,
                      gsizes, distribs, dargs, psizes,
                      int order=ORDER_C):
        """
        Create a datatype representing an HPF-like
        distributed array on Cartesian process grids
        """
        cdef int ndims = len(gsizes)
        cdef int *igsizes=NULL
        tmp1 = asarray_int(gsizes, &igsizes, ndims)
        cdef int *idistribs=NULL
        tmp2 = asarray_int(distribs, &idistribs, ndims)
        cdef int *idargs=NULL
        tmp3 = asarray_int(dargs, &idargs, ndims)
        cdef int *ipsizes=NULL
        tmp4 = asarray_int(psizes, &ipsizes, ndims)
        cdef int iorder = MPI_ORDER_C
        if order is not None: iorder = order
        #
        cdef Datatype datatype = type(self)()
        CHKERR( MPI_Type_create_darray(size, rank, ndims, igsizes,
                                       idistribs, idargs, ipsizes,
                                       iorder, self.ob_mpi,
                                       &datatype.ob_mpi) )
        return datatype

    @classmethod
    def Create_struct(cls, blocklengths, displacements, datatypes):
        """
        Create an datatype from a general set of
        block sizes, displacements and datatypes
        """
        cdef int count = len(displacements)
        cdef int *iblen=NULL
        tmp1 = asarray_int(blocklengths, &iblen, count)
        cdef MPI_Aint *idisp=NULL
        tmp2 = asarray_Aint(displacements, &idisp, count)
        cdef MPI_Datatype *itype=NULL
        tmp3 = asarray_Datatype(datatypes, &itype, count)
        #
        cdef Datatype datatype = cls()
        CHKERR( MPI_Type_create_struct(count, iblen, idisp, itype,
                                       &datatype.ob_mpi) )
        return datatype

    # Size-specific Datatypes
    # -----------------------

    @classmethod
    def Match_size(cls, int typeclass, int size):
        """
        Find a datatype matching a specified size in bytes
        """
        cdef MPI_Datatype datatype = MPI_DATATYPE_NULL
        CHKERR( MPI_Type_match_size(typeclass, size, &datatype) )
        return _new_Datatype(datatype)

    # Use of Derived Datatypes
    # ------------------------

    def Commit(self):
        """
        Commit the datatype
        """
        CHKERR( MPI_Type_commit(&self.ob_mpi) )
        return self

    def Free(self):
        """
        Free the datatype
        """
        CHKERR( MPI_Type_free(&self.ob_mpi) )

    # Datatype Resizing
    # -----------------

    def Create_resized(self, Aint lb, Aint extent):
        """
        Create a datatype with a new lower bound and extent
        """
        cdef Datatype datatype = type(self)()
        CHKERR( MPI_Type_create_resized(self.ob_mpi,
                                        lb, extent,
                                        &datatype.ob_mpi) )
        return datatype

    Resized = Create_resized

    def Get_true_extent(self):
        """
        Return the true lower bound and extent of a datatype
        """
        cdef MPI_Aint lb = 0, extent = 0
        CHKERR( MPI_Type_get_true_extent(self.ob_mpi,
                                         &lb, &extent) )
        return (lb, extent)

    property true_extent:
        """true extent"""
        def __get__(self):
            cdef MPI_Aint lb = 0, extent = 0
            CHKERR( MPI_Type_get_true_extent(self.ob_mpi,
                                             &lb, &extent) )
            return extent

    property true_lb:
        """true lower bound"""
        def __get__(self):
            cdef MPI_Aint lb = 0, extent = 0
            CHKERR( MPI_Type_get_true_extent(self.ob_mpi,
                                             &lb, &extent) )
            return lb

    property true_ub:
        """true upper bound"""
        def __get__(self):
            cdef MPI_Aint lb = 0, extent = 0
            CHKERR( MPI_Type_get_true_extent(self.ob_mpi, &lb,
                                             &extent) )
            return lb + extent

    # Pack and Unpack
    # ---------------

    def Pack(self, inbuf, outbuf, int position, Comm comm not None):
        """
        Pack into contiguous memory according to datatype.
        """
        cdef MPI_Aint lb = 0, extent = 0
        CHKERR( MPI_Type_get_extent(self.ob_mpi, &lb, &extent) )
        #
        cdef void *ibptr = NULL, *obptr = NULL
        cdef MPI_Aint iblen = 0, oblen = 0
        cdef ob1 = asbuffer_r(inbuf,  &ibptr, &iblen)
        cdef ob2 = asbuffer_w(outbuf, &obptr, &oblen)
        cdef int icount = <int>(iblen/extent), osize = <int>oblen
        #
        CHKERR( MPI_Pack(ibptr, icount, self.ob_mpi, obptr, osize,
                         &position, comm.ob_mpi) )
        return position

    def Unpack(self, inbuf, int position, outbuf, Comm comm not None):
        """
        Unpack from contiguous memory according to datatype.
        """
        cdef MPI_Aint lb = 0, extent = 0
        CHKERR( MPI_Type_get_extent(self.ob_mpi, &lb, &extent) )
        #
        cdef void *ibptr = NULL, *obptr = NULL
        cdef MPI_Aint iblen = 0, oblen = 0
        cdef ob1 = asbuffer_r(inbuf,  &ibptr, &iblen)
        cdef ob2 = asbuffer_w(outbuf, &obptr, &oblen)
        cdef int isize = <int>iblen, ocount = <int>(oblen/extent)
        #
        CHKERR( MPI_Unpack(ibptr, isize, &position, obptr, ocount,
                           self.ob_mpi, comm.ob_mpi) )
        return position

    def Pack_size(self, int count, Comm comm not None):
        """
        Returns the upper bound on the amount of space (in bytes)
        needed to pack a message according to datatype.
        """
        cdef int size = 0
        CHKERR( MPI_Pack_size(count, self.ob_mpi,
                              comm.ob_mpi, &size) )
        return size

    # Canonical Pack and Unpack
    # -------------------------

    def Pack_external(self, datarep, inbuf, outbuf, Aint position):
        """
        Pack into contiguous memory according to datatype,
        using a portable data representation (**external32**).
        """
        cdef char *cdatarep = NULL
        datarep = asmpistr(datarep, &cdatarep, NULL)
        cdef MPI_Aint lb = 0, extent = 0
        CHKERR( MPI_Type_get_extent(self.ob_mpi, &lb, &extent) )
        #
        cdef void *ibptr = NULL, *obptr = NULL
        cdef MPI_Aint iblen = 0, oblen = 0
        cdef ob1 = asbuffer_r(inbuf,  &ibptr, &iblen)
        cdef ob2 = asbuffer_w(outbuf, &obptr, &oblen)
        cdef int icount = <int>(iblen/extent)
        cdef MPI_Aint osize = <int>oblen
        #
        CHKERR( MPI_Pack_external(cdatarep, ibptr, icount,
                                  self.ob_mpi,
                                  obptr, osize, &position) )
        return position

    def Unpack_external(self, datarep, inbuf, Aint position, outbuf):
        """
        Unpack from contiguous memory according to datatype,
        using a portable data representation (**external32**).
        """
        cdef char *cdatarep = NULL
        datarep = asmpistr(datarep, &cdatarep, NULL)
        cdef MPI_Aint lb = 0, extent = 0
        CHKERR( MPI_Type_get_extent(self.ob_mpi, &lb, &extent) )
        #
        cdef void *ibptr = NULL, *obptr = NULL
        cdef MPI_Aint iblen = 0, oblen = 0
        cdef ob1 = asbuffer_r(inbuf,  &ibptr, &iblen)
        cdef ob2 = asbuffer_w(outbuf, &obptr, &oblen)
        cdef MPI_Aint isize = iblen,
        cdef int ocount = <int>(oblen/extent)
        #
        CHKERR( MPI_Unpack_external(cdatarep, ibptr, isize, &position,
                                    obptr, ocount, self.ob_mpi) )
        return position

    def Pack_external_size(self, datarep, int count):
        """
        Returns the upper bound on the amount of space (in bytes)
        needed to pack a message according to datatype,
        using a portable data representation (**external32**).
        """
        cdef char *cdatarep = NULL
        cdef MPI_Aint size = 0
        datarep = asmpistr(datarep, &cdatarep, NULL)
        CHKERR( MPI_Pack_external_size(cdatarep, count,
                                       self.ob_mpi, &size) )
        return size

    # Naming Objects
    # --------------

    def Get_name(self):
        """
        Get the print name for this datatype
        """
        cdef char name[MPI_MAX_OBJECT_NAME+1]
        cdef int nlen = 0
        CHKERR( MPI_Type_get_name(self.ob_mpi, name, &nlen) )
        return tompistr(name, nlen)

    def Set_name(self, name):
        """
        Set the print name for this datatype
        """
        cdef char *cname = NULL
        name = asmpistr(name, &cname, NULL)
        CHKERR( MPI_Type_set_name(self.ob_mpi, cname) )

    property name:
        """datatype name"""
        def __get__(self):
            return self.Get_name()
        def __set__(self, value):
            self.Set_name(value)

    # Fortran Handle
    # --------------

    def py2f(self):
        """
        """
        return MPI_Type_c2f(self.ob_mpi)

    @classmethod
    def f2py(cls, arg):
        """
        """
        cdef Datatype datatype = cls()
        datatype.ob_mpi = MPI_Type_f2c(arg)
        return datatype



# Address Function
# ----------------

def Get_address(location):
    """
    Get the address of a location in memory
    """
    cdef void *baseptr = NULL
    asmemory(location, &baseptr, NULL)
    cdef MPI_Aint address = 0
    CHKERR( MPI_Get_address(baseptr, &address) )
    return address



cdef Datatype __DATATYPE_NULL__      = _new_Datatype( MPI_DATATYPE_NULL      )

cdef Datatype __CHAR__               = _new_Datatype( MPI_CHAR               )
cdef Datatype __WCHAR__              = _new_Datatype( MPI_WCHAR              )
cdef Datatype __SIGNED_CHAR__        = _new_Datatype( MPI_SIGNED_CHAR        )
cdef Datatype __UNSIGNED_CHAR__      = _new_Datatype( MPI_UNSIGNED_CHAR      )
cdef Datatype __SHORT__              = _new_Datatype( MPI_SHORT              )
cdef Datatype __UNSIGNED_SHORT__     = _new_Datatype( MPI_UNSIGNED_SHORT     )
cdef Datatype __INT__                = _new_Datatype( MPI_INT                )
cdef Datatype __UNSIGNED__           = _new_Datatype( MPI_UNSIGNED           )
cdef Datatype __LONG__               = _new_Datatype( MPI_LONG               )
cdef Datatype __UNSIGNED_LONG__      = _new_Datatype( MPI_UNSIGNED_LONG      )
cdef Datatype __FLOAT__              = _new_Datatype( MPI_FLOAT              )
cdef Datatype __DOUBLE__             = _new_Datatype( MPI_DOUBLE             )
cdef Datatype __LONG_DOUBLE__        = _new_Datatype( MPI_LONG_DOUBLE        )
cdef Datatype __BYTE__               = _new_Datatype( MPI_BYTE               )
cdef Datatype __PACKED__             = _new_Datatype( MPI_PACKED             )

cdef Datatype __SHORT_INT__          = _new_Datatype( MPI_SHORT_INT          )
cdef Datatype __TWOINT__             = _new_Datatype( MPI_2INT               )
cdef Datatype __LONG_INT__           = _new_Datatype( MPI_LONG_INT           )
cdef Datatype __FLOAT_INT__          = _new_Datatype( MPI_FLOAT_INT          )
cdef Datatype __DOUBLE_INT__         = _new_Datatype( MPI_DOUBLE_INT         )
cdef Datatype __LONG_DOUBLE_INT__    = _new_Datatype( MPI_LONG_DOUBLE_INT    )

cdef Datatype __LONG_LONG__          = _new_Datatype( MPI_LONG_LONG          )
cdef Datatype __UNSIGNED_LONG_LONG__ = _new_Datatype( MPI_UNSIGNED_LONG_LONG )
cdef Datatype __LONG_LONG_INT__      = _new_Datatype( MPI_LONG_LONG_INT      )

cdef Datatype __CHARACTER__          = _new_Datatype( MPI_CHARACTER          )
cdef Datatype __LOGICAL__            = _new_Datatype( MPI_LOGICAL            )
cdef Datatype __INTEGER__            = _new_Datatype( MPI_INTEGER            )
cdef Datatype __REAL__               = _new_Datatype( MPI_REAL               )
cdef Datatype __DOUBLE_PRECISION__   = _new_Datatype( MPI_DOUBLE_PRECISION   )
cdef Datatype __F_COMPLEX__          = _new_Datatype( MPI_COMPLEX            )
cdef Datatype __F_DOUBLE_COMPLEX__   = _new_Datatype( MPI_DOUBLE_COMPLEX     )

cdef Datatype __UB__                 = _new_Datatype( MPI_UB                 )
cdef Datatype __LB__                 = _new_Datatype( MPI_LB                 )


# Predefined datatype handles
# ---------------------------

DATATYPE_NULL      = __DATATYPE_NULL__  #: Null datatype handle
# Elementary datatypes
CHAR               = __CHAR__
WCHAR              = __WCHAR__
SIGNED_CHAR        = __SIGNED_CHAR__
UNSIGNED_CHAR      = __UNSIGNED_CHAR__
SHORT              = __SHORT__
UNSIGNED_SHORT     = __UNSIGNED_SHORT__
INT                = __INT__
UNSIGNED           = __UNSIGNED__
LONG               = __LONG__
UNSIGNED_LONG      = __UNSIGNED_LONG__
FLOAT              = __FLOAT__
DOUBLE             = __DOUBLE__
LONG_DOUBLE        = __LONG_DOUBLE__
BYTE               = __BYTE__
PACKED             = __PACKED__
# Datatypes for reduction operations
SHORT_INT          = __SHORT_INT__
TWOINT             = __TWOINT__
LONG_INT           = __LONG_INT__
FLOAT_INT          = __FLOAT_INT__
DOUBLE_INT         = __DOUBLE_INT__
LONG_DOUBLE_INT    = __LONG_DOUBLE_INT__
# Optional datatypes
LONG_LONG          = __LONG_LONG__
UNSIGNED_LONG_LONG = __UNSIGNED_LONG_LONG__
LONG_LONG_INT      = __LONG_LONG_INT__
# Fortran datatypes
CHARACTER          = __CHARACTER__
LOGICAL            = __LOGICAL__
INTEGER            = __INTEGER__
REAL               = __REAL__
DOUBLE_PRECISION   = __DOUBLE_PRECISION__
F_COMPLEX          = __F_COMPLEX__
F_DOUBLE_COMPLEX   = __F_DOUBLE_COMPLEX__
# Special datatypes (for constructing derived datatypes)
UB                 = __UB__  #: upper-bound marker (deprecated in MPI-2)
LB                 = __LB__  #: lower-bound marker (deprecated in MPI-2)
# Convenience aliases (not in the MPI-1/MPI-2 stardards)
UNSIGNED_INT       = __UNSIGNED__
INT_INT            = __TWOINT__
