!===========================================================================
! Creates four grids for storing data. While all grids are unstructured,
! the data is 2D rectilinear, 2D structured (sphere), 3D rectilinear,
! 3D structured (sphere)
function fv_catalyst_create_grid(nstep, time, dim, &
     lonCoord, latCoord, levCoord, nPoints2D, myRank) result(continueProcessing)
  use ppgrid,       only: pver
  !-----------------------------------------------------------------------
  !
  ! Arguments
  !
  integer, intent(in)                :: nstep ! current timestep number
  real(kind=8),intent(in)            :: time  ! current time
  integer, dimension(1:3),intent(in) :: dim   ! dimensions: lon, lat, lev
  real(r8), allocatable,intent(in)   :: lonCoord(:)
  real(r8), pointer, intent(in)      :: latCoord(:)
  real(r8),intent(in)                :: levCoord(pver)
  integer, intent(in)                :: nPoints2D
  integer, intent(in)                :: myRank
  logical                            :: continueProcessing
  !
  ! Locals
  !
  integer                            :: i,c,j ! indexes
  real(r8)                 :: transvar(1)

  write(*,'(a, i5.2, f5.2)') "fv_catalyst_create_grid: ", nstep, time
  if (fv_requestdatadescription(nstep, time)) then
     continueProcessing = .true.
     if (fv_needtocreategrid()) then
        call fv_create_grid(dim, lonCoord, latCoord, levCoord, nPoints2D, &
             pcols, myRank)
     end if
  else
     continueProcessing = .false.
  end if
end function fv_catalyst_create_grid

!===========================================================================
! Add a chunk of data to the grid. Makes conversions from
! multi-dimensional arrays to uni-dimensional arrays so that the arrays are
! received correctly in C++
subroutine fv_catalyst_add_chunk(nstep, time, phys_state, ncols)
  use ppgrid,       only: pver
  !-----------------------------------------------------------------------
  !
  ! Arguments
  !
  integer, intent(in)                :: nstep ! current timestep number
  real(kind=8),intent(in)            :: time  ! current time
  type(physics_state), intent(in)    :: phys_state
  integer, intent(in)                :: ncols
  !
  ! Locals
  !
  integer                            :: i,c,j ! indexes
  real(r8)                           :: psTransVar(1), tTransVar(1), &
       uTransVar(1), vTransVar(1)

  write(*, "(A, I4, A, I4,  A, I4, A, I4, A, I4)") &
       "fv_catalyst_add_chunk: lchnk=", phys_state%lchnk, &
       " ncols=", ncols, &
       " size(lon)=", size(phys_state%lon), &
       " size(lat)=", size(phys_state%lat), &
       " size(ps,1)=",  size(phys_state%ps, 1),  &
       " size(t)=",   size(phys_state%t)
  call fv_add_chunk(nstep, ncols, &
       phys_state%lon, phys_state%lat, &
       transfer(phys_state%ps, psTransVar), &
       transfer(phys_state%t, tTransVar), &
       transfer(phys_state%u, uTransVar), &
       transfer(phys_state%v, vTransVar))
end subroutine fv_catalyst_add_chunk

!===========================================================================
! Coprocesses data
subroutine fv_catalyst_coprocess(nstep, time)
  use ppgrid,       only: pver
  !-----------------------------------------------------------------------
  !
  ! Arguments
  !
  integer, intent(in)                :: nstep ! current timestep number
  real(kind=8),intent(in)            :: time  ! current time
  !
  ! Locals
  !
  write(*,'(a, i5.2, f5.2)') "fv_catalyst_coprocess: ", nstep, time
  call fv_coprocess()
end subroutine fv_catalyst_coprocess

