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
  vtkCompleteArrays
  vtkEnvironmentAnnotationFilter
  vtkExtractsDeliveryHelper
  vtkMPIMToNSocketConnection
  vtkMPIMToNSocketConnectionPortInformation
  vtkNetworkAccessManager
  vtkPConvertSelection
  vtkPVAlgorithmPortsInformation
  vtkPVArrayInformation
  vtkPVClassNameInformation
  vtkPVCompositeDataInformation
  vtkPVCompositeDataInformationIterator
  vtkPVDataInformation
  vtkPVDataInformationHelper
  vtkPVDataSetAttributesInformation
  vtkPVDataSizeInformation
  vtkPVDisableStackTraceSignalHandler
  vtkPVEnableStackTraceSignalHandler
  vtkPVExtractSelection
  vtkPVFileInformationHelper
  vtkPVGenericAttributeInformation
  vtkPVInformation
  vtkPVMemoryUseInformation
  vtkPVMultiClientsInformation
  vtkPVOptions
  vtkPVOptionsXMLParser
  vtkPVPlugin
  vtkPVPluginLoader
  vtkPVPluginTracker
  vtkPVPluginsInformation
  vtkPVProgressHandler
  vtkPVPythonInformation
  vtkPVPythonModule
  vtkPVPythonOptions
  vtkPVPythonPluginInterface
  vtkPVSILInformation
  vtkPVServerInformation
  vtkPVServerManagerPluginInterface
  vtkPVServerOptions
  vtkPVSession
  vtkPVSystemConfigInformation
  vtkPVSystemInformation
  vtkPVTemporalDataInformation
  vtkPVTimerInformation
  vtkProcessModule
  vtkProcessModuleAutoMPI
  vtkSession
  vtkSessionIterator
  vtkTCPNetworkAccessManager)

set(built_shared 0)
if (BUILD_SHARED_LIBS)
  set(built_shared 1)
endif ()
set_property(SOURCE vtkPVPluginLoader.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS "BUILD_SHARED_LIBS=${built_shared}")

if (WIN32)
  set(libdir_relpath ".")
else ()
  file(RELATIVE_PATH libdir_relpath
    "/prefix/${_vtk_build_RUNTIME_DESTINATION}"
    "/prefix/${_vtk_build_LIBRARY_DESTINATION}")
endif ()
set_property(SOURCE vtkPVPluginTracker.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS
      "PARAVIEW_RELATIVE_LIBPATH=\"${libdir_relpath}\""
      "PARAVIEW_SUBDIR=\"paraview${paraview_version_suffix}/plugins\"")

if (TARGET VTK::PythonInterpreter)
  # TODO: Split these into a separate module so that there isn't a public
  # optional dependency.
  list(APPEND classes
    vtkPythonExtractSelection
    vtkPythonSelector)
endif ()

cmake_dependent_option(PARAVIEW_USE_MPI_SSEND
  "Use MPI synchronous-send commands for communication" OFF
  "PARAVIEW_USE_MPI" OFF)
cmake_dependent_option(PARAVIEW_INITIALIZE_MPI_ON_CLIENT
  "Initialize MPI on client-processes by default. Can be overridden using command line arguments" ON
  "PARAVIEW_USE_MPI" OFF)
mark_as_advanced(
  PARAVIEW_INITIALIZE_MPI_ON_CLIENT
  PARAVIEW_USE_MPI_SSEND)

set(vtkProcessModule_defines)
if (PARAVIEW_USE_MPI_SSEND)
  list(APPEND vtkProcessModule_defines
    PARAVIEW_USE_MPI_SSEND)
endif ()
if (PARAVIEW_INITIALIZE_MPI_ON_CLIENT)
  list(APPEND vtkProcessModule_defines
    PARAVIEW_INITIALIZE_MPI_ON_CLIENT)
endif ()
set_source_files_properties(vtkProcessModule.cxx
  PROPERTIES
    COMPILE_DEFINITIONS "${vtkProcessModule_defines}")

# TODO: Expose CMake options for these?
set(vtkPVOptions_defines)
if (PARAVIEW_ALWAYS_SECURE_CONNECTION)
  list(APPEND vtkPVOptions_defines
    PARAVIEW_ALWAYS_SECURE_CONNECTION)
endif ()
if (PARAVIEW_ENABLE_FPE)
  list(APPEND vtkPVOptions_defines
    PARAVIEW_ENABLE_FPE)
endif ()
if (VTK_OPENGL_HAS_EGL)
  list(APPEND vtkPVOptions_defines
    VTK_OPENGL_HAS_EGL)
endif ()
set_source_files_properties(vtkPVOptions.cxx
  PROPERTIES
    COMPILE_DEFINITIONS "${vtkPVOptions_defines}")

set(vtkProcessModuleAutoMPI_defines)
if (TARGET VTK::ParallelMPI)
  list(APPEND vtkProcessModuleAutoMPI_defines
    "PARAVIEW_MPIEXEC_EXECUTABLE=\"${MPIEXEC_EXECUTABLE}\""
    "PARAVIEW_MPI_NUMPROC_FLAG=\"${MPIEXEC_NUMPROC_FLAG}\"")
  if (MPI_PREFLAGS)
    list(APPEND vtkProcessModuleAutoMPI_defines
      "PARAVIEW_MPI_PREFLAGS=\"${MPIEXEC_PREFLAGS}\"")
  endif ()
  if (MPI_POSTFLAGS)
    list(APPEND vtkProcessModuleAutoMPI_defines
      "PARAVIEW_MPI_POSTFLAGS=\"${MPIEXEC_POSTFLAGS}\"")
  endif ()
endif ()
set_source_files_properties(vtkProcessModuleAutoMPI.cxx
  PROPERTIES
    COMPILE_DEFINITIONS "${vtkProcessModuleAutoMPI_defines}")

set(PARAVIEW_PLUGIN_LOADER_PATHS ""
  CACHE STRING "Extra paths to search for plugins")
mark_as_advanced(PARAVIEW_PLUGIN_LOADER_PATHS)
if (PARAVIEW_PLUGIN_LOADER_PATHS)
  set_source_files_properties(vtkPVPluginLoader.cxx
    PROPERTIES
      COMPILE_DEFINITIONS PARAVIEW_PLUGIN_LOADER_PATHS=\"${PARAVIEW_PLUGIN_LOADER_PATHS}\")
endif ()

vtk_module_add_module(ParaView::ClientServerCoreCore
  CLASSES ${classes})
