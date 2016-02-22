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
set (Module_SRCS
  CAdaptorAPI.cxx
  vtkCPAdaptorAPI.cxx
  vtkCPCxxHelper.cxx
  vtkCPDataDescription.cxx
  vtkCPInputDataDescription.cxx
  vtkCPPipeline.cxx
  vtkCPProcessor.cxx
)

set_source_files_properties(
  vtkCPAdaptorAPI
  vtkCPPipeline
  ABSTRACT)

set_source_files_properties(
  CAdaptorAPI
  vtkCPCxxHelper
  WRAP_EXCLUDE)

set (${vtk-module}_HDRS CAdaptorAPI.h)

configure_file(vtkCPConfig.h.in
               vtkCPConfig.h @ONLY)

set (CATALYST_FORTRAN_USING_MANGLING)

if (CMAKE_Fortran_COMPILER_WORKS)
  include(FortranCInterface)
  FortranCInterface_HEADER(FortranAdaptorAPIMangling.h
    SYMBOLS
      coprocessorinitialize
      coprocessorfinalize
      requestdatadescription
      needtocreategrid
      coprocess)

  set(CATALYST_FORTRAN_USING_MANGLING ${FortranCInterface_GLOBAL_FOUND})

  set (Module_SRCS
      ${Module_SRCS}
      FortranAdaptorAPI.cxx)

  set (${vtk-module}_HDRS
       ${${vtk-module}_HDRS}
       FortranAdaptorAPI.h
       ${CMAKE_CURRENT_BINARY_DIR}/FortranAdaptorAPIMangling.h)

  set_source_files_properties(
    FortranAdaptorAPI
    WRAP_EXCLUDE)

  set_source_files_properties(
    FortranAdaptorAPI
    PROPERTIES
    WRAP_EXCLUDE_PYTHON 1)
endif()

if (PARAVIEW_USE_MPI)
  # this needs to be done before the vtk_module_library() since
  # vtkMPI has add_definitions() call it in.
  include(vtkMPI)
endif()

vtk_module_library(${vtk-module} ${Module_SRCS})
if(PARAVIEW_USE_MPI)
  vtk_mpi_link(${vtk-module})
endif()
if (CATALYST_FORTRAN_USING_MANGLING)
  set_property(TARGET ${vtk-module} APPEND PROPERTY COMPILE_DEFINITIONS
      "CATALYST_FORTRAN_USING_MANGLING=1")
endif()
