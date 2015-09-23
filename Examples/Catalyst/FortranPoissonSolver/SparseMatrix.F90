module SparseMatrix
  ! this uses the MKL sparse matrix storage scheme. it assumes we have a
  ! node-based partitioning of the grid.
  implicit none
  public :: initialize, insert, finalize, allreducevector, matvec

  type SparseMatrixData
     real(kind=8), DIMENSION(:), allocatable :: values
     integer, DIMENSION(:), allocatable :: columns, rowindices, mapping
     integer :: globalsize
  end type SparseMatrixData

contains
  subroutine initialize(sm, ownedbox, dimensions)
    type(SparseMatrixData), intent(inout) :: sm
    integer, intent(in) :: ownedbox(6), dimensions(3)
    integer :: numpoints, i, j, k, numx, numy, numz, counter, rowcounter, globalindex
    logical :: negativey, positivey, negativez, positivez
    integer :: allocatestatus

    numpoints = 1
    numx = ownedbox(2)-ownedbox(1)+1
    if(numx < 1) numx = 1
    numy = ownedbox(4)-ownedbox(3)+1
    if(numy < 1) numy = 1
    numz = ownedbox(6)-ownedbox(5)+1
    if(numz < 1) numz = 1
    numpoints = numx*numy*numz

    ! over-allocate both values and columns
    allocate(sm%rowindices(numpoints+1), sm%columns(7*numpoints), sm%values(7*numpoints), STAT = allocatestatus)
    if (allocatestatus /= 0) STOP "*** SparseMatrix.F90: Not enough memory for arrays ***"

    sm%values(:) = 0.d0
    sm%columns(:) = -1

    ! a mapping from the global row index to the local row index
    sm%globalsize = dimensions(1)*dimensions(2)*dimensions(3)
    allocate(sm%mapping(sm%globalsize))
    sm%mapping(:) = -1 ! initialize with bad values

    rowcounter = 1
    counter = 1
    do k=1, numz
       negativez = .TRUE.
       if(k .eq. 1 .and. ownedbox(5) .eq. 1) negativez = .FALSE.
       positivez = .TRUE.
       if(k .eq. numz .and. ownedbox(6) .eq. dimensions(3)) positivez = .FALSE.
       do j=1, numy
          negativey = .TRUE.
          if(j .eq. 1 .and. ownedbox(3) .eq. 1) negativey = .FALSE.
          positivey = .TRUE.
          if(j .eq. numy .and. ownedbox(4) .eq. dimensions(2)) positivey = .FALSE.
          do i=1, numx
             globalindex = (ownedbox(5)+k-2)*dimensions(1)*dimensions(2)+(ownedbox(3)+j-2)*dimensions(1)+ownedbox(1)+i-1
             sm%rowindices(rowcounter) = counter
             sm%mapping(globalindex) = rowcounter
             rowcounter = rowcounter + 1
             if(negativez) then
                sm%columns(counter) = globalindex-dimensions(1)*dimensions(2)
                counter = counter + 1
             endif
             if(negativey) then
                sm%columns(counter) = globalindex-dimensions(1)
                counter = counter + 1
             endif
             if(i .ne. 1 .or. ownedbox(1) .ne. 1) then
                sm%columns(counter) = globalindex-1
                counter = counter + 1
             endif
             sm%columns(counter) = globalindex
             counter = counter + 1
             if(i .ne. numx .or. ownedbox(2) .ne. dimensions(1)) then
                sm%columns(counter) = globalindex+1
                counter = counter + 1
             endif
             if(positivey) then
                sm%columns(counter) = globalindex+dimensions(1)
                counter = counter + 1
             endif
             if(positivez) then
                sm%columns(counter) = globalindex+dimensions(1)*dimensions(2)
                counter = counter + 1
             endif
          end do
       end do
    end do
    ! the final value for ending the row
    sm%rowindices(rowcounter) = counter
  end subroutine initialize

  subroutine finalize(sm)
    type(SparseMatrixData), intent(inout) :: sm
    if(allocated(sm%values)) deallocate(sm%values)
    if(allocated(sm%columns)) deallocate(sm%columns)
    if(allocated(sm%rowindices)) deallocate(sm%rowindices)
    if(allocated(sm%mapping)) deallocate(sm%mapping)
  end subroutine finalize

  subroutine insert(sm, row, column, value)
    type(SparseMatrixData), intent(inout) :: sm
    integer, intent(in) :: row, column
    real(kind=8), intent(in) :: value
    integer :: i, rowindex

    rowindex = sm%mapping(row)

    do i=sm%rowindices(rowindex), sm%rowindices(rowindex+1)-1
       if(sm%columns(i) .eq. column) then
          sm%values(i) = value
          return
       endif
    end do

    write(*,*) 'SparseMatrix: bad indices for insert() ', row, column
  end subroutine insert

  subroutine get(sm, row, column, value)
    type(SparseMatrixData), intent(in) :: sm
    integer, intent(in) :: row, column
    real(kind=8), intent(out) :: value
    integer :: i, rowindex

    rowindex = sm%mapping(row)

    do i=sm%rowindices(rowindex), sm%rowindices(rowindex+1)-1
       if(sm%columns(i) .eq. column) then
          value = sm%values(i)
          return
       endif
    end do
    write(*,*) 'SparseMatrix: bad indices for get() ', row, column
  end subroutine get

  subroutine matvec(sm, x, b)
    !Ax=b
    type(SparseMatrixData), intent(inout) :: sm
    real(kind=8), intent(in) :: x(:)
    real(kind=8), intent(out) :: b(:)
    integer :: i, j

    do i=1, sm%globalsize
       b(i) = 0.d0
       if(sm%mapping(i) .gt. 0) then
          do j=sm%rowindices(sm%mapping(i)), sm%rowindices(sm%mapping(i)+1)-1
             b(i) = b(i) + sm%values(j)*x(sm%columns(j))
          end do
       endif
    end do

    call allreducevector(sm%globalsize, b)
  end subroutine matvec

  subroutine allreducevector(globalsize, vec)
    implicit none
    include 'mpif.h'
    integer, intent(in) :: globalsize
    real(kind=8), intent(inout) :: vec(:)
    real(kind=8), DIMENSION(:), allocatable :: temp
    integer :: ierr, i, numtasks, allocatestatus

    call mpi_comm_size(MPI_COMM_WORLD, numtasks, ierr)
    if(numtasks .eq. 1) then
       return
    endif

    allocate(temp(globalsize), STAT = allocatestatus)
    if (allocatestatus /= 0) STOP "*** SparseMatrix.F90: Not enough memory for temp array ***"
    do i=1, globalsize
       temp(i) = vec(i)
    enddo
    call mpi_allreduce(temp, vec, globalsize, MPI_DOUBLE_PRECISION, MPI_SUM, MPI_COMM_WORLD, ierr)
    deallocate(temp)

  end subroutine allreducevector

end module SparseMatrix
