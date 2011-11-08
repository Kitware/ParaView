# PVClientServerCore contains classes essential for client-server mechanisms in
# ParaView.
# This also includes additional extensions to VTK that depend on ProcessModule
# or sessions.

PROJECT(PVClientServerCore)

INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

# needed by vtkExportKit.cmake
SET(VTK_INSTALL_PACKAGE_DIR_CM24 ${PV_INSTALL_LIB_DIR})
SET(VTK_INSTALL_PACKAGE_DIR ${PV_INSTALL_LIB_DIR})

SET(KIT PVClientServerCore)
SET(KIT_UPCASE "PVCLIENTSERVERCORE")
IF(NOT WIN32)
  SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
ENDIF(NOT WIN32)

SET (Kit_SRCS
  vtk3DWidgetRepresentation.cxx
  vtkCaveSynchronizedRenderers.cxx
  vtkChartRepresentation.cxx
  vtkClientServerMoveData.cxx
  vtkCompleteArrays.cxx
  vtkCompositeRepresentation.cxx
  vtkCubeAxesRepresentation.cxx
  vtkDataLabelRepresentation.cxx
  vtkGeometryRepresentation.cxx
  vtkGeometryRepresentationWithFaces.cxx
  vtkGlyph3DRepresentation.cxx
  vtkImageSliceDataDeliveryFilter.cxx
  vtkPVImageSliceMapper.cxx
  vtkImageSliceRepresentation.cxx
  vtkImageVolumeRepresentation.cxx
  vtkMPIMToNSocketConnection.cxx
  vtkMPIMToNSocketConnectionPortInformation.cxx
  vtkMPIMoveData.cxx
  vtkNetworkAccessManager.cxx
  vtkNetworkImageSource.cxx
  vtkOutlineRepresentation.cxx
  vtkPV2DRenderView.cxx
  vtkPVAlgorithmPortsInformation.cxx
  vtkPVArrayInformation.cxx
  vtkPVBarChartView.cxx
  vtkPVCacheKeeper.cxx
  vtkPVCacheKeeperPipeline.cxx
  vtkPVCacheSizeInformation.cxx
  vtkPVCameraAnimationCue.cxx
  vtkPVCameraCueManipulator.cxx
  vtkPVCameraKeyFrame.cxx
  vtkPVClassNameInformation.cxx
  vtkPVClientServerSynchronizedRenderers.cxx
  vtkPVCompositeDataInformation.cxx
  vtkPVCompositeDataInformationIterator.cxx
  vtkPVCompositeRepresentation.cxx
  vtkPVContextView.cxx
  vtkPVDataInformation.cxx
  vtkPVDataRepresentation.cxx
  vtkPVDataRepresentationPipeline.cxx
  vtkPVDataSetAttributesInformation.cxx
  vtkPVDataSizeInformation.cxx
  vtkPVDisplayInformation.cxx
  vtkPVEnvironmentInformation.cxx
  vtkPVEnvironmentInformationHelper.cxx
  vtkPVFileInformation.cxx
  vtkPVFileInformationHelper.cxx
  vtkPVGenericAttributeInformation.cxx
  vtkPVImplicitPlaneRepresentation.cxx
  vtkPVInformation.cxx
  vtkPVLastSelectionInformation.cxx
  vtkPVLineChartView.cxx
  vtkPVOpenGLExtensionsInformation.cxx
  vtkPVOptions.cxx
  vtkPVOptionsXMLParser.cxx
  vtkPVParallelCoordinatesChartView.cxx
  vtkPVParallelCoordinatesRepresentation.cxx
  vtkPVPlugin.cxx
  vtkPVPluginLoader.cxx
  vtkPVPluginTracker.cxx
  vtkPVPluginsInformation.cxx
  vtkPVProgressHandler.cxx
  vtkPVPythonModule.cxx
  vtkPVPythonPluginInterface.cxx
  vtkPVRenderView.cxx
  vtkPVRepresentedDataInformation.cxx
  vtkPVSILInformation.cxx
  vtkPVSelectionInformation.cxx
  vtkPVServerInformation.cxx
  vtkPVServerManagerPluginInterface.cxx
  vtkPVServerOptions.cxx
  vtkPVSession.cxx
  vtkPVSynchronizedRenderWindows.cxx
  vtkPVSynchronizedRenderer.cxx
  vtkPVTemporalDataInformation.cxx
  vtkPVTimerInformation.cxx
  vtkPVView.cxx
  vtkPVXYChartView.cxx
  vtkProcessModule.cxx
  vtkProcessModuleAutoMPI.cxx
  vtkSelectionDeliveryFilter.cxx
  vtkSelectionRepresentation.cxx
  vtkSession.cxx
  vtkSessionIterator.cxx
  vtkSpreadSheetRepresentation.cxx
  vtkSpreadSheetView.cxx
  vtkTCPNetworkAccessManager.cxx
  vtkTextSourceRepresentation.cxx
  vtkUnstructuredDataDeliveryFilter.cxx
  vtkUnstructuredGridVolumeRepresentation.cxx
  vtkXMLPVAnimationWriter.cxx
  vtkXYChartRepresentation.cxx
)

