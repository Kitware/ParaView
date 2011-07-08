cmake_minimum_required(VERSION 2.8)
#cmake_policy(SET CMP0003 NEW)
#    ____    _ __           ____               __    ____
#   / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
#  _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
# /___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
#
# Copyright 2008 SciberQuest Inc.
#
# +---------------------------------------------------------------------------+
# |                                                                           |
# |                            SciVis Tool Kit                                |
# |                                                                           |
# +---------------------------------------------------------------------------+
#
project(SQToolkit)

if("${PROJECT_SOURCE_DIR}"  STREQUAL "${PROJECT_BINARY_DIR}")
  message(FATAL_ERROR "SQToolkit requires an out of source Build.")
endif()

if (ParaView_SOURCE_DIR)
  include_directories(
    ${VTK_INCLUDE_DIR}
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
  )
else (ParaView_SOURCE_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif (ParaView_SOURCE_DIR)



include(EigenConfig.cmake)
include(MPICHConfig.cmake)
include(GimpConfig.cmake)
include(GeneratePythonBindings.cmake)

# Server manager wrapped sources.
set(SM_CXX_SOURCES
  vtkSQBOVReader.cxx
  vtkSQDistributedStreamTracer.cxx
  vtkSQFieldTopologySelect.cxx
  vtkSQFieldTopologySplit.cxx
  vtkSQFieldTracer.cxx
  vtkSQHemisphereSource.cxx
  vtkSQImageGhosts.cxx
  vtkSQKernelConvolution.cxx
  vtkSQLineSource.cxx
  vtkSQCellGenerator.cxx
  vtkSQPlaneSourceCellGenerator.cxx
  vtkSQPStreamTracer.cxx
  vtkSQVolumeSourceCellGenerator.cxx
  vtkSQPlaneSource.cxx
  vtkSQPointSource.cxx
  vtkSQRandomCells.cxx
  vtkSQRandomSeedPoints.cxx
  vtkSQSeedPointLatice.cxx
  vtkSQSphereSource.cxx
  vtkSQStreamTracer.cxx
  vtkSQSurfaceVectors.cxx
  vtkSQTensorGlyph.cxx
  vtkSQTubeFilter.cxx
  vtkSQVolumeSource.cxx
  vtkSQVortexFilter.cxx
  )

set(SVTK_PROCESS_MONITOR ON CACHE BOOL "Build the process monitor.")
if (SVTK_PROCESS_MONITOR)
  message(STATUS "Building process monitor.")
  set(SM_CXX_SOURCES ${SM_CXX_SOURCES} vtkSQProcessMonitor.cxx)
endif()

set_source_files_properties(
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
  CartesianBounds.cxx
  CartesianDecomp.cxx
  CartesianDataBlock.cxx
  CartesianDataBlockIODescriptor.cxx
  CartesianDataBlockIODescriptorIterator.cxx
  CartesianExtent.cxx
  CellCopier.cxx
  IdBlock.cxx
  FieldLine.cxx
  FieldTraceData.cxx
  FlatIndex.cxx
  FsUtils.cxx
  GDAMetaData.cxx
  GDAMetaDataKeys.cxx
  GhostTransaction.cxx
  ImageDecomp.cxx
  IntersectionSet.cxx
  PolyDataCellCopier.cxx
  PolyDataFieldTopologyMap.cxx
  PoincareMapData.cxx
  postream.cxx
  PrintUtils.cxx
  RectilinearDecomp.cxx
  RefCountedPointer.cxx
  StreamlineData.cxx
  SystemInterfaceFactory.cxx
  TerminationCondition.cxx
  TopologicalClassSelector.cxx
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
  )

if (SVTK_PROCESS_MONITOR)
  if (WIN32)
    set(CXX_SOURCES
      ${CXX_SOURCES}
      WindowsSystemInterface.cxx
      win_windirent.cxx
      )
  elseif (APPLE)
    set(CXX_SOURCES
      ${CXX_SOURCES}
      vtkSQLog.cxx
      UnixSystemInterface.cxx
      OSXSystemInterface.cxx
      )
  else()
    set(CXX_SOURCES
      ${CXX_SOURCES}
      vtkSQLog.cxx
      UnixSystemInterface.cxx
      LinuxSystemInterface.cxx
      )
  endif()
endif()

if(PARAVIEW_BUILD_QT_GUI)
  message(STATUS "Including custom panels.")
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
  ADD_PARAVIEW_OBJECT_PANEL(
    IFACES IFACE_SRCS
    CLASS_NAME pqSQPlaneSource
    XML_NAME vtkSQPlaneSource
    XML_GROUP sources)
  set(_IFACES ${_IFACES} ${IFACES})
  set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})

  QT4_WRAP_CPP(MOC_SRCS pqSQBOVMetaReader.h)
  ADD_PARAVIEW_OBJECT_PANEL(
    IFACES IFACE_SRCS
    CLASS_NAME pqSQBOVMetaReader
    XML_NAME vtkSQBOVMetaReader
    XML_GROUP sources)
  set(_IFACES ${_IFACES} ${IFACES})
  set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})

  QT4_WRAP_CPP(MOC_SRCS pqSQFieldTracer.h)
  ADD_PARAVIEW_OBJECT_PANEL(
    IFACES IFACE_SRCS
    CLASS_NAME pqSQFieldTracer
    XML_NAME vtkSQFieldTracer vtkSQFieldTracerRK45 vtkSQFieldTopologyMapper
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

  QT4_WRAP_CPP(MOC_SRCS pqSQTensorGlyph.h)
  ADD_PARAVIEW_OBJECT_PANEL(
    IFACES IFACE_SRCS
    CLASS_NAME pqSQTensorGlyph
    XML_NAME vtkSQTensorGlyph
    XML_GROUP filters)
  set(_IFACES ${_IFACES} ${IFACES})
  set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})

  if (SVTK_PROCESS_MONITOR)
    message(STATUS "Building process monitor ui.")
    QT4_WRAP_CPP(MOC_SRCS pqSQProcessMonitor.h)
    QT4_WRAP_UI(UI_SRCS pqSQProcessMonitorForm.ui)
    ADD_PARAVIEW_OBJECT_PANEL(
      IFACES IFACE_SRCS
      CLASS_NAME pqSQProcessMonitor
      XML_NAME vtkSQProcessMonitor
      XML_GROUP sources)
    set(_IFACES ${_IFACES} ${IFACES})
    set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})
  endif()

  set (CXX_SOURCES
    ${CXX_SOURCES}
    ${MOC_SRCS}
    ${UI_SRCS}
    ${_IFACE_SRCS}
    pqSQBOVMetaReader.cxx
    pqSQFieldTracer.cxx
    pqSQHemisphereSource.cxx
    pqSQPlaneSource.cxx
    pqSQTensorGlyph.cxx
    pqSQVolumeSource.cxx
    )

  if (SVTK_PROCESS_MONITOR)
    set (CXX_SOURCES ${CXX_SOURCES} pqSQProcessMonitor.cxx)
  endif()

