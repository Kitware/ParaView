! A Fortran Catalyst example. Note that coproc.py
! must be in the directory where the example
! is run from.
! Thanks to Lucas Pettey for helping create the example.

PROGRAM coproc
  use tcp
  implicit none
  Integer,parameter :: nx=100,ny=100,nz=100,ntime=100
  integer :: i,j,k,time
  real :: max
  complex(kind=8) :: psi01(nx,ny,nz)
  max=sqrt(real(nx)**2+real(ny)**2+real(nz)**2)
  call coprocessorinitializewithpython("coproc.py",9)
  do k=1,nz
    do j=1,ny
      do i=1,nx
        psi01(i,j,k)=CMPLX(max-sqrt(real(i-50)**2+real(j-50)**2+real(k-50)**2),&
                           real(1+j+k))/max*100.0
      enddo
    enddo
  enddo

  do time=1,ntime
    do k=1,nz
      do j=1,ny
        do i=1,nx
          psi01(i,j,k)=CMPLX(real(0.30),0.0)+psi01(i,j,k)
        end do
      end do
    end do
   call testcoprocessor(nx,ny,nz,time,dble(time),psi01)
 enddo
 call coprocessorfinalize()
end program coproc