IF (PARAVIEW_USE_ICE_T AND VTK_USE_MPI)
  SET (Kit_SRCS
    ${Kit_SRCS}
    vtkIceTSynchronizedRenderers.cxx)
  INCLUDE_DIRECTORIES(${ICET_INCLUDE_DIR})
  SET(KIT_LIBS ${KIT_LIBS} IceTCore IceTMPI IceTGL)

  # The following was copy/paste from vtkIncludeDirectories.cmake
  # OpenGL include directories.
  IF(APPLE)
    IF(VTK_USE_X OR VTK_OPENGL_HAS_OSMESA)
      INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
    ENDIF(VTK_USE_X OR VTK_OPENGL_HAS_OSMESA)
  ELSE(APPLE)
    INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
  ENDIF(APPLE)
ENDIF (PARAVIEW_USE_ICE_T AND VTK_USE_MPI)

IF(PARAVIEW_ENABLE_PYTHON)
  SET(Kit_SRCS ${Kit_SRCS}
    vtkPythonAnimationCue.cxx
    vtkPythonCalculator.cxx
    vtkPythonProgrammableFilter.cxx
    )
  SET (KIT_LIBS ${KIT_LIBS} ${PYTHON_LIBRARIES} vtkPVPythonInterpretor)
  INCLUDE_DIRECTORIES(
    ${PYTHON_INCLUDE_PATH}
    ${VTKPythonWrapping_SOURCE_DIR}/Executable)
ENDIF(PARAVIEW_ENABLE_PYTHON)


SET_SOURCE_FILES_PROPERTIES(
  vtkProcessModuleAutoMPI.cxx
  vtkPVOptionsXMLParser.cxx
  vtkPVPlugin.cxx
  vtkPVServerOptions.cxx
  vtkPVServerManagerPluginInterface.cxx
  vtkPVPythonPluginInterface.cxx

  WRAP_EXCLUDE
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkNetworkAccessManager.cxx
  vtkPVContextView.cxx
  vtkPVDataRepresentation.cxx
  vtkPVInformation.cxx
  vtkPVPlugin.cxx
  vtkPVPluginTracker.cxx
  vtkPVPythonPluginInterface.cxx
  vtkPVServerManagerPluginInterface.cxx
  vtkPVSession.cxx
  vtkPVView.cxx
  vtkSession.cxx
  vtkTileDisplayHelper.cxx
  ABSTRACT)

INCLUDE_DIRECTORIES(
  ${PVCommon_SOURCE_DIR}
  ${PVCommon_BINARY_DIR}
  ${PVVTKExtensions_SOURCE_DIR}
  ${PVVTKExtensions_BINARY_DIR}
  ${PVClientServerCore_SOURCE_DIR}
  ${PVClientServerCore_BINARY_DIR}
  ${VTK_INCLUDE_DIR}
  ${VTK_ZLIB_INCLUDE_DIRS}
  )

