PROGRAM coproc
#ifdef USE_CATALYST
  use tcp               ! ParaView Catalyst adaptor
#endif
  use SparseMatrix      ! contains initialize() and finalize()
  use PoissonDiscretization ! contains fillmatrixandrhs()
  use ConjugateGradient ! contains solve()
  implicit none
  include 'mpif.h'
  integer,parameter :: nx=100,ny=100,nz=100,ntime=10
  integer :: i,j,k
  real(kind=8) :: dxsqinverse, dysqinverse, dzsqinverse, value
  integer :: numtasks,rank,ierr
  integer :: dimensions(3), ownedbox(6)
  type(SparseMatrixData) :: sm
  integer :: numx, numy, numz, row, allocatestatus
  real(kind=8), DIMENSION(:), allocatable :: x, rhs
  real(kind=8) :: rhs2(125)

  call mpi_init(ierr)
  call mpi_comm_size(MPI_COMM_WORLD, numtasks, ierr)
  call mpi_comm_rank(MPI_COMM_WORLD, rank, ierr)

#ifdef USE_CATALYST
  call initializecoprocessor()
#endif

  dimensions(1) = 5
  dimensions(2) = 5
  dimensions(3) = 5

  ! given a rank between 1 and numtasks, compute the nodes that
  ! are owned by this process (ownedbox)
  call getownedbox(rank, numtasks, dimensions, ownedbox)

  call initialize(sm, ownedbox, dimensions)

  ! each process has a full copy of the solution and the RHS to keep things simple
  allocate(x(dimensions(1)*dimensions(2)*dimensions(3)), rhs(dimensions(1)*dimensions(2)*dimensions(3)), STAT = allocatestatus)
  if (allocatestatus /= 0) STOP "*** Not enough memory for arrays ***"

  call fillmatrixandrhs(sm, rhs, ownedbox, dimensions)

  call solve(dimensions, sm, x, rhs2)

#ifdef USE_CATALYST
  call finalizecoprocessor()
#endif

  deallocate(x, rhs)

  call finalize(sm)

  call mpi_finalize(ierr)

end program coproc
