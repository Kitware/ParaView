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
set (Module_SRCS
  vtkPVComparativeAnimationCue.cxx
  vtkPVComparativeView.cxx
  vtkPVRepresentedArrayListSettings.cxx
  vtkSMBagChartSeriesListDomain.cxx
  vtkSMBoxRepresentationProxy.cxx
  vtkSMCameraLink.cxx
  vtkSMCameraProxy.cxx
  vtkSMChartRepresentationProxy.cxx
  vtkSMChartSeriesListDomain.cxx
  vtkSMChartSeriesSelectionDomain.cxx
  vtkSMChartUseIndexForAxisDomain.cxx
  vtkSMComparativeAnimationCueProxy.cxx
  vtkSMComparativeAnimationCueUndoElement.cxx
  vtkSMComparativeViewProxy.cxx
  vtkSMContextViewProxy.cxx
  vtkSMDataDeliveryManager.cxx
  vtkSMFunctionalBagChartSeriesSelectionDomain.cxx
  vtkSMImplicitPlaneRepresentationProxy.cxx
  vtkSMMultiSliceViewProxy.cxx
  vtkSMNewWidgetRepresentationProxy.cxx
  vtkSMOrthographicSliceViewProxy.cxx
  vtkSMPlotMatrixViewProxy.cxx
  vtkSMPVRepresentationProxy.cxx
  vtkSMRenderViewProxy.cxx
  vtkSMRepresentationProxy.cxx
  vtkSMRepresentationTypeDomain.cxx
  vtkSMRepresentedArrayListDomain.cxx
  vtkSMScalarBarWidgetRepresentationProxy.cxx
  vtkSMSaveScreenshotProxy.cxx
  vtkSMSelectionHelper.cxx
  vtkSMSelectionLink.cxx
  vtkSMSpreadSheetRepresentationProxy.cxx
  vtkSMTextWidgetRepresentationProxy.cxx
  vtkSMTransferFunctionManager.cxx
  vtkSMTransferFunctionPresets.cxx
  vtkSMTransferFunctionProxy.cxx
  vtkSMUtilities.cxx
  vtkSMViewLayoutProxy.cxx
  vtkSMViewProxy.cxx
  vtkSMViewProxyInteractorHelper.cxx
  vtkSMViewResolutionDomain.cxx
  vtkSMWidgetRepresentationProxy.cxx
)

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND Module_SRCS vtkSMPythonViewProxy.cxx)
endif()


#-----------------------------------------------------------------------------
# Stuff for ObjectFactory override for vtkSMParaViewPipelineControllerWithRendering.
list(APPEND Module_SRCS
  vtkSMParaViewPipelineControllerWithRendering.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

set_source_files_properties(${vtk-module}ObjectFactory WRAP_EXCLUDE)

# Override some rendering classes. Create object factory:
set(vtk_module_overrides "vtkSMParaViewPipelineController")
set(vtk_module_vtkSMParaViewPipelineController_override
  "vtkSMParaViewPipelineControllerWithRendering")

# Now we iterate and create that class file...
foreach(_class ${vtk_module_overrides})
  set(_override ${vtk_module_${_class}_override})
  set(_vtk_override_includes "${_vtk_override_includes}
#include \"${_override}.h\"")
  set(_vtk_override_creates "${_vtk_override_creates}
VTK_CREATE_CREATE_FUNCTION(${_override})")
  set(_vtk_override_do "${_vtk_override_do}
  this->RegisterOverride(\"${_class}\",
                         \"${_override}\",
                         \"Override for ${vtk-module} module\", 1,
                         vtkObjectFactoryCreate${_override});")
endforeach()

# Now lets create the object factory classes
string(TOUPPER ${vtk-module} VTK-MODULE)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Compile ColorMaps.json
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ColorMaps.json
          kwProcessXML
  COMMAND kwProcessXML
          ${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h
          vtkSMTransferFunctionPresets JSON JSON
          ${CMAKE_CURRENT_SOURCE_DIR}/ColorMaps.json
  COMMENT "Processing builtin color maps")
set(${vtk-module}_HDRS
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h)
set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h
  HEADER_FILE_ONLY
  WRAP_EXCLUDE)
#-----------------------------------------------------------------------------
if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  add_definitions(-DVTKGL2)
endif()
vtk_module_library(vtkPVServerManagerRendering ${Module_SRCS})
