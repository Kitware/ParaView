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
  CPythonAdaptorAPI.cxx
  vtkCPPythonAdaptorAPI.cxx
  vtkCPPythonScriptPipeline.cxx
)

set_source_files_properties(
  vtkCPPythonAdaptorAPI
  ABSTRACT)

set (${vtk-module}_HDRS CPythonAdaptorAPI.h)

set (CATALYST_FORTRAN_USING_MANGLING)
if (CMAKE_Fortran_COMPILER_WORKS)
  include(FortranCInterface)
  FortranCInterface_HEADER(FortranPythonAdaptorAPIMangling.h
    SYMBOLS coprocessorinitializewithpython coprocessoraddpythonscript)
  set(CATALYST_FORTRAN_USING_MANGLING ${FortranCInterface_GLOBAL_FOUND})
  set (Module_SRCS
      ${Module_SRCS}
      FortranPythonAdaptorAPI.cxx)

  set (${vtk-module}_HDRS
       ${${vtk-module}_HDRS}
       FortranPythonAdaptorAPI.h
       ${CMAKE_CURRENT_BINARY_DIR}/FortranPythonAdaptorAPIMangling.h)
endif()

vtk_module_library(${vtk-module} ${Module_SRCS})
target_link_libraries(${vtk-module} LINK_PRIVATE vtkUtilitiesPythonInitializer)

if (CATALYST_FORTRAN_USING_MANGLING)
  set_property(TARGET ${vtk-module} APPEND PROPERTY COMPILE_DEFINITIONS
      "CATALYST_FORTRAN_USING_MANGLING=1")
endif()
