module PoissonDiscretization
  implicit none
  public :: fillmatrixandrhs

contains
  subroutine fillmatrixandrhs(sm, rhs, ownedbox, dimensions)
    use SparseMatrix
    implicit none
    real(kind=8), intent(out) :: rhs(:)
    type(SparseMatrixData), intent(inout) :: sm
    integer, intent(in) :: ownedbox(6), dimensions(3)
    integer :: i,j,k
    real(kind=8) :: dxsqinverse, dysqinverse, dzsqinverse, value
    logical :: negativex, negativey, negativez, positivex, positivey, positivez, interior
    integer :: numx, numy, numz, row

    ! construct the matrix
    numx = ownedbox(2)-ownedbox(1)+1
    if(numx < 1) numx = 1
    numy = ownedbox(4)-ownedbox(3)+1
    if(numy < 1) numy = 1
    numz = ownedbox(6)-ownedbox(5)+1

    rhs(:) = 0.0

    dxsqinverse = 1.d0/((dimensions(1)-1)*(dimensions(1)-1))
    dysqinverse = 1.d0/((dimensions(2)-1)*(dimensions(2)-1))
    dzsqinverse = 1.d0/((dimensions(3)-1)*(dimensions(3)-1))

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
             negativex = .TRUE.
             if(i .eq. 1 .and. ownedbox(1) .eq. 1) negativex = .FALSE.
             positivex = .TRUE.
             if(i .eq. numx .and. ownedbox(2) .eq. dimensions(1)) positivex = .FALSE.
             interior = negativex .and. negativey .and. negativez .and. positivex .and. positivey .and. positivez
             row = i+ownedbox(1)-1+(j+ownedbox(3)-2)*dimensions(1)+(k+ownedbox(5)-2)*dimensions(1)*dimensions(2)
             if(.NOT. interior) then
                call insert(sm, row, row, 1.d0)
                if(.NOT. positivez) rhs(row) = 1.d0
             else
                call insert(sm, row, row-1, dxsqinverse)
                call insert(sm, row, row+1, dxsqinverse)
                call insert(sm, row, row-dimensions(1), dysqinverse)
                call insert(sm, row, row+dimensions(1), dysqinverse)
                call insert(sm, row, row-dimensions(1)*dimensions(2), dzsqinverse)
                call insert(sm, row, row+dimensions(1)*dimensions(2), dzsqinverse)
                call insert(sm, row, row, -2.d0*dxsqinverse-2.d0*dysqinverse-2.d0*dzsqinverse)
             endif
          end do
       end do
    end do

    ! now we need to diagonalize the matrix based on boundary conditions
    ! do the non-homogeneous bcs on the z=1 plane first
    if(ownedbox(5) .le. dimensions(3)-1 .and. ownedbox(6) .ge. dimensions(3)-1) then
       do i=ownedbox(1), ownedbox(2)
          do j=ownedbox(3), ownedbox(4)
             row = i+(j-1)*dimensions(1)+dimensions(1)*dimensions(2)*(dimensions(3)-2)
             call get(sm, row, row+dimensions(1)*dimensions(2), value)
             rhs(row) = rhs(row) - value*1.d0
             call insert(sm, row, row+dimensions(1)*dimensions(2), 0.d0)
          enddo
       enddo
    endif
    ! the homogeneous bcs
    ! the z=0 plane
    if(ownedbox(5) .le. 2 .and. ownedbox(6) .ge. 2) then
       do i=ownedbox(1), ownedbox(2)
          do j=ownedbox(3), ownedbox(4)
             row = i+(j-1)*dimensions(1)+dimensions(1)*dimensions(2)
             call insert(sm, row, row-dimensions(1)*dimensions(2), 0.d0)
          enddo
       enddo
    endif
    ! the x=0 plane
    if(ownedbox(1) .le. 2 .and. ownedbox(2) .ge. 2) then
       do i=ownedbox(3), ownedbox(4)
          do j=ownedbox(5), ownedbox(6)
             row = 2+(i-1)*dimensions(1)+(j-1)*dimensions(1)*dimensions(2)
             call insert(sm, row, row-1, 0.d0)
          enddo
       enddo
    endif
    ! the x=1 plane
    if(ownedbox(1) .le. dimensions(1)-1 .and. ownedbox(2) .ge. dimensions(1)-1) then
       do i=ownedbox(3), ownedbox(4)
          do j=ownedbox(5), ownedbox(6)
             row = dimensions(1)-1+(i-1)*dimensions(1)+(j-1)*dimensions(1)*dimensions(2)
             call insert(sm, row, row+1, 0.d0)
          enddo
       enddo
    endif
    ! the y=0 plane
    if(ownedbox(3) .le. 2 .and. ownedbox(4) .ge. 2) then
       do i=ownedbox(1), ownedbox(2)
          do j=ownedbox(5), ownedbox(6)
             row = i+dimensions(1)+(j-1)*dimensions(1)*dimensions(2)
             call insert(sm, row, row-dimensions(1), 0.d0)
          enddo
       enddo
    endif
    ! the y=1 plane
    if(ownedbox(3) .le. dimensions(2)-1 .and. ownedbox(4) .ge. dimensions(2)-1) then
       do i=ownedbox(1), ownedbox(2)
          do j=ownedbox(5), ownedbox(6)
             row = i+dimensions(1)*(dimensions(2)-2)+(j-1)*dimensions(1)*dimensions(2)
             call insert(sm, row, row+dimensions(1), 0.d0)
          enddo
       enddo
    endif

    ! now do an mpi_allreduce on the rhs
    call allreducevector(sm%globalsize, rhs)

  end subroutine fillmatrixandrhs

end module PoissonDiscretization
