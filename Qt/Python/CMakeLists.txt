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
set(classes
  pqLinkedObjectPythonTextArea
  pqPythonDebugLeaksView
  pqPythonEditorActions
  pqPythonFileIO
  pqPythonLineNumberArea
  pqPythonMacroSupervisor
  pqPythonManager
  pqPythonScriptEditor
  pqPythonShell
  pqPythonShellCompleter
  pqPythonSyntaxHighlighter
  pqPythonTabWidget
  pqPythonTextArea
  pqPythonUndoCommand
)

set(headers
  pqPythonUtils.h
  pqTextLinkerPython.h
)

set(ui_files
  pqPythonShell.ui)

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTOUIC 1)

vtk_module_find_package(
  PACKAGE     Qt${PARAVIEW_QT_MAJOR_VERSION}
  COMPONENTS  Widgets)

source_group("Resources" FILES
  ${ui_files})

set(translatable_files)
if (PARAVIEW_BUILD_TRANSLATIONS)
  list(APPEND translatable_files ${ui_files})
  foreach(el IN LISTS classes)
    list(APPEND translatable_files "${el}.cxx")
    list(APPEND translatable_files "${el}.h")
  endforeach()

    paraview_create_translation(
      TARGET "Python.ts"
      FILES ${translatable_files}
      OUTPUT_TS "${PARAVIEW_TRANSLATIONS_DIRECTORY}/Qt_Python.ts")
  add_dependencies(localization "Python.ts")
  endif ()

vtk_module_add_module(ParaView::pqPython
  CLASSES ${classes}
  SOURCES ${ui_files}
          ${resource_files}
  HEADERS ${headers})

vtk_module_link(ParaView::pqPython
  PRIVATE
    Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets)
vtk_module_definitions(ParaView::pqPython PRIVATE QT_NO_KEYWORDS)
