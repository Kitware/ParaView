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

# Handle out-of-source builds correctly.
#
#  1. Create a list of Python files to be installed/copied.
#  2. Copy them to the binary directory. Since paraview cannot be built
#     in place, we must always copy the files to the binary directory.
#  3. Use Python's compileall to compile the copied files.
#
# *** Step 1 has to be done carefully to avoid missing out files ***

# Copy generated module files from VTK.
# To this first so we can replace some of the VTK files by ParaView
# customizations.
if (VTK_BINARY_DIR)
  file (COPY "${VTK_BINARY_DIR}/Wrapping/Python/vtk"
        DESTINATION "${PV_PYTHON_MODULE_BINARY_DIR}"
        USE_SOURCE_PERMISSIONS)
endif()

# This will replace the "vtk/__init__.py" file as well as add a few extra files
# to the "vtk" module.
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/paraview
      DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages
      USE_SOURCE_PERMISSIONS
      FILES_MATCHING
      PATTERN *.py)

# Add a couple of vtk python files to "paraview" module.
file (COPY "${ParaView_SOURCE_DIR}/VTK/Wrapping/Python/vtk/util/numpy_support.py"
           "${ParaView_SOURCE_DIR}/VTK/Wrapping/Python/vtk/util/vtkConstants.py"
      DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages/paraview
      USE_SOURCE_PERMISSIONS)


# Byte compile the Python files.
CONFIGURE_FILE(${PV_PYTHON_MODULE_SOURCE_DIR}/compile_all_pv.py.in
  ${PV_PYTHON_MODULE_BINARY_DIR}/compile_all_pv.py
  @ONLY IMMEDIATE)

ADD_CUSTOM_COMMAND(
  WORKING_DIRECTORY ${PV_PYTHON_MODULE_BINARY_DIR}
  COMMAND ${PYTHON_EXECUTABLE}
  ARGS compile_all_pv.py
  DEPENDS ${PV_PYTHON_SOURCE_FILES} ${PV_PYTHON_MODULE_BINARY_DIR}/compile_all_pv.py
  ${PV_PYTHON_OUTPUT_FILES}
  OUTPUT "${PV_PYTHON_MODULE_BINARY_DIR}/pv_compile_complete"
  )

ADD_CUSTOM_TARGET(paraview_pyc ALL 
  DEPENDS "${PV_PYTHON_MODULE_BINARY_DIR}/pv_compile_complete")

# Install the paraview module files.
INSTALL(DIRECTORY ${PV_PYTHON_MODULE_BINARY_DIR}
    DESTINATION ${VTK_INSTALL_LIBRARY_DIR}/site-packages
    COMPONENT Runtime)
