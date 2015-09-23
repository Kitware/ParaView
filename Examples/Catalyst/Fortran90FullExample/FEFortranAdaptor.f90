module tcp
  use iso_c_binding
  implicit none
  public
  interface tcp_adaptor
    module procedure testcoprocessor
    end interface
    contains

subroutine testcoprocessor(nxstart,nxend,nx,ny,nz,step,time,psi01)
  use iso_c_binding
  implicit none
  integer, intent(in) :: nxstart,nxend,nx,ny,nz,step
  real(kind=8), intent(in) :: time
  complex(kind=8), dimension(:,:,:), intent (in) :: psi01
  integer :: flag
  call requestdatadescription(step,time,flag)
  if (flag .ne. 0) then
    call needtocreategrid(flag)
    if (flag .ne. 0) then
      call createcpimagedata(nxstart,nxend,nx,nz,nz)
    end if
    ! adding //char(0) appends the C++ terminating character
    ! to the Fortran array
    call addfield(psi01,"psi01"//char(0))
    call coprocess()
  end if

  return

  end subroutine
end module tcp
