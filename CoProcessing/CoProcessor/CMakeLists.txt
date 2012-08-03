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
set (Modules_SRCS
  vtkCPCxxHelper.cxx
  vtkCPProcessor.cxx
  CPSystemInformation.h
)

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND Modules_SRCS
    vtkCPPythonHelper.cxx
    vtkCPPythonScriptPipeline.cxx)

  include_directories(
  "${VTK_SOURCE_DIR}/Wrapping/Python"
  "${VTK_BINARY_DIR}/Wrapping/Python")
  set(VTK_WRAP_PYTHON_FIND_LIBS ON)
  include (vtkWrapPython)
  include_directories("${PYTHON_INCLUDE_PATH}")
endif (PARAVIEW_ENABLE_PYTHON)

if(PARAVIEW_USE_MPI)
  set(COPROCESSOR_USE_MPI 1)
endif(PARAVIEW_USE_MPI)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CPSystemInformation.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CPSystemInformation.h"
    @ONLY IMMEDIATE)

# disable header testing for this module.
set (${vtk-module}_NO_HeaderTest TRUE)

vtk_module_library(${vtk-module}
  ${Modules_SRCS})

if (PARAVIEW_ENABLE_PYTHON)
  SET(PYTHON_MODULE_PREFIX "")
  vtk_write_python_modules_header_for_wrapped_modules(
    ${CMAKE_CURRENT_BINARY_DIR}/cppythonmodules.h
    static_python_modules_list)
  target_link_libraries(${vtk-module} ${static_python_modules_list})
endif()

if (BUILD_TESTING)
  add_subdirectory(Testing/TestDriver)
endif ()
