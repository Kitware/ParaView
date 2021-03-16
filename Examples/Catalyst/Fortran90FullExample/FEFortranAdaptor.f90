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
  use catalyst
  implicit none
  integer, intent(in) :: nxstart,nxend,nx,ny,nz,step
  real(kind=8), intent(in) :: time
  complex(kind=8), dimension(:,:,:), intent (in) :: psi01
  logical :: flag
  flag = catalyst_request_data_description(step, time)
  if (flag) then
    flag = catalyst_need_to_create_grid()
    if (flag) then
      call createcpimagedata(nxstart,nxend,nx,nz,nz)
    end if
    ! adding //char(0) appends the C++ terminating character
    ! to the Fortran array
    call addfield(psi01,"psi01"//char(0))
    call catalyst_process()
  end if

  return

  end subroutine
end module tcp
