! SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
! SPDX-License-Identifier: BSD-3-Clause
!
! A Fortran Catalyst2 example.
! Adapted from Examples/Catalyst/Fortran90FullExample
! at commit 7da5c2dbb746524f1d575b2ec62ec0eab69fc28f

PROGRAM fedriver
  use catalyst_adaptor
  implicit none
  include 'mpif.h'

  integer(4), parameter :: nx = 100, ny = 100, nz = 100, ntime = 10
  integer(4) :: i, j, k, time, nxstart, nxend
  real :: max
  complex(kind=8), allocatable :: psi01(:, :, :)
  integer :: numtasks, rank, ierr

  call mpi_init(ierr)
  if (ierr /= MPI_SUCCESS) then
    write (stderr, *) "ERROR: Initializing MPI: ", ierr
  end if

  call mpi_comm_size(MPI_COMM_WORLD, numtasks, ierr)
  call mpi_comm_rank(MPI_COMM_WORLD, rank, ierr)

  call catalyst_adaptor_initialize()

  ! partition in the x-direction only
  nxstart = rank*nx/numtasks + 1
  nxend = (rank + 1)*nx/numtasks
  if (numtasks .ne. rank + 1) then
    nxend = nxend + 1
  end if
  !write(*,* ) "rank",rank,"numtasks",numtasks," nxstart",nxstart, "nxend",nxend

  allocate (psi01(nxend - nxstart + 1, ny, nz))
  ! set initial values
  max = sqrt(real(nx)**2 + real(ny)**2 + real(nz)**2)
  do k = 1, nz
    do j = 1, ny
      do i = 1, nxend - nxstart + 1
        psi01(i, j, k) = CMPLX(max - sqrt(real(i + nxstart - 50)**2 + real(j - 50)**2 + real(k - 50)**2), &
                               real(1 + j + k))/max*100.0
      end do
    end do
  end do

  ! Execute the catalyst script for every timestep
  do time = 1, ntime
    do k = 1, nz
      do j = 1, ny
        do i = 1, nxend - nxstart + 1
          psi01(i, j, k) = CMPLX(real(0.30), 0.0) + psi01(i, j, k)
        end do
      end do
    end do
    call catalyst_adaptor_execute(nxstart, nxend, nx, ny, nz, time, dble(time), psi01)
  end do
  deallocate (psi01)

  call catalyst_adaptor_finalize()
  call mpi_finalize(ierr)

  if (ierr /= MPI_SUCCESS) then
    write (stderr, *) "ERROR: Finalizing MPI: ", ierr
  end if

end program fedriver
