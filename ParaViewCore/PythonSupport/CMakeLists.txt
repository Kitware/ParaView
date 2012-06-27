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
  vtkPVPythonInteractiveInterpretor.cxx
  vtkPVPythonInteractiveInterpretor.h
  vtkPVPythonInterpretor.cxx
  vtkPVPythonInterpretor.h
  vtkPVPythonInterpretorWrapper.h
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkPythonAppInitConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPythonAppInitConfigure.h
  @ONLY)

include_directories(
  "${VTK_SOURCE_DIR}/Wrapping/PythonCore"
  "${VTK_BINARY_DIR}/Wrapping/PythonCore"
  "${PYTHON_INCLUDE_PATH}"
  )

vtk_module_library(vtkPVPythonSupport ${Module_SRCS})
target_link_libraries(vtkPVPythonSupport
  vtkPythonCore)

# When paraview is linked statically (for example during
# cross-compiling) the python wrapper module "init" functions must be
# referenced and called at startup.  The necessary code is written
# into pvpythonmodules.h and included in vtkPVPythonInterpretor.cxx
# for compilation.  Then the vtkPVPythonInterpretor static library
# "links" to all the module archives.  This creates a circular
# dependency chain between the interpreter and the wrapper modules.
#
# In the future we should change this to have a library whose purpose
# is to reference all the module init functions.  That library may
# then depend on the modules and vtkPVPythonInterpretor without
# creating a cycle.
set(PYTHON_MODULE_PREFIX "")
PYTHON_WRITE_MODULES_HEADER(${CMAKE_CURRENT_BINARY_DIR}/pvpythonmodules.h)
# The PYTHON_ADD_MODULE macro stores the list of static modules in a global property
GET_PROPERTY(PY_STATIC_MODULES_LIST GLOBAL PROPERTY PY_STATIC_MODULES_LIST)
if(PY_STATIC_MODULES_LIST)
  target_link_libraries(vtkPVPythonSupport ${PY_STATIC_MODULES_LIST})
endif(PY_STATIC_MODULES_LIST)
