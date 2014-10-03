PROGRAM coproc
#ifdef USE_CATALYST
  use CoProcessor               ! ParaView Catalyst adaptor
#endif
  use SparseMatrix              ! contains initialize() and finalize()
  use PoissonDiscretization     ! contains fillmatrixandrhs()
  use ConjugateGradient         ! contains solve()
  use Box                       ! contains getownedbox()
  implicit none
  include 'mpif.h'
  integer :: numtasks,rank,ierr,allocatestatus
  integer :: dimensions(3), ownedbox(6)
  type(SparseMatrixData) :: sm
  real(kind=8), DIMENSION(:), allocatable :: x, rhs

  call mpi_init(ierr)
  call mpi_comm_size(MPI_COMM_WORLD, numtasks, ierr)
  call mpi_comm_rank(MPI_COMM_WORLD, rank, ierr)

#ifdef USE_CATALYST
  call initializecoprocessor()
#endif

  dimensions(1) = 10
  dimensions(2) = 10
  dimensions(3) = 10

  ! given a piece between 1 and numtasks, compute the nodes that
  ! are owned by this process (ownedbox)
  call getownedbox(rank+1, numtasks, dimensions, ownedbox)

  call initialize(sm, ownedbox, dimensions)

  ! each process has a full copy of the solution and the RHS to keep things simple
  allocate(x(dimensions(1)*dimensions(2)*dimensions(3)), rhs(dimensions(1)*dimensions(2)*dimensions(3)), STAT = allocatestatus)
  if (allocatestatus /= 0) STOP "*** FEDriver.F90: Not enough memory for arrays ***"

  call fillmatrixandrhs(sm, rhs, ownedbox, dimensions)

  call solve(dimensions, sm, x, rhs)

#ifdef USE_CATALYST
  call finalizecoprocessor()
#endif

  deallocate(x, rhs)

  call finalize(sm)

  call mpi_finalize(ierr)

  write(*,*) 'Finished on rank', rank, 'of', numtasks

end program coproc
