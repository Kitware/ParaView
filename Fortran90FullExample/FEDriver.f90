! A Fortran Catalyst example. Note that coproc.py
! must be in the directory where the example
! is run from.
! Thanks to Lucas Pettey for helping create the example.

PROGRAM coproc
  use tcp
  implicit none
  include 'mpif.h'
  integer,parameter :: nx=100,ny=100,nz=100,ntime=10
  integer :: i,j,k,time,nxstart,nxend
  real :: max
  complex(kind=8), allocatable :: psi01(:,:,:)
  integer :: numtasks,rank,ierr

  call mpi_init(ierr)
  call mpi_comm_size(MPI_COMM_WORLD, numtasks, ierr)
  call mpi_comm_rank(MPI_COMM_WORLD, rank, ierr)

  call coprocessorinitializewithpython("coproc.py",9)

  ! partition in the x-direction only
  nxstart=rank*nx/numtasks+1
  nxend=(rank+1)*nx/numtasks
  if(numtasks .ne. rank+1) then
     nxend=nxend+1
  endif

  allocate(psi01(nxend-nxstart+1,ny,nz))
  ! set initial values
  max=sqrt(real(nx)**2+real(ny)**2+real(nz)**2)
  do k=1,nz
     do j=1,ny
        do i=1,nxend-nxstart+1
           psi01(i,j,k)=CMPLX(max-sqrt(real(i-50)**2+real(j-50)**2+real(k-50)**2),&
                real(1+j+k))/max*100.0
        enddo
     enddo
  enddo

  do time=1,ntime
     do k=1,nz
        do j=1,ny
           do i=1,nxend-nxstart+1
              psi01(i,j,k)=CMPLX(real(0.30),0.0)+psi01(i,j,k)
           end do
        end do
     end do
     call testcoprocessor(nxstart,nxend,nx,ny,nz,time,dble(time),psi01)
  enddo
  deallocate(psi01)

  call coprocessorfinalize()
  call mpi_finalize(ierr)

end program coproc
