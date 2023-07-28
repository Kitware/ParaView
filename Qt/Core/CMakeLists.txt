# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  pqAnimationCue
  pqAnimationScene
  pqAnnotationsModel
  pqApplicationCore
  pqAutoStartInterface
  pqBoxChartView
  pqCameraWidgetViewLink
  pqCheckBoxDelegate
  pqCollaborationEventPlayer
  pqCollaborationManager
  pqComparativeContextView
  pqComparativeRenderView
  pqComparativeXYBarChartView
  pqComparativeXYChartView
  pqContextView
  pqCoreConfiguration
  pqCoreInit
  pqCoreTestUtility
  pqCoreUtilities
  pqDataRepresentation
  pqExtractor
  pqFileDialog
  pqFileDialogEventPlayer
  pqFileDialogEventTranslator
  pqFileDialogFavoriteModel
  pqFileDialogFilter
  pqFileDialogLocationModel
  pqFileDialogModel
  pqFileDialogRecentDirsModel
  pqFileUtilitiesEventPlayer
  pqFlatTreeViewEventPlayer
  pqFlatTreeViewEventTranslator
  pqGraphLayoutStrategyInterface
  pqImageUtil
  pqInteractiveViewLink
  pqInterfaceTracker
  pqKeySequences
  pqLinkedObjectInterface
  pqLinkedObjectQTextEdit
  pqLinkViewWidget
  pqLinksModel
  pqLogViewerWidget
  pqMainWindowEventManager
  pqModalShortcut
  pqMultiSliceAxisWidget
  pqMultiSliceView
  pqNonEditableStyledItemDelegate
  pqObjectBuilder
  pqOutputPort
  pqOutputWidget
  pqParallelCoordinatesChartView
  pqPipelineFilter
  pqPipelineRepresentation
  pqPipelineSource
  pqPlotMatrixView
  pqPluginLocationInterface
  pqPluginManager
  pqProgressManager
  pqPropertyLinks
  pqPropertyLinksConnection
  pqPropertyManager
  pqProxy
  pqProxyModifiedStateUndoElement
  pqProxySelection
  pqQVTKWidget
  pqQVTKWidgetEventPlayer
  pqQVTKWidgetEventTranslator
  pqRecentlyUsedResourceLoaderInterface
  pqRecentlyUsedResourcesList
  pqRenderView
  pqRenderViewBase
  pqRepresentation
  pqSMAdaptor
  pqSMProxy
  pqScalarBarRepresentation
  pqScalarsToColors
  pqServer
  pqServerConfiguration
  pqServerConfigurationCollection
  pqServerManagerModel
  pqServerManagerModelInterface
  pqServerManagerModelItem
  pqServerManagerObserver
  pqServerResource
  pqSettings
  pqSignalsBlocker
  pqSpreadSheetView
  pqSpreadSheetViewModel
  pqSpreadSheetViewSelectionModel
  pqSpreadSheetViewWidget
  pqStandardServerManagerModelInterface
  pqTextLinker
  pqTimeKeeper
  pqTreeLayoutStrategyInterface
  pqUndoStack
  pqView
  pqXMLEventObserver
  pqXMLEventSource
  pqXMLUtil
  pqXYBarChartView
  pqXYChartView
  pqXYHistogramChartView
  vtkPVGUIPluginInterface)

set(headers
  pqQVTKWidgetBase.h)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pqQtConfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pqQtConfig.h"
  @ONLY)
list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/pqQtConfig.h")

set_property(SOURCE pqCoreInit.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS "BUILD_SHARED_LIBS=$<BOOL:${BUILD_SHARED_LIBS}>")
set_property(SOURCE pqCoreTestUtility.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS "PARAVIEW_DATA_ROOT=\"${PARAVIEW_DATA_ROOT}\"")

set(ui_files
  Resources/UI/pqFileDialog.ui
  Resources/UI/pqLogViewerWidget.ui
  Resources/UI/pqOutputWidget.ui
  Resources/UI/pqPluginEULADialog.ui)

set(resource_files
  Resources/pqCore.qrc)

if (PARAVIEW_USE_PYTHON)
  # TODO: Split these into a separate module so that there isn't a public
  # optional dependency.
  list(APPEND classes
    pqPythonView)
endif ()

set(qt_components
  Widgets)

if (PARAVIEW_QT_MAJOR_VERSION GREATER "5")
  list(APPEND qt_components
    Core5Compat)
endif ()

if (PARAVIEW_USE_QTHELP)
  list(APPEND qt_components
    Help)
endif ()

vtk_module_find_package(
  PACKAGE     "Qt${PARAVIEW_QT_MAJOR_VERSION}"
  COMPONENTS  ${qt_components}
  VERSION     5.12
  FORWARD_VERSION_REQ MINOR)

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
    TARGET "Core.ts"
    FILES ${translatable_files}
    OUTPUT_TS "${PARAVIEW_TRANSLATIONS_DIRECTORY}/Qt_Core.ts")
  add_dependencies(localization "Core.ts")
endif ()

source_group("Resources" FILES
  ${ui_files}
  ${resource_files})

vtk_module_add_module(ParaView::pqCore
  CLASSES ${classes}
  SOURCES ${ui_files}
          ${resource_files}
  HEADERS ${headers})
vtk_module_link(ParaView::pqCore
  PRIVATE
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets")

if (PARAVIEW_QT_MAJOR_VERSION GREATER "5")
  vtk_module_link(ParaView::pqCore
    PRIVATE
      "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core5Compat")
endif ()

if (PARAVIEW_USE_QTHELP)
  vtk_module_link(ParaView::pqCore
    PRIVATE
      "Qt${PARAVIEW_QT_MAJOR_VERSION}::Help")
endif ()
vtk_module_definitions(ParaView::pqCore PRIVATE QT_NO_KEYWORDS)

vtk_module_definitions(ParaView::pqCore
  PRIVATE "PARAVIEW_PQCORE_ENABLE_PYTHON=$<BOOL:${PARAVIEW_USE_PYTHON}>")
