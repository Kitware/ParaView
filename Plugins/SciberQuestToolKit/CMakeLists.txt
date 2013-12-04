#    ____    _ __           ____               __    ____
#   / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
#  _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
# /___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
#
# Copyright 2012 SciberQuest Inc.
#
cmake_minimum_required(VERSION 2.8)
if (POLICY CMP0017)
  cmake_policy(SET CMP0017 NEW)
endif()
if (POLICY CMP0003)
  cmake_policy(SET CMP0003 NEW)
endif()

project(SciberQuestToolKit)

if("${PROJECT_SOURCE_DIR}"  STREQUAL "${PROJECT_BINARY_DIR}")
  message(FATAL_ERROR "SciberQuestToolKit requires an out of source Build.")
endif()

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

#--------------------------------------------------
# Use ParaView configuration for MPI
#--------------------------------------------------
if(PARAVIEW_USE_MPI)
  set(SQTK_WITHOUT_MPI OFF)
  add_definitions("-DMPICH_IGNORE_CXX_SEEK")
else()
  set(SQTK_WITHOUT_MPI ON)
  add_definitions(-DSQTK_WITHOUT_MPI)
endif()

#--------------------------------------------------
# Configuration options
#--------------------------------------------------
set(SQTK_CUDA OFF CACHE BOOL "Enable CUDA optimizations.")
mark_as_advanced(SQTK_CUDA)

set(SQTK_PYTHON_BINDINGS OFF CACHE BOOL "Enable Python bindings.")
mark_as_advanced(SQTK_PYTHON_BINDINGS)

if (PARAVIEW_USE_UNIFIED_BINDINGS)
  set(SQTK_PYTHON_BINDINGS ON CACHE BOOL "Enable Python bindings (needed for unified bindings)." FORCE)
endif ()

if (BUILD_TESTING)
  #message(STATUS "SQTK Building regression tests.")
  subdirs(Testing)
endif()

set(SQTK_DEBUG OFF CACHE BOOL "Enable debug output to std err.")
mark_as_advanced(SQTK_DEBUG)
if (SQTK_DEBUG)
  add_definitions(-DSQTK_DEBUG)
endif()

include(EigenConfig.cmake)

if (SQTK_CUDA)
  include(CUDAConfig.cmake)
endif (SQTK_CUDA)

include(GeneratePythonBindings.cmake)
include(GetDate.cmake)

# identify the build
set(SQTK_RELEASE "20120904")
set(SQTK_BUILD_DATE)
GetDate(SQTK_BUILD_DATE)
set (SQTK_VERSION "SQTK-${SQTK_RELEASE} ${CMAKE_SYSTEM} ${CMAKE_CXX_COMPILER} ${SQTK_BUILD_DATE}")
#message(STATUS "SQTK version ${SQTK_VERSION}")

# put the plugin inside the pv install
set(SQTK_LIB_INSTALL_PATH
  "${CMAKE_INSTALL_PREFIX}/lib/paraview-${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
  )
#message(STATUS "SQTK will be installed at ${SQTK_LIB_INSTALL_PATH}")

# Server manager wrapped sources.
set(SM_CXX_SOURCES
  vtkSQAgyrotropyFilter.cxx
  vtkSQBinaryThreshold.cxx
  vtkSQBOVMetaReader.cxx
  vtkSQBOVReader.cxx
  vtkSQBOVReaderBase.cxx
  vtkSQBOVWriter.cxx
  vtkSQCellGenerator.cxx
  vtkSQEdgeFilter.cxx
  vtkSQFieldTopologySelect.cxx
  vtkSQFieldTopologySplit.cxx
  vtkSQFieldTracer.cxx
  vtkSQFTLE.cxx
  vtkSQHemisphereSource.cxx
  vtkSQImageGhosts.cxx
  vtkSQImageSource.cxx
  vtkSQKernelConvolution.cxx
  vtkSQLineSource.cxx
  vtkSQLog.cxx
  vtkSQLogSource.cxx
  vtkSQMedianFilter.cxx
  vtkSQPlaneSourceCellGenerator.cxx
  vtkSQVolumeSourceCellGenerator.cxx
  vtkSQPlaneSource.cxx
  vtkSQPointSource.cxx
  vtkSQRandomCells.cxx
  vtkSQRandomSeedPoints.cxx
  vtkSQSeedPointLatice.cxx
  vtkSQSphereSource.cxx
  vtkSQSurfaceVectors.cxx
  vtkSQTubeFilter.cxx
  vtkSQVolumeSource.cxx
  vtkSQVortexFilter.cxx
  )

set_source_files_properties(
  vtkSQBOVReaderBase.cxx
  vtkSQOOCReader.cxx
  vtkSQCellGenerator.cxx
  ABSTRACT
  )

