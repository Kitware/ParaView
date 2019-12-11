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

# FIXME: export symbols on non-Windows as well.
set(CMAKE_CXX_VISIBILITY_PRESET "default")
set(CMAKE_VISIBILITY_INLINES_HIDDEN 0)

# ParaView can no longer support QT_TESTING_WITH_PYTHON due to Matplotlib
# initializing Python.
set(QT_TESTING_WITH_PYTHON FALSE)

set(BUILD_EXAMPLES OFF)
set(QtTesting_QT_VERSION 5)
set(QtTesting_INSTALL_BIN_DIR "${_vtk_build_RUNTIME_DESTINATION}")
set(QtTesting_INSTALL_INCLUDE_DIR "${_vtk_build_HEADERS_DESTINATION}/vtkqttesting")
set(QtTesting_INSTALL_LIB_DIR "${_vtk_build_LIBRARY_DESTINATION}")
set(QtTesting_INSTALL_CMAKE_DIR "${_vtk_build_CMAKE_DESTINATION}")
set(QtTesting_INSTALL_NO_DEVELOPMENT TRUE)
set(QT_TESTING_EVENT_PLAYBACK_DELAY 100)
set(QtTesting_ENABLE_EXAMPLES OFF)
set(QtTesting_ENABLE_TESTING OFF)
set(QT_TESTING_INSTALL_EXPORT_NAME "ParaView")
set(BUILD_TESTING OFF)

vtk_module_third_party_internal(
  LICENSE_FILES "${PROJECT_SOURCE_DIR}/License_v1.2.txt"
  VERSION       "master")
