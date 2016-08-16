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
# ParaView can no longer support QT_TESTING_WITH_PYTHON due to Matplotlib
# initializing Python.
set (QT_TESTING_WITH_PYTHON FALSE)
set (QtTesting_INSTALL_BIN_DIR ${VTK_INSTALL_RUNTIME_DIR})
set (QtTesting_INSTALL_LIB_DIR ${VTK_INSTALL_LIBRARY_DIR})
set (QtTesting_INSTALL_LIBRARY_DIR ${VTK_INSTALL_LIBRARY_DIR})
set (QT_TESTING_INSTALL_EXPORT_NAME ${VTK_INSTALL_EXPORT_NAME})
set (QtTesting_INSTALL_INCLUDE_DIR ${VTK_INSTALL_INCLUDE_DIR})
set (QtTesting_INSTALL_NO_DEVELOPMENT ${VTK_INSTALL_NO_DEVELOPMENT})
set (QtTesting_QT_VERSION ${PARAVIEW_QT_VERSION})
##########################################################################
# import Qt build settings
vtk_module_third_party(qttesting
  INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkqttesting
    ${CMAKE_CURRENT_BINARY_DIR}/vtkqttesting
  LIBRARIES QtTesting
)
vtk_target_export(QtTesting)
