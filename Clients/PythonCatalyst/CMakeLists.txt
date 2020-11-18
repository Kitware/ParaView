#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================
set(classes
  CPythonAdaptorAPI
  vtkCPPythonAdaptorAPI
  vtkCPPythonPipeline
  vtkCPPythonScriptPipeline
  vtkCPPythonScriptV2Helper
  vtkCPPythonScriptV2Pipeline
  vtkCPPythonStringPipeline)

set(catalyst_fortran_using_mangling)

if (CMAKE_Fortran_COMPILER_WORKS)
  include(FortranCInterface)
  FortranCInterface_HEADER(FortranPythonAdaptorAPIMangling.h
    SYMBOLS
      coprocessorinitializewithpython
      coprocessoraddpythonscript)

  if (FortranCInterface_GLOBAL_PREFIX OR
      FortranCInterface_GLOBAL_SUFFIX OR
      (NOT FortranCInterface_GLOBAL_CASE STREQUAL "LOWER"))
    set(catalyst_fortran_using_mangling "${FortranCInterface_GLOBAL_FOUND}")
  else()
    # otherwise there's no mangling necessary for Fortran/C interchange and
    # hence we leave catalyst_fortran_using_mangling unset.
  endif()

  list(APPEND classes
    FortranPythonAdaptorAPI)
  list(APPEND headers
    "${CMAKE_CURRENT_BINARY_DIR}/FortranPythonAdaptorAPIMangling.h")
endif ()

vtk_module_add_module(ParaView::PythonCatalyst
  CLASSES ${classes}
  HEADERS ${headers})
vtk_module_client_server_exclude()

if (catalyst_fortran_using_mangling)
  vtk_module_definitions(ParaView::PythonCatalyst
    PRIVATE
      CATALYST_FORTRAN_USING_MANGLING=1)
endif ()
