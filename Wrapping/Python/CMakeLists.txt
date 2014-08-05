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

set(PV_PYTHON_MODULE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/paraview")
set(PV_PYTHON_MODULE_BINARY_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages/paraview")


# Copy generated module files from VTK into place in the build tree
set(_vtkpy_modules
  # Ninja BUG 760: In past this depended on vtkpython_pyc. However, due to Ninja
  # bug, the dependency wasn't being setup properly. Hence we directly depend on
  # the generated file. Once Ninja or Cmake is fixed, we can remove this file
  # depedency and leave the target dependecy.
  ${CMAKE_BINARY_DIR}/VTK/Wrapping/Python/vtk_compile_complete
  vtkpython_pyc
  )
if (TARGET vtkWebPython)
  list(APPEND _vtkpy_modules vtkWebPython)
endif()
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/vtk_py_copy_completed"
  # Copy into $pydir/paraview/vtk.
  # This is what scripts get when they do "import paraview.vtk".
  COMMAND ${CMAKE_COMMAND} ARGS -E echo "copying to ${PV_PYTHON_MODULE_BINARY_DIR}"
  COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory
          "${VTK_BINARY_DIR}/Wrapping/Python/vtk"
          "${PV_PYTHON_MODULE_BINARY_DIR}/vtk"
  # Overlay that with paraview's customizations.
  COMMAND ${CMAKE_COMMAND} ARGS -E echo "overlaying onto ${PV_PYTHON_MODULE_BINARY_DIR}"
  COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/paraview/vtk"
          "${PV_PYTHON_MODULE_BINARY_DIR}/vtk"

  # Also copy into $pydir/vtk.
  # Scripts that want all of vtk can now "import vtk".
  COMMAND ${CMAKE_COMMAND} ARGS -E echo "copying to ${VTK_BUILD_PYTHON_MODULE_DIR}/vtk"
  COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory
          "${VTK_BINARY_DIR}/Wrapping/Python/vtk"
          "${VTK_BUILD_PYTHON_MODULE_DIR}/vtk"

  # Touch the OUTPUT file, otherwise this command will always be dirty.
  COMMAND ${CMAKE_COMMAND} ARGS -E touch "${CMAKE_CURRENT_BINARY_DIR}/vtk_py_copy_completed"

  DEPENDS ${_vtkpy_modules}
)

add_custom_target(copy_vtk_py_files ALL
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/vtk_py_copy_completed"
)

# Copy ParaView specific python files
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/paraview
      DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages
      USE_SOURCE_PERMISSIONS
      FILES_MATCHING
      PATTERN *.py)

# Add a couple of vtk python files to "paraview" module.
file (COPY "${ParaView_SOURCE_DIR}/VTK/Wrapping/Python/vtk/util/numpy_support.py"
           "${ParaView_SOURCE_DIR}/VTK/Wrapping/Python/vtk/util/vtkConstants.py"
      DESTINATION ${PV_PYTHON_MODULE_BINARY_DIR}
      USE_SOURCE_PERMISSIONS)

# copy ParaView's default lookup tables
file(COPY "${CMAKE_SOURCE_DIR}/Qt/Components/Resources/XML/ColorMaps.xml"
  DESTINATION "${PV_PYTHON_MODULE_BINARY_DIR}"
  USE_SOURCE_PERMISSIONS)

# Byte compile the paraview Python files.
configure_file(${PV_PYTHON_MODULE_SOURCE_DIR}/compile_all_pv.py.in
  ${PV_PYTHON_MODULE_BINARY_DIR}/compile_all_pv.py
  @ONLY IMMEDIATE)

configure_file(${PV_PYTHON_MODULE_SOURCE_DIR}/cpexport.py.in
  ${PV_PYTHON_MODULE_BINARY_DIR}/cpexport.py
  @ONLY IMMEDIATE)



add_custom_command(
  WORKING_DIRECTORY ${PV_PYTHON_MODULE_BINARY_DIR}
  COMMAND ${PYTHON_EXECUTABLE}
  ARGS compile_all_pv.py
  DEPENDS ${PV_PYTHON_SOURCE_FILES} ${PV_PYTHON_MODULE_BINARY_DIR}/compile_all_pv.py copy_vtk_py_files
  ${PV_PYTHON_OUTPUT_FILES}
  OUTPUT "${PV_PYTHON_MODULE_BINARY_DIR}/pv_compile_complete"
  )

add_custom_target(paraview_pyc ALL
  DEPENDS copy_vtk_py_files "${PV_PYTHON_MODULE_BINARY_DIR}/pv_compile_complete")

# Install the paraview module files.
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages
    DESTINATION ${VTK_INSTALL_LIBRARY_DIR}
    COMPONENT Runtime
    PATTERN *-complete EXCLUDE
    PATTERN *_complete EXCLUDE)
