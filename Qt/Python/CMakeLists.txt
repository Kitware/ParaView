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
set(Module_SRCS
  pqPythonDebugLeaksView.cxx
  pqPythonDebugLeaksView.h
  pqPythonDialog.cxx
  pqPythonDialog.h
  pqPythonMacroSupervisor.cxx
  pqPythonMacroSupervisor.h
  pqPythonManager.cxx
  pqPythonManager.h
  pqPythonScriptEditor.cxx
  pqPythonScriptEditor.h
  pqPythonShell.cxx
  pqPythonShell.h
  pqPythonSyntaxHighlighter.cxx
  pqPythonSyntaxHighlighter.h
)

set (Module_MOC_HDRS
  pqPythonDebugLeaksView.h
  pqPythonDialog.h
  pqPythonMacroSupervisor.h
  pqPythonManager.h
  pqPythonScriptEditor.h
  pqPythonShell.h
  pqPythonSyntaxHighlighter.h
)

set (Module_UI_FILES
  pqPythonDialog.ui
)

find_package(PythonLibs REQUIRED)

include_directories(
  "${VTK_SOURCE_DIR}/Wrapping/Python"
  "${VTK_BINARY_DIR}/Wrapping/Python"
  ${PYTHON_INCLUDE_DIRS}
  )

#-------------------------------------------------------------------------
set(${vtk-module}_NO_HeaderTest 1)

include(ParaViewQt)
pv_find_package_qt(qt_private_targets REQUIRED
                QT4_COMPONENTS QtGui
                QT5_COMPONENTS Widgets)
pv_qt_wrap_cpp(__Module_MOC_SRCS ${Module_MOC_HDRS})
pv_qt_wrap_ui(__Module_UI_SRCS ${Module_UI_FILES})

set(${vtk-module}_EXPORT_CODE_INSTALL
  "include(\${ParaView_CMAKE_DIR}/ParaViewQt.cmake)
  pv_find_package_qt(__${vtk-module}_qt_targets REQUIRED QUIET
    QT4_COMPONENTS QtGui
    QT5_COMPONENTS Widgets)")
set(${vtk-module}_EXPORT_CODE_BUILD "${${vtk-module}_EXPORT_CODE_INSTALL}")

SOURCE_GROUP("Resources" FILES
  ${Module_UI_FILES}
  ${Module_RESOURCE_FILES}
)

SOURCE_GROUP("Generated" FILES
  ${__Module_MOC_SRCS}
  ${__Module_UI_SRCS}
)

vtk_module_library(${vtk-module}
  ${Module_SRCS}
  ${__Module_MOC_SRCS}
  ${__Module_UI_SRCS})
target_link_libraries(${vtk-module} LINK_PRIVATE ${qt_private_targets})

## Install
##--------------------------------------------------------------------------------------------------
## Libraries
#IF(NOT PV_INSTALL_NO_LIBRARIES)
#  SET (targets_to_install QtPython)
#  INSTALL(TARGETS ${targets_to_install}
#    EXPORT ${PV_INSTALL_EXPORT_NAME}
#    RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
#    LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
#    ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
#ENDIF()
## Development
#IF (NOT PV_INSTALL_NO_DEVELOPMENT)
#  # Headers
#  GLOB_INSTALL_DEVELOPMENT(
#      ${CMAKE_CURRENT_SOURCE_DIR}
#      ${PV_INSTALL_INCLUDE_DIR} 
#      "*.h;*.hxx;*.txx")
#  INSTALL(
#      FILES ${dynamicHeaders}
#      DESTINATION ${PV_INSTALL_INCLUDE_DIR}
#      COMPONENT Development)
#  # QT resources
#  GLOB_INSTALL_DEVELOPMENT( 
#    ${CMAKE_CURRENT_SOURCE_DIR}
#    ${PV_INSTALL_INCLUDE_DIR}/Qt/Python/Resources
#    "*.ui;*.ico;*.png;*.svg;*.xpm;*.rc;*.qrc")
#ENDIF ()
