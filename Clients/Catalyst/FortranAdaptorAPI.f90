! SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
! SPDX-License-Identifier: BSD-3-Clause

module catalyst
    implicit none
    public catalyst_initialize, catalyst_finalize, catalyst_request_data_description, catalyst_need_to_create_grid, catalyst_process

interface
    subroutine coprocessorinitialize() bind(C, name='coprocessorinitialize')
        implicit none
    end subroutine coprocessorinitialize

    subroutine coprocessorfinalize() bind(C, name='coprocessorfinalize')
        implicit none
    end subroutine coprocessorfinalize

    subroutine requestdatadescription(time_step, time, need_to_coprocess_this_time_step) bind(C, name='requestdatadescription')
        use, intrinsic :: iso_c_binding, only : c_int, c_double

        implicit none

        integer(c_int) :: time_step
        real(c_double) :: time
        integer(c_int) :: need_to_coprocess_this_time_step
    end subroutine requestdatadescription

    subroutine needtocreategrid(need_grid) bind(C, name='needtocreategrid')
        use, intrinsic :: iso_c_binding, only : c_int

        implicit none

        integer(c_int) :: need_grid
    end subroutine needtocreategrid

    subroutine coprocess() bind(C, name='coprocess')
        implicit none
    end subroutine coprocess
end interface

contains

subroutine catalyst_initialize()
    implicit none
    call coprocessorinitialize()
end subroutine catalyst_initialize

subroutine catalyst_finalize()
    implicit none
    call coprocessorfinalize()
end subroutine catalyst_finalize

logical(1) function catalyst_request_data_description(time_step, time)
    use, intrinsic :: iso_c_binding, only : c_int, c_double

    implicit none

    integer, intent(in) :: time_step
    real(8), intent(in) :: time

    integer(c_int) :: c_time_step
    real(c_double) :: c_time
    integer(c_int) :: c_need_to_coprocess_this_time_step

    c_time_step = time_step
    c_time = time
    c_need_to_coprocess_this_time_step = 0

    call requestdatadescription(c_time_step, c_time, c_need_to_coprocess_this_time_step)

    if (c_need_to_coprocess_this_time_step == 0) then
        catalyst_request_data_description = .FALSE.
    else
        catalyst_request_data_description = .TRUE.
    end if
end function catalyst_request_data_description

logical(1) function catalyst_need_to_create_grid()
    use, intrinsic :: iso_c_binding, only : c_int

    implicit none

    integer(c_int) :: c_need_to_create_grid

    c_need_to_create_grid = 0

    call needtocreategrid(c_need_to_create_grid)

    if (c_need_to_create_grid == 0) then
        catalyst_need_to_create_grid = .FALSE.
    else
        catalyst_need_to_create_grid = .TRUE.
    end if
end function catalyst_need_to_create_grid

subroutine catalyst_process()
    implicit none
    call coprocess()
end subroutine catalyst_process

end module catalyst
