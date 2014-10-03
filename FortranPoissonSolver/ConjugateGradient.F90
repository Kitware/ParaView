module ConjugateGradient
  use SparseMatrix
#ifdef USE_CATALYST
  use CoProcessor
#endif
  implicit none
  private :: dotproduct
  public :: solve

contains

  real(kind=8) function dotproduct(sm, a, b)
    type(SparseMatrixData), intent(inout) :: sm
    integer :: i
    real(kind=8), intent(in) :: a(:), b(:)
    real(kind=8) :: value

    value = 0.d0
    do i=1, sm%globalsize
       value = value + a(i)*b(i)
    enddo
    dotproduct = value
  end function dotproduct

  subroutine solve(dimensions, sm, x, rhs)
    type(SparseMatrixData), intent(inout) :: sm
    integer, intent(in) :: dimensions(3)
    real(kind=8), intent(in) :: rhs(:)
    real(kind=8), intent(inout) :: x(:)
    integer :: k, i
    real(kind=8) :: alpha, beta, rdotproduct, rnewdotproduct, sqrtorigresid
    real(kind=8), DIMENSION(:), allocatable :: r(:), p(:), ap(:)

    allocate(r(sm%globalsize), p(sm%globalsize), ap(sm%globalsize))

#ifdef USE_CATALYST
    x(:) = 0.d0
    call runcoprocessor(dimensions, 0, 0.d0, x)
#endif

    r(:) = rhs(:)
    p(:) = rhs(:)
    k = 1
    rdotproduct = dotproduct(sm, r, r)
    sqrtorigresid = sqrt(rdotproduct)
    do while(k .le. sm%globalsize .and. sqrt(rdotproduct) .gt. sqrtorigresid*0.000001d0)
       call matvec(sm, p, ap)
       alpha = rdotproduct/dotproduct(sm, ap, p)
       x(:) = x(:) + alpha*p(:)
       r(:) = r(:) - alpha*ap(:)
       rnewdotproduct = dotproduct(sm, r, r)
       beta = rnewdotproduct/rdotproduct
       p(:) = r(:) + beta*p(:)
       rdotproduct = rnewdotproduct
       !write(*,*) 'on iteration ', k, sqrtorigresid, sqrt(rdotproduct), alpha
#ifdef USE_CATALYST
       call runcoprocessor(dimensions, k, k*1.d0, x)
#endif
       k = k+1
    end do

    deallocate(r, p, ap)

  end subroutine solve


end module ConjugateGradient
