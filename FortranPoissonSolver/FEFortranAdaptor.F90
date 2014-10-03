module CoProcessor
  implicit none
  public initializecoprocessor, runcoprocessor, finalizecoprocessor

contains

  subroutine initializecoprocessor()
    implicit none
    integer :: ilen, i
    character(len=200) :: arg

    call coprocessorinitialize()
    do i=1, iargc()
       call getarg(i, arg)
       ilen = len_trim(arg)
       arg(ilen+1:) = char(0)
       call coprocessoraddpythonscript(arg, ilen)
    enddo
  end subroutine initializecoprocessor

  subroutine runcoprocessor(dimensions, step, time, x)
    use iso_c_binding
    implicit none
    integer, intent(in) :: dimensions(3), step
    real(kind=8), dimension(:), intent(in) :: x
    real(kind=8), intent(in) :: time
    integer :: flag, extent(6)
    real(kind=8), DIMENSION(:), allocatable :: xcp(:)

    call requestdatadescription(step,time,flag)
    if (flag .ne. 0) then
       call needtocreategrid(flag)
       call getvtkextent(dimensions, extent)

       if (flag .ne. 0) then
          call createcpimagedata(dimensions, extent)
       end if
       ! x is the array with global values, we need just this process's
       ! values for Catalyst which will be put in xcp
       allocate(xcp((extent(2)-extent(1)+1)*(extent(4)-extent(3)+1)*(extent(6)-extent(5)+1)))
       call getlocalfield(dimensions, extent, x, xcp)

       ! adding //char(0) appends the C++ terminating character
       ! to the Fortran array
       call addfield(xcp,"solution"//char(0))
       call coprocess()
       deallocate(xcp)
    end if
  end subroutine runcoprocessor

  subroutine finalizecoprocessor()
    call coprocessorfinalize()
  end subroutine finalizecoprocessor

  ! helper methods
  subroutine getvtkextent(dimensions, extent)
    use Box
    implicit none
    include 'mpif.h'
    integer, intent(in) :: dimensions(3)
    integer, intent(out) :: extent(6)
    integer :: numtasks, rank, ierr

    call mpi_comm_size(MPI_COMM_WORLD, numtasks, ierr)
    call mpi_comm_rank(MPI_COMM_WORLD, rank, ierr)
    call getlocalbox(rank+1, numtasks, dimensions, extent)
  end subroutine getvtkextent

  subroutine getlocalfield(dimensions, extent, x, xcp)
    implicit none
    integer :: i, j, k, counter
    integer, intent(in) :: dimensions(3), extent(6)
    real(kind=8), dimension(:), intent(in) :: x
    real(kind=8), dimension(:), intent(out) :: xcp
    counter = 1
    do k=extent(5), extent(6)
       do j=extent(3), extent(4)
          do i=extent(1), extent(2)
             xcp(counter) = x(i+(j-1)*dimensions(1)+(k-1)*dimensions(1)*dimensions(2))
             counter = counter + 1
          enddo
       enddo
    enddo
  end subroutine getlocalfield

end module CoProcessor