endif (PARAVIEW_BUILD_QT_GUI)

set (CLIENT_XML
  SciVisToolKitClientReaders.xml
  SciVisToolKitClientSources.xml
  SciVisToolKitClientFilters.xml)

# Generate the plugin, servermanager and wrapped language bindings.
add_paraview_plugin(
  SQToolkit "11.02"
  GUI_INTERFACES ${_IFACES}
  SERVER_MANAGER_SOURCES ${SM_CXX_SOURCES}
  SERVER_MANAGER_XML SciVisToolKitServerManager.xml
  GUI_RESOURCE_FILES ${CLIENT_XML}
  SOURCES ${CXX_SOURCES})

if (WIN32 AND SVTK_PROCESS_MONITOR)
  target_link_libraries(SQToolkit Psapi)
endif()

install(TARGETS SQToolkit DESTINATION ${CMAKE_INSTALL_PREFIX})

if (NOT WIN32)
  generate_python_bindings(SQToolkit ${SM_CXX_SOURCES})
endif()

# GIMP scripts
set (SVTK_POSTPROC_SCRIPTS OFF CACHE BOOL "Install post-processing scripts.")
if ((NOT GIMP_FOUND) AND SVTK_POSTPROC_SCRIPTS)
  message(SEND_ERROR "GIMP was not found.")
