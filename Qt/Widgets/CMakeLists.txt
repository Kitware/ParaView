# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  pqAnimationKeyFrame
  pqAnimationModel
  pqAnimationTrack
  pqAnimationWidget
  pqCheckBoxPixMaps
  pqCheckBoxPixMaps
  pqCheckableHeaderView
  pqClickableLabel
  pqCollapsedGroup
  pqColorButtonEventPlayer
  pqColorButtonEventTranslator
  pqColorChooserButton
  pqColorDialogEventPlayer
  pqColorDialogEventTranslator
  pqConnect
  pqConsoleWidget
  pqConsoleWidgetEventPlayer
  pqConsoleWidgetEventTranslator
  pqDelimitedTextParser
  pqDiscreteDoubleWidget
  pqDoubleLineEdit
  pqDoubleRangeDialog
  pqDoubleRangeWidget
  pqDoubleSliderWidget
  pqDoubleSpinBox
  pqExpandableTableView
  pqExpanderButton
  pqFlatTreeView
  pqHeaderView
  pqHierarchicalGridLayout
  pqHierarchicalGridWidget
  pqHighlightablePushButton
  pqHighlightableToolButton
  pqLineEdit
  pqLineEditEventPlayer
  pqListWidgetCheckHelper
  pqListWidgetItemObject
  pqMultiColumnHeaderView
  pqProgressWidget
  pqQuickLaunchDialog
  pqScaleByButton
  pqScaledSpinBox
  pqSectionVisibilityContextMenu
  pqSeriesGeneratorDialog
  pqSetData
  pqSetName
  pqSignalAdaptors
  pqSpinBox
  pqTableView
  pqTextEdit
  pqTimelineScrollbar
  pqTreeView
  pqTreeViewExpandState
  pqTreeViewSelectionHelper
  pqTreeWidget
  pqTreeWidgetCheckHelper
  pqTreeWidgetItem
  pqTreeWidgetItemObject
  pqTreeWidgetSelectionHelper
  pqVectorWidget
  pqWaitCursor
  pqWidgetCompleter
  pqWidgetsInit)

set(headers)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pqQtWidgetsConfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pqQtWidgetsConfig.h"
  @ONLY)
list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/pqQtWidgetsConfig.h")

set(qt_components
  Core
  Gui
  Network
  Widgets)

if (APPLE)
  list(APPEND qt_components
    PrintSupport)
endif ()

if (PARAVIEW_QT_MAJOR_VERSION GREATER "5")
  list(APPEND qt_components
    Core5Compat)
endif ()

vtk_module_find_package(
  PACKAGE     "Qt${PARAVIEW_QT_MAJOR_VERSION}"
  COMPONENTS  ${qt_components})

##########################################################################
## Add *.ui files to be uic processed.
set(ui_files
  Resources/UI/pqExpanderButton.ui
  Resources/UI/pqQuickLaunchDialog.ui
  Resources/UI/pqSeriesGeneratorDialog.ui)

##########################################################################
## Add resources.
set(resource_files
  Resources/QtWidgets.qrc)

source_group("Resources" FILES
  ${ui_files}
  ${resource_files})

list(APPEND headers
  pqQtDeprecated.h
)

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")

set(translatable_files)
if (PARAVIEW_BUILD_TRANSLATIONS)
  list(APPEND translatable_files ${ui_files})
  foreach(el IN LISTS classes)
    list(APPEND translatable_files "${el}.cxx")
    list(APPEND translatable_files "${el}.h")
  endforeach()

  paraview_create_translation(
    TARGET "Widgets.ts"
    FILES ${translatable_files}
    OUTPUT_TS "${PARAVIEW_TRANSLATIONS_DIRECTORY}/Qt_Widgets.ts")
  add_dependencies(localization "Widgets.ts")
endif ()

vtk_module_add_module(ParaView::pqWidgets
  CLASSES ${classes}
  SOURCES ${ui_files}
          ${resource_files}
  HEADERS ${headers})
vtk_module_link(ParaView::pqWidgets
  PUBLIC
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core"
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Gui"
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets"
  PRIVATE
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Network")
vtk_module_definitions(ParaView::pqWidgets PRIVATE QT_NO_KEYWORDS)

if (PARAVIEW_QT_MAJOR_VERSION GREATER "5")
  vtk_module_link(ParaView::pqWidgets
    PRIVATE
      "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core5Compat")
endif ()

if (APPLE)
  vtk_module_link(ParaView::pqWidgets
    PRIVATE
      "Qt${PARAVIEW_QT_MAJOR_VERSION}::PrintSupport")
endif ()
