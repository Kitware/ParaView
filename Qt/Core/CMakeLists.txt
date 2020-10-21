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
#=========================================================================
set(classes
  pqAnimationCue
  pqAnimationScene
  pqApplicationCore
  pqAutoStartInterface
  pqBoxChartView
  pqCheckBoxDelegate
  pqCollaborationEventPlayer
  pqCollaborationManager
  pqComparativeContextView
  pqComparativeRenderView
  pqComparativeXYBarChartView
  pqComparativeXYChartView
  pqContextView
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
  pqFileDialogModel
  pqFileDialogRecentDirsModel
  pqFlatTreeViewEventPlayer
  pqFlatTreeViewEventTranslator
  pqGraphLayoutStrategyInterface
  pqImageUtil
  pqInteractiveViewLink
  pqInterfaceTracker
  pqLinkViewWidget
  pqLinksModel
  pqLogViewerWidget
  pqMainWindowEventManager
  pqMultiSliceAxisWidget
  pqMultiSliceView
  pqNonEditableStyledItemDelegate
  pqObjectBuilder
  pqOptions
  pqOutputPort
  pqOutputWidget
  pqParallelCoordinatesChartView
  pqPipelineFilter
  pqPipelineRepresentation
  pqPipelineSource
  pqPlotMatrixView
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
  pqSpreadSheetView
  pqSpreadSheetViewModel
  pqSpreadSheetViewSelectionModel
  pqSpreadSheetViewWidget
  pqStandardServerManagerModelInterface
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

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND classes
    pqDebug)
endif()

set(built_shared 0)
if (BUILD_SHARED_LIBS)
  set(built_shared 1)
endif ()
set_property(SOURCE pqCoreInit.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS "BUILD_SHARED_LIBS=${built_shared}")
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

if (PARAVIEW_USE_QTHELP)
  list(APPEND qt_components
    Help)
endif ()

vtk_module_find_package(
  PACKAGE     Qt5
  COMPONENTS  ${qt_components}
  VERSION     5.12
  FORWARD_VERSION_REQ MINOR)

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")

source_group("Resources" FILES
  ${ui_files}
  ${resource_files})

vtk_module_add_module(ParaView::pqCore
  CLASSES ${classes}
  SOURCES ${ui_files}
          ${resource_files})
vtk_module_link(ParaView::pqCore
  PRIVATE
    Qt5::Widgets)
if (PARAVIEW_USE_QTHELP)
  vtk_module_link(ParaView::pqCore
    PRIVATE
      Qt5::Help)
endif ()

if (PARAVIEW_USE_PYTHON)
  vtk_module_definitions(ParaView::pqCore
    PRIVATE PARAVIEW_PQCORE_ENABLE_PYTHON=1)
else ()
  vtk_module_definitions(ParaView::pqCore
    PRIVATE PARAVIEW_PQCORE_ENABLE_PYTHON=0)
endif()
