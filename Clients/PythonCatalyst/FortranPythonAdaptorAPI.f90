! =========================================================================
!
!   Program:   ParaView
!   Module:    FortranPythonAdaptorAPI.h
!
!   Copyright (c) Kitware, Inc.
!   All rights reserved.
!   See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
!
!      This software is distributed WITHOUT ANY WARRANTY; without even
!      the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
!      PURPOSE.  See the above copyright notice for more information.
!
! =========================================================================

module catalyst_python
    implicit none
    public catalyst_initialize_with_python, catalyst_add_python_script

interface
    subroutine coprocessorinitializewithpython(file_name, length) bind(C, name='coprocessorinitializewithpython')
        use, intrinsic :: iso_c_binding, only : c_char, c_int

        implicit none

        character(kind=c_char) :: file_name(*)
        integer(c_int) :: length
    end subroutine coprocessorinitializewithpython

    subroutine coprocessoraddpythonscript(file_name, length) bind(C, name='coprocessoraddpythonscript')
        use, intrinsic :: iso_c_binding, only : c_char, c_int

        implicit none

        character(kind=c_char) :: file_name(*)
        integer(c_int) :: length
    end subroutine coprocessoraddpythonscript
end interface

contains

subroutine catalyst_initialize_with_python(file_name)
    use, intrinsic :: iso_c_binding, only : c_int

    implicit none

    character(len = *), target :: file_name

    integer(c_int) :: c_length

    c_length = len(file_name)

    call coprocessorinitializewithpython(file_name, c_length)
end subroutine catalyst_initialize_with_python

subroutine catalyst_add_python_script(file_name)
    use, intrinsic :: iso_c_binding, only : c_int

    implicit none

    character(len = *), target :: file_name

    integer(c_int) :: c_length

    c_length = len(file_name)

    call coprocessoraddpythonscript(file_name, c_length)
end subroutine catalyst_add_python_script

end module catalyst_python
