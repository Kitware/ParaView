cdef class Group:

    """
    Group
    """

    def __cinit__(self):
        self.ob_mpi = MPI_GROUP_NULL

    def __dealloc__(self):
        if not (self.flags & PyMPI_OWNED): return
        CHKERR( _del_Group(&self.ob_mpi) )

    def __richcmp__(self, other, int op):
        if not isinstance(self,  Group): return NotImplemented
        if not isinstance(other, Group): return NotImplemented
        cdef Group s = self, o = other
        if   op == 2: return (s.ob_mpi == o.ob_mpi)
        elif op == 3: return (s.ob_mpi != o.ob_mpi)
        else: raise TypeError(mpistr("only '==' and '!='"))

    def __nonzero__(self):
        return self.ob_mpi != MPI_GROUP_NULL

    # Group Accessors
    # ---------------

    def Get_size(self):
        """
        Return the size of a group
        """
        cdef int size = -1
        CHKERR( MPI_Group_size(self.ob_mpi, &size) )
        return size

    property size:
        """number of processes in group"""
        def __get__(self):
            return Group.Get_size(self)

    def Get_rank(self):
        """
        Return the rank of this process in a group
        """
        cdef int rank = -1
        CHKERR( MPI_Group_rank(self.ob_mpi, &rank) )
        return rank

    property rank:
        """rank of this process in group"""
        def __get__(self):
            return Group.Get_rank(self)

    @classmethod
    def Translate_ranks(cls, Group group1 not None, ranks1,
                        Group group2=None):
        """
        Translate the ranks of processes in
        one group to those in another group
        """
        cdef MPI_Group grp1 = MPI_GROUP_NULL
        cdef MPI_Group grp2 = MPI_GROUP_NULL
        cdef int i = 0, n = len(ranks1)
        cdef int *iranks1 = NULL, *iranks2 = NULL
        cdef object tmp1 = newarray_int(n, &iranks1)
        cdef object tmp2 = newarray_int(n, &iranks2)
        for i from 0 <= i < n: iranks1[i] = ranks1[i]
        #
        grp1 = group1.ob_mpi
        if group2 is not None:
            grp2 = group2.ob_mpi
        else:
            CHKERR( MPI_Comm_group(MPI_COMM_WORLD, &grp2) )
        try:
            CHKERR( MPI_Group_translate_ranks(grp1, n, iranks1,
                                              grp2, iranks2) )
        finally:
            if group2 is None:
                CHKERR( MPI_Group_free(&grp2) )
        #
        return [iranks2[i] for i from 0 <= i < n]

    @classmethod
    def Compare(cls,
                Group group1 not None,
                Group group2 not None):
        """
        Compare two groups
        """
        cdef int flag = MPI_UNEQUAL
        CHKERR( MPI_Group_compare(group1.ob_mpi, group2.ob_mpi, &flag) )
        return flag

    # Group Constructors
    # ------------------

    def Dup(self):
        """
        Duplicate a group
        """
        cdef Group group = type(self)()
        CHKERR( MPI_Group_union(self.ob_mpi, MPI_GROUP_EMPTY, &group.ob_mpi) )
        return group

    @classmethod
    def Union(cls,
              Group group1 not None,
              Group group2 not None):
        """
        Produce a group by combining
        two existing groups
        """
        cdef Group group = cls()
        CHKERR( MPI_Group_union(group1.ob_mpi, group2.ob_mpi, &group.ob_mpi) )
        return group

    @classmethod
    def Intersect(cls,
                  Group group1 not None,
                  Group group2 not None):
        """
        Produce a group as the intersection
        of two existing groups
        """
        cdef Group group = cls()
        CHKERR( MPI_Group_intersection(group1.ob_mpi, group2.ob_mpi, &group.ob_mpi) )
        return group

    @classmethod
    def Difference(cls,
                   Group group1 not None,
                   Group group2 not None):
        """
        Produce a group from the difference
        of two existing groups
        """
        cdef Group group = cls()
        CHKERR( MPI_Group_difference(group1.ob_mpi, group2.ob_mpi, &group.ob_mpi) )
        return group

    def Incl(self, ranks):
        """
        Produce a group by reordering an existing
        group and taking only listed members
        """
        cdef int i, n = len(ranks), *iranks = NULL
        cdef object tmp = newarray_int(n, &iranks)
        for i from 0 <= i < n: iranks[i] = ranks[i]
        cdef Group group = type(self)()
        CHKERR( MPI_Group_incl(self.ob_mpi, n, iranks, &group.ob_mpi) )
        return group

    def Excl(self, ranks):
        """
        Produce a group by reordering an existing
        group and taking only unlisted members
        """
        cdef int i, n = len(ranks), *iranks = NULL
        cdef object tmp = newarray_int(n, &iranks)
        for i from 0 <= i < n: iranks[i] = ranks[i]
        cdef Group group = type(self)()
        CHKERR( MPI_Group_excl(self.ob_mpi, n, iranks, &group.ob_mpi) )
        return group

    def Range_incl(self, ranks):
        """
        Create a new group from ranges of
        of ranks in an existing group
        """
        cdef int i, n = len(ranks), *p = NULL,
        cdef int (*ranges)[3] # = NULL ## XXX cython fails
        ranges = NULL
        cdef object tmp1 = newarray_int3(n, &ranges)
        for i from 0 <= i < n:
            p = <int*> ranges[i]
            p[0], p[1], p[2] = ranks[i]
        cdef Group group = type(self)()
        CHKERR( MPI_Group_range_incl(self.ob_mpi, n, ranges, &group.ob_mpi) )
        return group

    def Range_excl(self, ranks):
        """
        Create a new group by excluding ranges
        of processes from an existing group
        """
        cdef int i, n = len(ranks), *p = NULL,
        cdef int (*ranges)[3] # = NULL ## XXX cython fails
        ranges = NULL
        cdef object tmp1 = newarray_int3(n, &ranges)
        for i from 0 <= i < n:
            p = <int*> ranges[i]
            p[0], p[1], p[2] = ranks[i]
        cdef Group group = type(self)()
        CHKERR( MPI_Group_range_excl(self.ob_mpi, n, ranges, &group.ob_mpi) )
        return group

    # Group Destructor
    # ----------------

    def Free(self):
        """
        Free a group
        """
        if self.ob_mpi != MPI_GROUP_EMPTY:
            CHKERR( MPI_Group_free(&self.ob_mpi) )
        elif self is not __GROUP_EMPTY__:
            self.ob_mpi = MPI_GROUP_NULL
        else: CHKERR( MPI_ERR_GROUP )

    # Fortran Handle
    # --------------

    def py2f(self):
        """
        """
        return MPI_Group_c2f(self.ob_mpi)

    @classmethod
    def f2py(cls, arg):
        """
        """
        cdef Group group = cls()
        group.ob_mpi = MPI_Group_f2c(arg)
        return group



cdef Group __GROUP_NULL__  = _new_Group ( MPI_GROUP_NULL  )
cdef Group __GROUP_EMPTY__ = _new_Group ( MPI_GROUP_EMPTY )


# Predefined group handles
# ------------------------

GROUP_NULL  = __GROUP_NULL__   #: Null group handle
GROUP_EMPTY = __GROUP_EMPTY__  #: Empty group handle