IF (VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
  SET (Kit_SRCS ${Kit_SRCS} vtkPVMPICommunicator.cxx)
  SET_SOURCE_FILES_PROPERTIES(
    vtkPVMPICommunicator.cxx
    WRAP_EXCLUDE)
  IF (MPI_LIBRARY)
    SET(KIT_LIBS ${KIT_LIBS} "${MPI_LIBRARY}")
  ELSE (MPI_LIBRARY)
    MESSAGE("Could not find the required MPI libraries")
  ENDIF (MPI_LIBRARY)
  IF (MPI_EXTRA_LIBRARY)
    SET(KIT_LIBS ${KIT_LIBS} "${MPI_EXTRA_LIBRARY}")
  ENDIF (MPI_EXTRA_LIBRARY)
ENDIF (VTK_USE_MPI)

# for vtkPVFileInformation
IF(WIN32)
  SET(KIT_LIBS ${KIT_LIBS} mpr)
ENDIF(WIN32)
IF(MINGW)
  SET(KIT_LIBS ${KIT_LIBS} uuid ole32)
ENDIF(MINGW)
IF(APPLE)
  SET(KIT_LIBS ${KIT_LIBS} "-framework CoreServices")
ENDIF(APPLE)

SET(VTK_WRAP_HINTS ${CMAKE_CURRENT_SOURCE_DIR}/hints)

# Setup vtkInstantiator registration for this library's classes.
INCLUDE(${ParaView_SOURCE_DIR}/VTK/CMake/vtkMakeInstantiator.cmake)
VTK_MAKE_INSTANTIATOR3(vtk${KIT}Instantiator KitInstantiator_SRCS
  "${Kit_SRCS}"
  VTK_EXPORT
  "${CMAKE_CURRENT_BINARY_DIR}" "")

# load the ClientServer command
INCLUDE(${ParaView_SOURCE_DIR}/Utilities/VTKClientServer/vtkClientServer.cmake)
CS_INITIALIZE_WRAP()
INCLUDE_DIRECTORIES(${VTKCLIENTSERVER_INCLUDE_DIR})

# Wrap PVServer
# Create ClientServer wrappers for ParaView classes.
SET(_VTK_BINARY_DIR "${VTK_BINARY_DIR}")
SET(VTK_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
INCLUDE(${VTK_CMAKE_DIR}/vtkExportKit.cmake)
VTK_EXPORT_KIT("${KIT}" "${KIT_UPCASE}" "${Kit_SRCS}")
SET(VTK_BINARY_DIR "${_VTK_BINARY_DIR}")

# Wrap PVServerCommon
SET(KIT_WRAP_DEPS PVVTKExtensions)
SET(VTK_KITS_DIR "${CMAKE_CURRENT_BINARY_DIR}/Utilities")
PV_WRAP_VTK_CS("${KIT}" "${KIT_UPCASE}" "${KIT_WRAP_DEPS}")

# Wrap for python
IF (PARAVIEW_ENABLE_PYTHON)
  SET(Kit_PYTHON_EXTRA_SRCS)
  SET(KIT_PYTHON_LIBS
    vtkViewsPythonD
    vtkPVVTKExtensionsPythonD
    )
  # Tell vtkWrapPython.cmake to set VTK_PYTHON_LIBRARIES for us.
  SET(VTK_WRAP_PYTHON_FIND_LIBS 1)
  SET(VTK_WRAP_INCLUDE_DIRS ${VTK_INCLUDE_DIR})
  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapPython.cmake")
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  SET(KIT_PYTHON_DEPS)

  SET(VTK_INSTALL_LIB_DIR "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR "${VTKPythonWrapping_INSTALL_BIN_DIR}")
  SET(VTK_INSTALL_LIB_DIR_CM24 "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR_CM24 "${VTKPythonWrapping_INSTALL_BIN_DIR}")

  # include python wrapping from VTK
  INCLUDE(KitCommonPythonWrapBlock)
ENDIF (PARAVIEW_ENABLE_PYTHON)



# Create the library.
VTK_ADD_LIBRARY(vtk${KIT}
  ${Kit_SRCS}
  ${KitInstantiator_SRCS}
  )
TARGET_LINK_LIBRARIES(vtk${KIT}
  vtkPVVTKExtensions
  vtkPVVTKExtensionsCS
  ${KIT_LIBS}
)

IF(NOT PV_INSTALL_NO_LIBRARIES)
  SET(temp_TO_INSTALL
    vtk${KIT})

  INSTALL(TARGETS ${temp_TO_INSTALL}
    EXPORT ${PV_INSTALL_EXPORT_NAME}
    RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
    LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)

# Development files install
IF (NOT PV_INSTALL_NO_DEVELOPMENT)
  GLOB_INSTALL_DEVELOPMENT(
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${PV_INSTALL_INCLUDE_DIR}
      "*.h;*.hxx;*.txx")
ENDIF (NOT PV_INSTALL_NO_DEVELOPMENT)

IF (BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF (BUILD_TESTING)