# Un-wrapped sources
set(CXX_SOURCES
  BOVMetaData.cxx
  BOVReader.cxx
  BOVTimeStepImage.cxx
  BOVScalarImage.cxx
  BOVVectorImage.cxx
  BOVWriter.cxx
  CartesianBounds.cxx
  CartesianDecomp.cxx
  CartesianDataBlock.cxx
  CartesianDataBlockIODescriptor.cxx
  CartesianDataBlockIODescriptorIterator.cxx
  CartesianExtent.cxx
  CartesianExtentIterator.cxx
  CellCopier.cxx
  CellIdIterator.cxx
  CPUConvolutionDriver.cxx
  IdBlock.cxx
  FieldDisplacementMapData.cxx
  FieldLine.cxx
  FieldTopologyMapData.cxx
  FieldTraceData.cxx
  FlatIndex.cxx
  FsUtils.cxx
  GDAMetaData.cxx
  GDAMetaDataKeys.cxx
  GhostTransaction.cxx
  ImageDecomp.cxx
  IntersectionSet.cxx
  LogBuffer.cxx
  PolyDataCellCopier.cxx
  PolyDataFieldDisplacementMap.cxx
  PolyDataFieldTopologyMap.cxx
  PoincareMapData.cxx
  postream.cxx
  PrintUtils.cxx
  RectilinearDecomp.cxx
  RefCountedPointer.cxx
  StreamlineData.cxx
  TerminationCondition.cxx
  TopologicalClassSelector.cxx
  UnstructuredFieldDisplacementMap.cxx
  UnstructuredFieldTopologyMap.cxx
  UnstructuredGridCellCopier.cxx
  vtkSQHemisphereSourceConfigurationWriter.cxx
  vtkSQHemisphereSourceConfigurationReader.cxx
  vtkSQMetaDataKeys.cxx
  vtkSQOOCBOVReader.cxx
  vtkSQOOCReader.cxx
  vtkSQPlaneSourceConfigurationWriter.cxx
  vtkSQPlaneSourceConfigurationReader.cxx
  vtkSQVolumeSourceConfigurationWriter.cxx
  vtkSQVolumeSourceConfigurationReader.cxx
  XMLUtils.cxx
  )

if (WIN32)
  set(CXX_SOURCES
    ${CXX_SOURCES}
    win_windirent.cxx)
endif()

if (CUDA_FOUND AND SQTK_CUDA)
  #message(STATUS "SQTK Including CUDA accelerated filters.")
  #message(STATUS "SQTK Cuda ${CUDA_VERSION_STRING} found.")
  set(CUDA_NVCC_FLAGS "-arch=sm_20;--compiler-options;-fPIC" CACHE STRING "nvcc flags")
  set(CUDA_NVCC_DEBUG_FLAGS "-g;-G;" CACHE STRING "nvcc debug flags")
  set(CUDA_NVCC_RELEASE_FLAGS "-O3" CACHE STRING "nvcc release flags")
  #message(STATUS "SQTK CUDA_NVCC_FLAGS=${CUDA_NVCC_FLAGS}")
  #message(STATUS "SQTK CUDA_NVCC_DEBUG_FLAGS=${CUDA_NVCC_DEBUG_FLAGS}")
  #message(STATUS "SQTK CUDA_NVCC_RELEASE_FLAGS=${CUDA_NVCC_FLAGS}")
  set(CU_SOURCES CUDAConvolutionDriver.cu)
  set_property(
    SOURCE ${CU_SOURCES}
    APPEND PROPERTY COMPILE_DEFINITIONS SQTK_CUDA
    )
  CUDA_ADD_LIBRARY(CUDAConvolutionDriver ${CU_SOURCES})
  install(TARGETS CUDAConvolutionDriver DESTINATION ${SQTK_LIB_INSTALL_PATH})
  CUDA_BUILD_CLEAN_TARGET()
else()
  set(CXX_SOURCES
    ${CXX_SOURCES}
    CUDAConvolutionDriver.cxx
    )
endif()