!===========================================================================
! Creates two unstructured grids for storing data: a sphere and a ball
function se_catalyst_create_grid(nstep, time, ne, np, &
     nlon, lonRad, nlat, latRad, nlev, levCoord, nPoints2D, myRank) result(continueProcessing)
  use ppgrid,       only: pver
  !-----------------------------------------------------------------------
  !
  ! Arguments
  !
  integer, intent(in)                :: nstep ! current timestep number
  real(kind=8),intent(in)            :: time  ! current time
  integer, intent(in)                :: ne    ! number of cells along each edge of each cube face
  integer, intent(in)                :: np    ! each cell is subdivided by np x np quadrature points
  integer, intent(in)                :: nlon  ! number of longitude values
  real(r8), allocatable,intent(in)   :: lonRad(:) ! longitude values in radians
  integer, intent(in)                :: nlat  ! number of latitude values
  real(r8), pointer, intent(in)      :: latRad(:) ! latitude values in radians
  integer, intent(in)                :: nlev  ! number of level values
  real(r8),intent(in)                :: levCoord(pver)
  integer, intent(in)                :: nPoints2D
  integer, intent(in)                :: myRank
  logical                            :: continueProcessing
  !
  ! Locals
  !

  write(*,'(a, i5.2, f5.2, i5.2)') "se_catalyst_create_grid: ", nstep, time, &
       myRank
  continueProcessing = .false.;
  if (se_requestdatadescription(nstep, time)) then
     continueProcessing = .true.
     if (se_needtocreategrid()) then
        call se_create_grid(ne, np, nlon, lonRad, nlat, latRad, &
             nlev, levCoord, nPoints2D, pcols, myRank)
     end if
  end if
end function se_catalyst_create_grid

!===========================================================================
! Add a chunk of data to the grid. Makes conversions from
! multi-dimensional arrays to uni-dimensional arrays so that the arrays are
! received correctly in C++
subroutine se_catalyst_add_chunk(nstep, time, phys_state, ncols)
  use ppgrid,       only: pver
  !-----------------------------------------------------------------------
  !
  ! Arguments
  !
  integer, intent(in)                :: nstep ! current timestep number
  real(kind=8),intent(in)            :: time  ! current time
  type(physics_state), intent(in)    :: phys_state
  integer, intent(in)                :: ncols
  !
  ! Locals
  !
  real(r8)                           :: psTransVar(1), tTransVar(1), &
       uTransVar(1), vTransVar(1)



  write(*, "(A, I4, A, I4,  A, I4, A, I4, A, I4)") &
       "se_catalyst_add_chunk: lchnk=", phys_state%lchnk, &
       " ncols=", ncols, &
       " size(lon)=", size(phys_state%lon), &
       " size(lat)=", size(phys_state%lat), &
       " size(ps)=",  size(phys_state%ps),  &
       " size(t,1)=",   size(phys_state%t, 1), &
       " size(t,2)=",   size(phys_state%t, 2)
  call se_add_chunk(nstep, ncols, &
       phys_state%lon, phys_state%lat, &
       transfer(phys_state%ps, psTransVar), &
       transfer(phys_state%t, tTransVar), &
       transfer(phys_state%u, uTransVar), &
       transfer(phys_state%v, vTransVar))
end subroutine se_catalyst_add_chunk

!===========================================================================
! Coprocesses data
subroutine se_catalyst_coprocess(nstep, time)
  use ppgrid,       only: pver
  !-----------------------------------------------------------------------
  !
  ! Arguments
  !
  integer, intent(in)                :: nstep ! current timestep number
  real(kind=8),intent(in)            :: time  ! current time
  !
  ! Locals
  !
  write(*,'(a, i5.2, f5.2)') "catalyst_coprocess: ", nstep, time
  call se_coprocess()
end subroutine se_catalyst_coprocess


