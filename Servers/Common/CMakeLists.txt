PROJECT(PVServerCommon)
INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

# needed by vtkExportKit.cmake
SET(VTK_INSTALL_PACKAGE_DIR_CM24 ${PV_INSTALL_LIB_DIR})
SET(VTK_INSTALL_PACKAGE_DIR ${PV_INSTALL_LIB_DIR})

# Any build configured headers should be added to this 
# variable so that they also get installed.
SET (dynamicHeaders)


SET(KIT PVServerCommon)
SET(KIT_UPCASE "PVSERVERCOMMON")
IF(NOT WIN32)
  SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
ENDIF(NOT WIN32)

SET (PVCommandOptions_SRCS
  vtkCommandOptions.cxx
  vtkCommandOptionsXMLParser.cxx
  vtkPVXMLParser.cxx
  vtkPVXMLElement.cxx
)

ADD_LIBRARY(vtkPVCommandOptions ${PVCommandOptions_SRCS})
TARGET_LINK_LIBRARIES(vtkPVCommandOptions
  vtkIO)

SET(Kit_SRCS
  vtkCacheSizeKeeper.cxx
  vtkCellIntegrator.cxx
  vtkClientConnection.cxx
  vtkConnectionIterator.cxx
  vtkFileSequenceParser.cxx
  vtkMPISelfConnection.cxx
  vtkMultiProcessControllerHelper.cxx
  vtkPriorityHelper.cxx
  vtkProcessModuleConnection.cxx
  vtkProcessModuleConnectionManager.cxx
  vtkProcessModule.cxx
  vtkProcessModuleGUIHelper.cxx
  vtkPVAlgorithmPortsInformation.cxx
  vtkPVArrayInformation.cxx
  vtkPVCacheSizeInformation.cxx
  vtkPVClassNameInformation.cxx
  vtkPVClientServerIdCollectionInformation.cxx
  vtkPVCompositeDataInformation.cxx
  vtkPVCompositeDataInformationIterator.cxx
  vtkPVDataInformation.cxx
  vtkPVDataSetAttributesInformation.cxx
  vtkPVDataSizeInformation.cxx
  vtkPVDisplayInformation.cxx
  vtkPVEnvironmentInformation.cxx
  vtkPVEnvironmentInformationHelper.cxx
  vtkPVFileInformation.cxx
  vtkPVFileInformationHelper.cxx
  vtkPVGenericAttributeInformation.cxx
  vtkPVInformation.cxx
  vtkPVOpenGLExtensionsInformation.cxx
  vtkPVOptions.cxx
  vtkPVOptionsXMLParser.cxx
  vtkPVPlugin.cxx
  vtkPVPluginInformation.cxx
  vtkPVPluginLoader.cxx
  vtkPVProgressHandler.cxx
  vtkPVPythonModule.cxx
  vtkPVPythonPluginInterface.cxx
  vtkPVSelectionInformation.cxx
  vtkPVServerInformation.cxx
  vtkPVServerManagerPluginInterface.cxx
  vtkPVServerOptions.cxx
  vtkPVSILInformation.cxx
  vtkPVTemporalDataInformation.cxx
  vtkPVTestUtilities.cxx
  vtkPVTimerInformation.cxx
  vtkRemoteConnection.cxx
  vtkSelectionSerializer.cxx
  vtkSelfConnection.cxx
  vtkServerConnection.cxx
  vtkStringList.cxx  
  vtkSynchronousMPISelfConnection.cxx
  vtkUndoElement.cxx
  vtkUndoSet.cxx
  vtkUndoStack.cxx

# these files seem to be socket related, so for Cray Xt3/Catamount they probably have to 
# be excluded, e.g. via IF(HAVE_SOCKETS) coming from VTK/Parallel/VTKParallelCMakeTests.cmake, Alex
    vtkMPIMToNSocketConnection.cxx
    vtkMPIMToNSocketConnectionPortInformation.cxx
    vtkPVServerSocket.cxx )
  
IF (VTK_USE_N_WAY_ARRAYS)

ENDIF (VTK_USE_N_WAY_ARRAYS)