if(PARAVIEW_BUILD_QT_GUI)
  #message(STATUS "SQTK Including custom panels.")
  set(_IFACES)
  set(_IFACE_SRCS)
  # Qt custom panels
  QT4_WRAP_CPP(MOC_SRCS pqSQHemisphereSource.h)
  QT4_WRAP_UI(UI_SRCS pqSQHemisphereSourceForm.ui)
  ADD_PARAVIEW_OBJECT_PANEL(
    IFACES IFACE_SRCS
    CLASS_NAME pqSQHemisphereSource
    XML_NAME vtkSQHemisphereSource
    XML_GROUP sources)
  set(_IFACES ${_IFACES} ${IFACES})
  set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})

  QT4_WRAP_CPP(MOC_SRCS pqSQPlaneSource.h)
  QT4_WRAP_UI(UI_SRCS pqSQPlaneSourceForm.ui)
  QT4_WRAP_CPP(MOC_SRCS pqSQTranslateDialog.h)
  QT4_WRAP_UI(UI_SRCS pqSQTranslateDialogForm.ui)
  ADD_PARAVIEW_OBJECT_PANEL(
    IFACES IFACE_SRCS
    CLASS_NAME pqSQPlaneSource
    XML_NAME vtkSQPlaneSource
    XML_GROUP sources)
  set(_IFACES ${_IFACES} ${IFACES})
  set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})

  set(SQTK_SUPPORTS_DYNAMIC_LOAD_BALANCING
    vtkSQFieldTracer
    vtkSQFieldTracerRK45
    vtkSQFieldTopologyMapper
    vtkSQFieldTopologyMapperRK45
    vtkSQPoincareMapper
    vtkSQPoincareMapper45
    vtkSQDisplacementMapper
    vtkSQDisplacementMapperRK45
    )
  QT4_WRAP_CPP(MOC_SRCS pqSQFieldTracer.h)
  ADD_PARAVIEW_OBJECT_PANEL(
    IFACES IFACE_SRCS
    CLASS_NAME pqSQFieldTracer
    XML_NAME ${SQTK_SUPPORTS_DYNAMIC_LOAD_BALANCING}
    XML_GROUP filters)
  set(_IFACES ${_IFACES} ${IFACES})
  set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})

  QT4_WRAP_CPP(MOC_SRCS pqSQImageGhosts.h)
  ADD_PARAVIEW_OBJECT_PANEL(
    IFACES IFACE_SRCS
    CLASS_NAME pqSQImageGhosts
    XML_NAME vtkSQImageGhosts
    XML_GROUP filters)
  set(_IFACES ${_IFACES} ${IFACES})
  set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})

  QT4_WRAP_CPP(MOC_SRCS pqSQVolumeSource.h)
  QT4_WRAP_UI(UI_SRCS pqSQVolumeSourceForm.ui)
  ADD_PARAVIEW_OBJECT_PANEL(
    IFACES IFACE_SRCS
    CLASS_NAME pqSQVolumeSource
    XML_NAME vtkSQVolumeSource
    XML_GROUP sources)
  set(_IFACES ${_IFACES} ${IFACES})
  set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})

  set (CXX_SOURCES
    ${CXX_SOURCES}
    ${MOC_SRCS}
    ${UI_SRCS}
    ${_IFACE_SRCS}
    pqSQFieldTracer.cxx
    pqSQHemisphereSource.cxx
    pqSQImageGhosts.cxx
    pqSQPlaneSource.cxx
    pqSQTranslateDialog.cxx
    pqSQVolumeSource.cxx
    )

endif (PARAVIEW_BUILD_QT_GUI)

set (SQTK_XML
  SciberQuestToolKitSources.xml
  SciberQuestToolKitFilters.xml)

set (SQTK_SM_XML
  SciberQuestToolKitSMReaders.xml
  SciberQuestToolKitSMWriters.xml
  SciberQuestToolKitSMSources.xml
  SciberQuestToolKitSMFilters.xml)

if (SQTK_PYTHON_BINDINGS)
  #message(STATUS "SQTK generating vtk python bindings.")
  generate_python_bindings(SciberQuestToolKit "${SM_CXX_SOURCES}")
  install(TARGETS SciberQuestToolKitPython DESTINATION ${SQTK_LIB_INSTALL_PATH})
  install(TARGETS SciberQuestToolKitPythonD DESTINATION ${SQTK_LIB_INSTALL_PATH})
endif()

# Generate the plugin, servermanager and wrapped language bindings.
add_paraview_plugin(
  SciberQuestToolKit "${SQTK_VERSION}"
  GUI_INTERFACES ${_IFACES}
  SERVER_MANAGER_SOURCES ${SM_CXX_SOURCES}
  SERVER_MANAGER_XML ${SQTK_SM_XML}
  GUI_RESOURCE_FILES ${SQTK_XML}
  SOURCES ${CXX_SOURCES})

if (SQTK_CUDA AND CUDA_FOUND)
  target_link_libraries(SciberQuestToolKit
  LINK_PUBLIC CUDAConvolutionDriver)
endif()

target_link_libraries(SciberQuestToolKit
  LINK_PUBLIC vtkFiltersFlowPaths
  LINK_PRIVATE vtksys)

install(TARGETS SciberQuestToolKit DESTINATION ${SQTK_LIB_INSTALL_PATH})

if(PARAVIEW_USE_MPI)
  include(vtkMPI)
  vtk_mpi_link(SciberQuestToolKit)
endif()