endif()
if (GIMP_FOUND AND SVTK_POSTPROC_SCRIPTS)
  message(STATUS "Installing post-processing scripts.")
  set(POSTPROC_SCRIPT_DIR "${PROJECT_SOURCE_DIR}/script/postproc/")
  set(SVTK_GIMP_SCM_SOURCES
    "${POSTPROC_SCRIPT_DIR}/annotate.scm"
    "${POSTPROC_SCRIPT_DIR}/autocrop.scm"
    "${POSTPROC_SCRIPT_DIR}/crop.scm"
    )
  set(SVTK_GIMP_SH_SOURCES
    "${POSTPROC_SCRIPT_DIR}/annotate.sh"
    "${POSTPROC_SCRIPT_DIR}/autocrop.sh"
    "${POSTPROC_SCRIPT_DIR}/crop.sh"
    "${POSTPROC_SCRIPT_DIR}/mp42avi.sh"
    "${POSTPROC_SCRIPT_DIR}/png2mp4.sh"
    )
  install(FILES ${SVTK_GIMP_SCM_SOURCES} DESTINATION ${GIMP_SCRIPT_PATH})
  install(PROGRAMS ${SVTK_GIMP_SH_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

# batch scripts
set(SVTK_BATCH_SCRIPTS OFF CACHE BOOL "Install batch submission scripts.")
if (SVTK_BATCH_SCRIPTS)
  message(STATUS "Installing batch scripts.")
  set(BATCH_SCRIPT_DIR "${PROJECT_SOURCE_DIR}/script/batch/")
  set(SVTK_BATCH_SCRIPT_SOURCES
    "${BATCH_SCRIPT_DIR}/qsub-gentp-longhorn.sh"
    "${BATCH_SCRIPT_DIR}/qsub-mtopo-longhorn.sh"
    "${BATCH_SCRIPT_DIR}/qsub-pvserver-fc-batch-longhorn.sh"
    "${BATCH_SCRIPT_DIR}/qsub-pvserver-rc-batch-longhorn.sh"
    "${BATCH_SCRIPT_DIR}/qsub-test4-topo-3d-pleiades.sh"
    )
  install(PROGRAMS ${SVTK_BATCH_SCRIPT_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

# shell scripts
set(SVTK_SHELL_SCRIPTS ON CACHE BOOL "Install shell scripts.")
if (SVTK_SHELL_SCRIPTS)
  message(STATUS "Installing shell scripts.")
  set(SHELL_SCRIPT_DIR "${PROJECT_SOURCE_DIR}/script/util/")
  set(SVTK_SHELL_SCRIPT_SOURCES
    "${SHELL_SCRIPT_DIR}/projxy.sh"
    "${SHELL_SCRIPT_DIR}/projxz.sh"
    "${SHELL_SCRIPT_DIR}/pvd.sh"
    )
  install(PROGRAMS ${SVTK_SHELL_SCRIPT_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

# command line utilities
if (UNIX)
  set(SVTK_UTILITIES OFF CACHE BOOL "Build command line utilities.")
  if (SVTK_UTILITIES)
    message(STATUS "Building utilities.")
    include(FindMPI)
    if (NOT MPI_FOUND)
      message(SEND_ERROR "MPI is required for the gentp utility.")
    endif(NOT MPI_FOUND)
    add_definitions("-DMPICH_IGNORE_CXX_SEEK")

    add_executable(gentp gentp.cpp FsUtils.cxx)
    target_link_libraries(gentp ${MPI_LIBRARIES})
    install(TARGETS gentp DESTINATION ${CMAKE_INSTALL_PREFIX})

    add_executable(genzero genzero.cpp)
    install(TARGETS genzero DESTINATION ${CMAKE_INSTALL_PREFIX})

    add_executable(MagnetosphereTopoBatch MagnetosphereTopoBatch.cpp XMLUtils.cxx)
    target_link_libraries(MagnetosphereTopoBatch ${MPI_LIBRARIES} SQToolkit)
    install(TARGETS MagnetosphereTopoBatch DESTINATION ${CMAKE_INSTALL_PREFIX})

    add_executable(Slicer Slicer.cpp)
    target_link_libraries(Slicer ${MPI_LIBRARIES} SQToolkit)
    install(TARGETS Slicer DESTINATION ${CMAKE_INSTALL_PREFIX})

    #add_executable(IOBench IOBench.cpp)
    #target_link_libraries(IOBench ${MPI_LIBRARIES} SQToolkit)
    #install(TARGETS IOBench DESTINATION ${CMAKE_INSTALL_PREFIX})

    add_executable(MagnetosphereTopoBatchPVD MagnetosphereTopoBatchPVD.cpp XMLUtils.cxx)
    target_link_libraries(MagnetosphereTopoBatchPVD ${MPI_LIBRARIES} SQToolkit)
    install(TARGETS MagnetosphereTopoBatchPVD DESTINATION ${CMAKE_INSTALL_PREFIX})

  endif()
endif()

# testing
set(SVTK_TESTING OFF CACHE BOOL "Build project tests.")
if (SVTK_TESTING)
  message(STATUS "Building tests.")
  include(FindMPI)
  if (NOT MPI_FOUND)
    message(SEND_ERROR "MPI is required for the tests.")
  endif(NOT MPI_FOUND)
  add_definitions("-DMPICH_IGNORE_CXX_SEEK")
  add_executable(TestFieldTopologyMapper TestFieldTopologyMapper.cpp)
  target_link_libraries(TestFieldTopologyMapper SQToolkit ${MPI_LIBRARIES})
  install(TARGETS TestFieldTopologyMapper DESTINATION ${CMAKE_INSTALL_PREFIX})
endif ()



