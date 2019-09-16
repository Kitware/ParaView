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
  vtkPVComparativeAnimationCue
  vtkPVComparativeView
  vtkPVEncodeSelectionForServer
  vtkPVRepresentedArrayListSettings
  vtkSMBoxRepresentationProxy
  vtkSMCameraLink
  vtkSMCameraProxy
  vtkSMChartRepresentationProxy
  vtkSMChartSeriesListDomain
  vtkSMChartSeriesSelectionDomain
  vtkSMChartUseIndexForAxisDomain
  vtkSMComparativeAnimationCueProxy
  vtkSMComparativeAnimationCueUndoElement
  vtkSMComparativeViewProxy
  vtkSMContextViewProxy
  vtkSMDataDeliveryManagerProxy
  vtkSMImplicitPlaneRepresentationProxy
  vtkSMLightProxy
  vtkSMMaterialDomain
  vtkSMMaterialLibraryProxy
  vtkSMMultiSliceViewProxy
  vtkSMNewWidgetRepresentationProxy
  vtkSMOrthographicSliceViewProxy
  vtkSMPVMoleculeRepresentationProxy
  vtkSMPVRepresentationProxy
  vtkSMParaViewPipelineControllerWithRendering
  vtkSMPlotMatrixViewProxy
  vtkSMRangedTransferFunctionDomain
  vtkSMRenderViewProxy
  vtkSMRendererDomain
  vtkSMRepresentationProxy
  vtkSMRepresentationTypeDomain
  vtkSMRepresentedArrayListDomain
  vtkSMSaveScreenshotProxy
  vtkSMScalarBarWidgetRepresentationProxy
  vtkSMSelectionHelper
  vtkSMSelectionLink
  vtkSMSpreadSheetRepresentationProxy
  vtkSMSpreadSheetViewProxy
  vtkSMTextWidgetRepresentationProxy
  vtkSMTransferFunctionManager
  vtkSMTransferFunctionPresets
  vtkSMTransferFunctionProxy
  vtkSMUtilities
  vtkSMViewLayoutProxy
  vtkSMViewProxy
  vtkSMViewProxyInteractorHelper
  vtkSMViewResolutionDomain
  vtkSMWidgetRepresentationProxy)

if (TARGET ParaView::ClientServerCorePython AND
    TARGET ParaView::ClientServerCorePythonRendering)
  # TODO: Split these into a separate module so that there isn't a public
  # optional dependency.
  list(APPEND classes
    vtkSMPythonViewProxy)
endif ()

# Compile ColorMaps.json
add_custom_command(
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ColorMaps.json"
          ParaView::ProcessXML
  COMMAND ParaView::ProcessXML
          "${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h"
          vtkSMTransferFunctionPresets JSON JSON
          "${CMAKE_CURRENT_SOURCE_DIR}/ColorMaps.json"
  COMMENT "Processing builtin color maps")
set(private_headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h")

vtk_object_factory_declare(
  BASE vtkSMParaViewPipelineController
  OVERRIDE vtkSMParaViewPipelineControllerWithRendering)
vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKPVSERVERMANAGERRENDERING_EXPORT")

vtk_module_add_module(ParaView::ServerManagerRendering
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header} ${private_headers})