! Creates the grid (if necessary), copies the data into Catalyst and
! calls coprocess
! This routine uses functions and variables from cam5 which is an executable,
! so this routine is included into a physpkg.F90, a file in cam5
subroutine catalyst_add_and_coprocess(phys_state)
  use time_manager, only: get_nstep, get_curr_time
  use physconst,     only: pi
  use dyn_grid,     only: get_horiz_grid_dim_d, get_horiz_grid_d, get_dyn_grid_parm_real1d, &
       get_dyn_grid_parm
  use dycore,       only: dycore_is
  use hycoef,       only: hyam, hybm, ps0
  use ppgrid,       only: pver
  ! for getting the MPI rank
  use cam_pio_utils, only: pio_subsystem

  ! Input/Output arguments
  !
  type(physics_state), intent(inout), dimension(begchunk:endchunk) :: phys_state
  !-----------------------------------------------------------------------
  !
  ! Locals
  !
  integer :: nstep          ! current timestep number
  real(kind=8) :: time      ! current time
  integer :: ndcur          ! day component of current time
  integer :: nscur          ! seconds component of current time
  integer, dimension(1:3) :: dim    ! lon, lat and lev
  real(r8), pointer :: latdeg(:)    ! degrees gaussian latitudes
  integer :: plon
  real(r8), allocatable :: alon(:)  ! longitude values (degrees)
  real(r8) :: alev(pver)    ! level values (pascals)
  integer :: i,f,c          ! indexes
  integer :: nPoints2D

  integer             :: dim1s,dim2s             ! global size of the first and second horizontal dim.
  integer             :: ncol
  real(r8), pointer   :: alat(:)                 ! latitude values (degrees)
  integer             :: ne, np                  ! SE grid parameters

  call t_startf ('catalyst_add_and_coprocess')

  ! current time step and time
  nstep = get_nstep()
  call get_curr_time(ndcur, nscur)
  time = ndcur + nscur/86400._r8

  if (dycore_is('LR') ) then
     ! FV dynamic core

     ! lon, lat
     call get_horiz_grid_dim_d(dim(1),dim(2))
     ! lev
     dim(3) = pver

     ! longitude
     plon = get_dyn_grid_parm('plon')
     allocate(alon(plon))
     do i=1,plon
        alon(i) = (i-1) * 360.0_r8 / plon
     end do

     ! latitude
     latdeg => get_dyn_grid_parm_real1d('latdeg')

     ! levels
     ! converts Pascals to millibars
     alev(:pver) = 0.01_r8*ps0*(hyam(:pver) + hybm(:pver))

     ! total number of points on a MPI node
     nPoints2D = 0
     do c=begchunk, endchunk
        nPoints2D = nPoints2D + get_ncols_p(c)
     end do
     if (fv_catalyst_create_grid(nstep, time, dim, alon, latdeg, alev, &
          nPoints2D, pio_subsystem%comp_rank)) then
        do c=begchunk, endchunk
           call fv_catalyst_add_chunk(nstep, time, phys_state(c), get_ncols_p(c))
        end do
        call fv_catalyst_coprocess(nstep, time)
     end if
     deallocate(alon)

  else if (dycore_is('UNSTRUCTURED')) then
     ! SE dynamic core
     call get_horiz_grid_dim_d(dim1s, dim2s)
     ncol = dim1s*dim2s

     ! read longitude and latitude
     allocate(alon(ncol))
     call get_horiz_grid_d(ncol, clon_d_out=alon)

     allocate(alat(ncol))
     call get_horiz_grid_d(ncol, clat_d_out=alat)

     ! levels
     ! converts Pascals to millibars
     alev(:pver) = 0.01_r8*ps0*(hyam(:pver) + hybm(:pver))

     ! total number of points on a MPI node
     nPoints2D = 0
     do c=begchunk, endchunk
        nPoints2D = nPoints2D + get_ncols_p(c)
     end do

     ! ne, np
     ne = get_dyn_grid_parm('ne')
     np = get_dyn_grid_parm('np')

     if (se_catalyst_create_grid(nstep, time, ne, np, ncol, alon, ncol, alat, pver, alev, &
          nPoints2D, pio_subsystem%comp_rank)) then
        do c=begchunk, endchunk
           call se_catalyst_add_chunk(nstep, time, phys_state(c), get_ncols_p(c))
        end do
        call se_catalyst_coprocess(nstep, time)
     end if
     deallocate(alon)
     deallocate(alat)
  endif

  call t_stopf ('catalyst_add_and_coprocess')
end subroutine catalyst_add_and_coprocess
