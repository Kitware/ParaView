# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  pqLinkedObjectPythonTextArea
  pqPythonCalculatorCompleter
  pqPythonCompleter
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
  PACKAGE     "Qt${PARAVIEW_QT_MAJOR_VERSION}"
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
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets")
vtk_module_definitions(ParaView::pqPython PRIVATE QT_NO_KEYWORDS)
