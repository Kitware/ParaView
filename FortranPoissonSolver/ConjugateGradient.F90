module ConjugateGradient
  use SparseMatrix
#ifdef USE_CATALYST
  use tcp
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
    real(kind=8) :: alpha, beta, rdotproduct, rnewdotproduct, origresid
    real(kind=8), DIMENSION(:), allocatable :: r(:), rnew(:), p(:), ap(:)

    allocate(r(sm%globalsize), rnew(sm%globalsize), p(sm%globalsize), ap(sm%globalsize))

#ifdef USE_CATALYST
    x(:) = 0.d0
    call runcoprocessor(dimensions, 0, 0.d0, x)
#endif

    r(:) = rhs(:)
    p(:) = rhs(:)
    k = 1
    rdotproduct = dotproduct(sm, r, r)
    origresid = rdotproduct
    do while(k .le. sm%globalsize .and. rdotproduct .gt. origresid*0.00001d0)
       call matvec(sm, p, ap)
       alpha = rdotproduct/dotproduct(sm, ap, p)
       x(:) = x(:) + alpha*p(:)
       rnew(:) = r(:) - alpha*ap(:)
       rnewdotproduct = dotproduct(sm, rnew, rnew)
       beta = rnewdotproduct/rdotproduct
       p(:) = rnew(:) + beta*p(:)
       rdotproduct = rnewdotproduct
       !write(*,*) 'on iteration ', k, origresid, rdotproduct, alpha
#ifdef USE_CATALYST
       call runcoprocessor(dimensions, k, k*1.d0, x)
#endif
       k = k+1
    end do

    deallocate(r, rnew, p, ap)

  end subroutine solve


end module ConjugateGradient
