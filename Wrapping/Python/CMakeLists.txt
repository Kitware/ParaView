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
set(_vtkpy_modules
  # Ninja BUG 760: In past this depended on vtkpython_pyc. However, due to Ninja
  # bug, the dependency wasn't being setup properly. Hence we directly depend on
  # the generated file. Once Ninja or Cmake is fixed, we can remove this file
  # depedency and leave the target dependecy.
  ${CMAKE_BINARY_DIR}/VTK/Wrapping/Python/vtk_compile_complete
  vtkpython_pyc
  )
if (TARGET vtkWebPythonCommon)
  list(APPEND _vtkpy_modules vtkWebPythonCommon)
endif()

#------------------------------------------------------------------------------
# List ParaView *.py files.
set(PV_PYTHON_SOURCE_FILES
  paraview/_backwardscompatibilityhelper.py
  paraview/annotation.py
  paraview/benchmark/__init__.py
  paraview/benchmark/logbase.py
  paraview/benchmark/logparser.py
  paraview/benchmark/manyspheres.py
  paraview/benchmark/basic.py
  paraview/calculator.py
  paraview/collaboration.py
  paraview/coprocessing.py
  paraview/cpstate.py
  paraview/demos/demo1.py
  paraview/demos/show_grid_as_background.py
  paraview/extract_selection.py
  paraview/__init__.py
  paraview/lookuptable.py
  paraview/numeric.py
  paraview/pvfilters.py
  paraview/pvvtkextensions.py
  paraview/python_view.py
  paraview/servermanager.py
  paraview/simple.py
  paraview/smstate.py
  paraview/smtesting.py
  paraview/smtrace.py
  paraview/spatiotemporalparallelism.py
  paraview/util.py
  paraview/variant.py
  paraview/vtk/__init__.py
  )

# This is odd; we shouldnt' need to configure cpexport.py, we can simply fill in
# the version number when cpexport.py is used by the ParaView client.
# Leaving this unchanged for now since don't want to make too many unrelated changes
# here.
configure_file(cpexport.py.in cpexport.py @ONLY)

#------------------------------------------------------------------------------
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pv_copy_and_compile_py_files_complete"

  # We will presently skip removing old py files since the code to put together
  # *.py files for the `paraview` package is scattered around e.g. under the Web module.
  # The responsibility to put together the `paraview` Python package should really be in one place
  # (which is here). Let's do that in near future. Until then, let's leave these "empty dir" lines
  # commented out.
  # # Empty '$pydir/paraview' to remove old files.
  # COMMAND ${CMAKE_COMMAND} ARGS -E echo "emptying '.../site-packages/paraview'"
  # COMMAND ${CMAKE_COMMAND} ARGS -E remove_directory "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview"
  # COMMAND ${CMAKE_COMMAND} ARGS -E echo "emptying '.../site-packages/vtk'"
  # COMMAND ${CMAKE_COMMAND} ARGS -E remove_directory "${VTK_BUILD_PYTHON_MODULE_DIR}/vtk"

  COMMAND ${CMAKE_COMMAND} ARGS -E echo "copying paraview/*.py to '.../site-paraview/paraview'"
  COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/paraview"
          "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview"

  COMMAND ${CMAKE_COMMAND} ARGS -E copy
          "${CMAKE_CURRENT_BINARY_DIR}/cpexport.py"
          "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview/cpexport.py"

  # Copy into $pydir/paraview/vtk.
  # This is what scripts get when they do "import paraview.vtk".
  COMMAND ${CMAKE_COMMAND} ARGS -E echo "copying VTKs py files to '.../site-packages/paraview/vtk'"
  COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory
          "${VTK_BINARY_DIR}/Wrapping/Python/vtk"
          "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview/vtk"

  # Overlay that with paraview's customizations.
  COMMAND ${CMAKE_COMMAND} ARGS -E echo "copy minimized __init__.py to '.../site-packages/paraview/vtk'"
  COMMAND ${CMAKE_COMMAND} ARGS -E copy
          "${CMAKE_CURRENT_SOURCE_DIR}/paraview/vtk/__init__.py"
          "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview/vtk/__init__.py"

  # Also copy into $pydir/vtk.
  # ParaView (pvpython/pvbatch) scripts that want all of vtk can now "import vtk".
  COMMAND ${CMAKE_COMMAND} ARGS -E echo "also copying VTKs py to .../site-packages/vtk"
  COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory
          "${VTK_BINARY_DIR}/Wrapping/Python/vtk"
          "${VTK_BUILD_PYTHON_MODULE_DIR}/vtk"

  # Duplicate a few files from vtk (backwards compatibility)
  COMMAND ${CMAKE_COMMAND} -E copy
          "${VTK_SOURCE_DIR}/Wrapping/Python/vtk/util/numpy_support.py"
          "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview/numpy_support.py"

  COMMAND ${CMAKE_COMMAND} -E copy
          "${VTK_SOURCE_DIR}/Wrapping/Python/vtk/util/vtkConstants.py"
          "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview/vtkConstants.py"

  # Now compile the python module files.
  COMMAND ${PYTHON_EXECUTABLE} -m compileall -q
          "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview"
          "${VTK_BUILD_PYTHON_MODULE_DIR}/vtk"

  # Copy ColorMaps.xml (obsolete: we should remove this soonish)
  COMMAND ${CMAKE_COMMAND} ARGS -E copy
          "${CMAKE_CURRENT_SOURCE_DIR}/ColorMaps.xml"
          "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview/ColorMaps.xml"

  # Touch the OUTPUT file, otherwise this command will always be dirty.
  COMMAND ${CMAKE_COMMAND} ARGS -E touch "${CMAKE_CURRENT_BINARY_DIR}/pv_copy_and_compile_py_files_complete"

  DEPENDS
    ${_vtkpy_modules}
    "${CMAKE_CURRENT_BINARY_DIR}/cpexport.py"
    "${VTK_BINARY_DIR}/Wrapping/Python/vtk/__init__.py"
    "${VTK_SOURCE_DIR}/Wrapping/Python/vtk/util/numpy_support.py"
    "${VTK_SOURCE_DIR}/Wrapping/Python/vtk/util/vtkConstants.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/ColorMaps.xml"
    ${PV_PYTHON_SOURCE_FILES}
)

add_custom_target(paraview_pyc ALL
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/pv_copy_and_compile_py_files_complete")

# Install the paraview module files.
# XXX(uda): why are we installing all packages in site-packages? We should only install
# ones we created here i.e. paraview and vtk.
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages
    DESTINATION ${VTK_INSTALL_LIBRARY_DIR}
    COMPONENT Runtime
    PATTERN *-complete EXCLUDE
    PATTERN *_complete EXCLUDE)
