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

# This file manages the building/installing of ParaView's python modules.
# Wrapping of classes is taken care of by VTK itself.
if (NOT PARAVIEW_ENABLE_PYTHON)
  return()
endif()

# Copy generated module files from VTK into place in the build tree
set(_vtkpy_modules vtk_python_package)
if (TARGET vtkWebPythonCommon)
  list(APPEND _vtkpy_modules vtkWebPythonCommon)
endif()

#------------------------------------------------------------------------------
# Build paraview package.
vtk_python_package(pvpython_pyc paraview DEPENDS ${_vtkpy_modules})

#------------------------------------------------------------------------------
# This crazy hack is needed to copy `vtk` package under `paraview` package while replacing the
# `paraview/vtk/__init__.py` to avoid importing all vtk modules.
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pv_copy_and_compile_py_files_complete"

  COMMAND ${CMAKE_COMMAND} ARGS -E echo "emptying '.../site-packages/paraview/vtk'"
  COMMAND ${CMAKE_COMMAND} ARGS -E remove_directory "${VTK_BUILD_PYTHON_MODULES_DIR}/paraview/vtk"

  # Copy into .../paraview/vtk.
  # This is what scripts get when they do "import paraview.vtk".
  COMMAND ${CMAKE_COMMAND} ARGS -E echo "copying VTKs py files to '.../site-packages/paraview/vtk'"
  COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory
          "${VTK_BUILD_PYTHON_MODULES_DIR}/vtk"
          "${VTK_BUILD_PYTHON_MODULES_DIR}/paraview/vtk"

  # Overlay that with paraview's customizations.
  COMMAND ${CMAKE_COMMAND} ARGS -E echo "copy minimized __init__.py to '.../site-packages/paraview/vtk'"
  COMMAND ${CMAKE_COMMAND} ARGS -E copy
          "${CMAKE_CURRENT_SOURCE_DIR}/paraview/vtk/__init__.py"
          "${VTK_BUILD_PYTHON_MODULES_DIR}/paraview/vtk/__init__.py"

  # Copy ColorMaps.xml (obsolete: we should remove this soonish)
  COMMAND ${CMAKE_COMMAND} ARGS -E copy
          "${CMAKE_CURRENT_SOURCE_DIR}/ColorMaps.xml"
          "${VTK_BUILD_PYTHON_MODULES_DIR}/paraview/ColorMaps.xml"

  # Touch the OUTPUT file, otherwise this command will always be dirty.
  COMMAND ${CMAKE_COMMAND} ARGS -E touch "${CMAKE_CURRENT_BINARY_DIR}/pv_copy_and_compile_py_files_complete"

  DEPENDS pvpython_pyc
)

add_custom_target(paraview_python_package
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/pv_copy_and_compile_py_files_complete")

# Install the paraview module files.
# XXX(uda): why are we installing all packages in site-packages? We should only install
# ones we created here i.e. paraview and vtk.
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages
    DESTINATION ${VTK_INSTALL_LIBRARY_DIR}
    COMPONENT Runtime
    PATTERN *-complete EXCLUDE
    PATTERN *_complete EXCLUDE)
