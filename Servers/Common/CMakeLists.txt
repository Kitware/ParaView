PROJECT(PVServerCommon)
INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

SET(KIT PVServerCommon)
SET(KIT_UPCASE "PVSERVERCOMMON")
IF(NOT WIN32)
  SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
ENDIF(NOT WIN32)

ADD_LIBRARY(vtkPVCommandOptions
  vtkCommandOptions.cxx
  vtkCommandOptionsXMLParser.cxx
  vtkPVOptions.cxx
  vtkPVOptionsXMLParser.cxx
  vtkPVXMLParser.cxx
  vtkPVXMLElement.cxx
)
TARGET_LINK_LIBRARIES(vtkPVCommandOptions
  vtkIO)

SET(Kit_SRCS
  vtkClientConnection.cxx
  vtkConnectionIterator.cxx
  vtkMPIMToNSocketConnection.cxx
  vtkMPIMToNSocketConnectionPortInformation.cxx
  vtkMPISelfConnection.cxx
  vtkPVArrayInformation.cxx
  vtkPVClassNameInformation.cxx
  vtkPVCompositeDataInformation.cxx
  vtkPVCreateProcessModule.cxx
  vtkPVDataInformation.cxx
  vtkPVDataSetAttributesInformation.cxx
  vtkPVDisplayInformation.cxx
  vtkPVGenericAttributeInformation.cxx
  vtkPVInformation.cxx
  vtkPVLODPartDisplayInformation.cxx
  vtkPVNumberOfOutputsInformation.cxx
  vtkPVOpenGLExtensionsInformation.cxx
  vtkPVProgressHandler.cxx
  vtkPVSelectionInformation.cxx
  vtkPVServerInformation.cxx
  vtkPVServerOptions.cxx
  vtkPVServerSocket.cxx
  vtkPVTimerInformation.cxx
  vtkProcessModule.cxx
  vtkProcessModuleConnection.cxx
  vtkProcessModuleConnectionManager.cxx
  vtkProcessModuleGUIHelper.cxx
  vtkRemoteConnection.cxx
  vtkSelectionConverter.cxx
  vtkSelectionSerializer.cxx
  vtkSelfConnection.cxx
  vtkServerConnection.cxx
  vtkStringList.cxx  
  vtkUndoElement.cxx
  vtkUndoSet.cxx
  vtkUndoStack.cxx
  vtkPVClientServerIdCollectionInformation.cxx
  )

# Abstract and Wrap exclude
SET_SOURCE_FILES_PROPERTIES(
  vtkPVCreateProcessModule
  vtkPVInformation
  vtkProcessModuleConnection
  vtkProcessModuleGUIHelper
  vtkRemoteConnection
  vtkUndoElement
  ABSTRACT
  )
SET_SOURCE_FILES_PROPERTIES(
  vtkCommandOptions.cxx
  vtkCommandOptionsXMLParser.cxx
  vtkConnectionIterator.cxx
  vtkPVProgressHandler.cxx
  vtkStringList.cxx
  vtkPVOptions.cxx
  vtkPVOptionsXMLParser.cxx
  vtkPVServerOptions.cxx
  vtkPVServerSocket.cxx
  vtkPVCreateProcessModule
  WRAP_EXCLUDE
  )

INCLUDE_DIRECTORIES(
  ${TCL_INCLUDE_PATH}
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
  vtkHybridCS vtkParallelCS KWCommon vtksys
  ${PARAVIEW_HDF5_LIBRARIES}
  vtkClientServer
  vtkPVCommandOptions)

# Create Tcl wrappers for classes.
IF (PARAVIEW_BUILD_GUI)
  SET(VTK_WRAP_TCL_EXE ${PARAVIEW_VTK_WRAP_TCL_EXE})
  SET(VTK_WRAP_TCL_INIT_EXE ${PARAVIEW_VTK_WRAP_TCL_INIT_EXE})
  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapTcl.cmake")
  VTK_WRAP_TCL3(
    vtk${KIT}TCL 
    KitTCL_SRCS 
    "${Kit_SRCS}"
    ""
    )
  ADD_LIBRARY(vtk${KIT}TCL ${KitTCL_SRCS}
    )
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtk${KIT} vtkCommonTCL)
ENDIF (PARAVIEW_BUILD_GUI)

IF (PARAVIEW_ENABLE_PYTHON)
  SET(VTK_WRAP_PYTHON_EXE ${PARAVIEW_VTK_WRAP_PYTHON_EXE})
  SET(VTK_WRAP_PYTHON_INIT_EXE ${PARAVIEW_VTK_WRAP_PYTHON_INIT_EXE})
  SET(Kit_PYTHON_EXTRA_SRCS
    )
  SET(KIT_PYTHON_LIBS
    vtkCommonPythonD vtkPVServerCommon vtk${KIT}CS
    )
  # Tell vtkWrapPython.cmake to set VTK_PYTHON_LIBRARIES for us.
  SET(VTK_WRAP_PYTHON_FIND_LIBS 1)
  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapPython.cmake")
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  SET(KIT_PYTHON_DEPS)
  SET(VTK_INSTALL_NO_LIBRARIES 1)
  IF(VTKPythonWrapping_INSTALL_BIN_DIR)
    SET(VTK_INSTALL_NO_LIBRARIES)
  ENDIF(VTKPythonWrapping_INSTALL_BIN_DIR)
  SET(VTK_INSTALL_LIB_DIR "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR "${VTKPythonWrapping_INSTALL_BIN_DIR}")
  STRING(REGEX REPLACE "^/" "" VTK_INSTALL_LIB_DIR_CM24 "${VTK_INSTALL_LIB_DIR}")
  STRING(REGEX REPLACE "^/" "" VTK_INSTALL_BIN_DIR_CM24 "${VTK_INSTALL_BIN_DIR}")

  # include python wrapping from VTK
  INCLUDE(KitCommonPythonWrapBlock)
ENDIF (PARAVIEW_ENABLE_PYTHON)

IF(NOT PV_INSTALL_NO_LIBRARIES)
  SET(temp_TO_INSTALL
    vtk${KIT}CS
    vtk${KIT})
  IF (PARAVIEW_BUILD_GUI)
    SET (temp_TO_INSTALL
      ${temp_TO_INSTALL}
      vtk${KIT}TCL)
  ENDIF(PARAVIEW_BUILD_GUI)
  IF (PARAVIEW_ENABLE_PYTHON)
    SET (temp_TO_INSTALL 
      ${temp_TO_INSTALL}
      vtk${KIT}Python)
  ENDIF (PARAVIEW_ENABLE_PYTHON)
    
  IF(PV_INSTALL_HAS_CMAKE_24)
    INSTALL(TARGETS ${temp_TO_INSTALL}
      RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
      LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
      ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)
  ELSE(PV_INSTALL_HAS_CMAKE_24)
    INSTALL_TARGETS(${PV_INSTALL_LIB_DIR}
      ${temp_TO_INSTALL}
      )
  ENDIF(PV_INSTALL_HAS_CMAKE_24)

ENDIF(NOT PV_INSTALL_NO_LIBRARIES)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