IF (VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
  SET (Kit_SRCS ${Kit_SRCS}
    vtkPVMPICommunicator.cxx)
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

# Abstract and Wrap exclude
SET_SOURCE_FILES_PROPERTIES(
  vtkCellIntegrator.cxx
  vtkProcessModuleConnection
  vtkProcessModuleGUIHelper
  vtkPVCreateProcessModule
  vtkPVInformation
  vtkPVPlugin.cxx
  vtkRemoteConnection
  vtkUndoElement
  vtkPVServerManagerPluginInterface
  vtkPVPythonPluginInterface
  ABSTRACT
  )
SET_SOURCE_FILES_PROPERTIES(
  vtkCommandOptionsXMLParser.cxx
  vtkConnectionIterator.cxx
  vtkPVCreateProcessModule
  vtkPVOptionsXMLParser.cxx
  vtkPVPlugin.cxx
  vtkPVServerOptions.cxx
  vtkPVServerSocket.cxx
  vtkPVTestUtilities.cxx
  vtkPVServerManagerPluginInterface.cxx
  vtkPVPythonPluginInterface.cxx
  # vtkStringList.cxx
  WRAP_EXCLUDE
  )

INCLUDE_DIRECTORIES(
  ${PVServerCommon_BINARY_DIR}
  ${PVServerCommon_SOURCE_DIR}
  ${HDF5_INCLUDE_DIR}
  ${VTK_INCLUDE_DIR}
  ${KWCommon_INCLUDE_PATH}
  )

IF (VTK_USE_X)
  FIND_PACKAGE(X11)
  INCLUDE_DIRECTORIES(${X11_INCLUDE_DIR})
ENDIF (VTK_USE_X)

SET(VTK_WRAP_HINTS ${CMAKE_CURRENT_SOURCE_DIR}/hints)

# Include PVCommandOptions_SRCS in the Kit_SRCS -nix
SET (Kit_SRCS
      ${Kit_SRCS}
      ${PVCommandOptions_SRCS} 
    )
  
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

SET (dynamicHeaders "${dynamicHeaders};${PVServerCommon_BINARY_DIR}/vtkPVPaths.h")
CONFIGURE_FILE(${PVServerCommon_SOURCE_DIR}/vtkPVPaths.h.in
  ${PVServerCommon_BINARY_DIR}/vtkPVPaths.h
  ESCAPE_QUOTES)

# Wrap PVServerCommon
SET(KIT_WRAP_DEPS Parallel Hybrid Rendering)
SET(VTK_KITS_DIR "${CMAKE_CURRENT_BINARY_DIR}/Utilities")
PV_WRAP_VTK_CS("${KIT}" "${KIT_UPCASE}" "${KIT_WRAP_DEPS}")

# Create the library.
ADD_LIBRARY(vtk${KIT}
  ${Kit_SRCS}
  ${KitInstantiator_SRCS}
  )
TARGET_LINK_LIBRARIES(vtk${KIT}
  vtkHybridCS vtkParallelCS vtkInfovisCS KWCommon vtksys
  ${PARAVIEW_HDF5_LIBRARIES}
  ${KIT_LIBS}
  vtkClientServer
  vtkPVCommandOptions)

# for vtkPVFileInformation
IF(WIN32)
  TARGET_LINK_LIBRARIES(vtk${KIT} mpr)
ENDIF(WIN32)
IF(MINGW)
  TARGET_LINK_LIBRARIES(vtk${KIT} uuid ole32)
ENDIF(MINGW)
IF(APPLE)
  TARGET_LINK_LIBRARIES (vtk${KIT} "-framework CoreServices")
ENDIF(APPLE)

IF (PARAVIEW_ENABLE_PYTHON)
  SET(Kit_PYTHON_EXTRA_SRCS)

#    SET (Kit_SRCS
#       ${Kit_SRCS}
#       ${PVCommandOptions_SRCS} 
#     )
  SET(KIT_PYTHON_LIBS
    vtkCommonPythonD vtkFilteringPythonD vtkIOPythonD vtkPVServerCommon vtk${KIT}CS
    )
  # Tell vtkWrapPython.cmake to set VTK_PYTHON_LIBRARIES for us.
  SET(VTK_WRAP_PYTHON_FIND_LIBS 1)
  SET(VTK_WRAP_INCLUDE_DIRS ${VTK_INCLUDE_DIR})
  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapPython.cmake")
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  SET(KIT_PYTHON_DEPS)
  SET(VTK_INSTALL_NO_LIBRARIES 1)
  IF(VTKPythonWrapping_INSTALL_BIN_DIR)
    SET(VTK_INSTALL_NO_LIBRARIES)
  ENDIF(VTKPythonWrapping_INSTALL_BIN_DIR)

  SET(VTK_INSTALL_LIB_DIR "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR "${VTKPythonWrapping_INSTALL_BIN_DIR}")
  SET(VTK_INSTALL_LIB_DIR_CM24 "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR_CM24 "${VTKPythonWrapping_INSTALL_BIN_DIR}")

  # include python wrapping from VTK
  INCLUDE(KitCommonPythonWrapBlock)
ENDIF (PARAVIEW_ENABLE_PYTHON)

IF(NOT PV_INSTALL_NO_LIBRARIES)
  SET(temp_TO_INSTALL
    vtk${KIT}CS
    vtk${KIT}
    vtkPVCommandOptions)
  IF (PARAVIEW_ENABLE_PYTHON)
    SET (temp_TO_INSTALL 
      ${temp_TO_INSTALL}
      vtk${KIT}Python)
  ENDIF (PARAVIEW_ENABLE_PYTHON)

  INSTALL(TARGETS ${temp_TO_INSTALL}
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
  INSTALL(
      FILES ${dynamicHeaders}
      DESTINATION ${PV_INSTALL_INCLUDE_DIR}
      COMPONENT Development)
ENDIF (NOT PV_INSTALL_NO_DEVELOPMENT)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
