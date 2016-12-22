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
  vtkCompleteArrays.cxx
  vtkExtractsDeliveryHelper.cxx
  vtkMPIMToNSocketConnection.cxx
  vtkMPIMToNSocketConnectionPortInformation.cxx
  vtkNetworkAccessManager.cxx
  vtkPConvertSelection.cxx
  vtkProcessModuleAutoMPI.cxx
  vtkProcessModule.cxx
  vtkPVAlgorithmPortsInformation.cxx
  vtkPVArrayInformation.cxx
  vtkPVClassNameInformation.cxx
  vtkPVCompositeDataInformation.cxx
  vtkPVCompositeDataInformationIterator.cxx
  vtkPVDataInformation.cxx
  vtkPVDataInformationHelper.cxx
  vtkPVDataSetAttributesInformation.cxx
  vtkPVDataSizeInformation.cxx
  vtkPVDisableStackTraceSignalHandler.cxx
  vtkPVEnableStackTraceSignalHandler.cxx
  vtkPVExtractSelection.cxx
  vtkPVExtractArraysOverTime.cxx
  vtkPVFileInformationHelper.cxx
  vtkPVGenericAttributeInformation.cxx
  vtkPVInformation.cxx
  vtkPVMemoryUseInformation.cxx
  vtkPVMultiClientsInformation.cxx
  vtkPVOptions.cxx
  vtkPVOptionsXMLParser.cxx
  vtkPVPlugin.cxx
  vtkPVPluginLoader.cxx
  vtkPVPluginsInformation.cxx
  vtkPVPluginTracker.cxx
  vtkPVProgressHandler.cxx
  vtkPVPythonInformation.cxx
  vtkPVPythonModule.cxx
  vtkPVPythonOptions.cxx
  vtkPVPythonPluginInterface.cxx
  vtkPVServerInformation.cxx
  vtkPVServerManagerPluginInterface.cxx
  vtkPVServerOptions.cxx
  vtkPVSession.cxx
  vtkPVSILInformation.cxx
  vtkPVSystemConfigInformation.cxx
  vtkPVSystemInformation.cxx
  vtkPVTemporalDataInformation.cxx
  vtkPVTimerInformation.cxx
  vtkSession.cxx
  vtkSessionIterator.cxx
  vtkTCPNetworkAccessManager.cxx
  vtkEnvironmentAnnotationFilter.cxx
)

set(PARAVIEW_PLUGIN_LOADER_PATHS ""
  CACHE STRING "Extra paths to search for plugins")
mark_as_advanced(PARAVIEW_PLUGIN_LOADER_PATHS)
if (PARAVIEW_PLUGIN_LOADER_PATHS)
  set_source_files_properties(vtkPVPluginLoader.cxx
    PROPERTIES
      COMPILE_DEFINITIONS PARAVIEW_PLUGIN_LOADER_PATHS=\"${PARAVIEW_PLUGIN_LOADER_PATHS}\")
endif ()

if (PARAVIEW_WITH_SUPERBUILD_MESA)
  set(pv_options_definitions
    PARAVIEW_WITH_SUPERBUILD_MESA)

  if (PARAVIEW_WITH_SUPERBUILD_MESA_SWR)
    list(APPEND pv_options_definitions
      PARAVIEW_WITH_SUPERBUILD_MESA_SWR)
  endif ()

  set_source_files_properties(vtkPVOptions.cxx
    PROPERTIES
      COMPILE_DEFINITIONS "${pv_options_definitions}")
endif ()

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND Module_SRCS
    vtkPythonExtractSelection.cxx
    vtkPythonProgrammableFilter.cxx
    vtkPythonAnnotationFilter.cxx
    )
endif()

set_source_files_properties(
  vtkExtractsDeliveryHelper
  vtkProcessModuleAutoMPI
  vtkPVOptionsXMLParser
  vtkPVPlugin
  vtkPVPythonPluginInterface
  vtkPVServerManagerPluginInterface
  vtkPVServerOptions

  WRAP_EXCLUDE
  )

set_source_files_properties(
  vtkNetworkAccessManager
  vtkPVDataInformationHelper
  vtkPVInformation
  vtkPVPlugin
  vtkPVPluginTracker
  vtkPVPythonPluginInterface
  vtkPVServerManagerPluginInterface
  vtkPVSession
  vtkSession
  ABSTRACT)

if(VTK_USE_OFFSCREEN_EGL)
  set_source_files_properties(
    vtkPVOptions.cxx
    PROPERTIES
    COMPILE_DEFINITIONS VTK_USE_OFFSCREEN_EGL)
endif()

if(PARAVIEW_USE_MPI)
  include(vtkMPI)
endif()

# Use a custom hints file for this module.
set(${vtk-module}_WRAP_HINTS "${CMAKE_CURRENT_SOURCE_DIR}/hints")
vtk_module_library(vtkPVClientServerCoreCore ${Module_SRCS})
if(PARAVIEW_USE_MPI)
  vtk_mpi_link(${vtk-module})
endif()
