PROJECT(PVServerCommon)
INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

SET(Common_SRCS
  vtkCommandOptions.cxx
  vtkCommandOptionsXMLParser.cxx
  vtkMPIMToNSocketConnection.cxx
  vtkMPIMToNSocketConnectionPortInformation.cxx
  vtkPVArrayInformation.cxx
  vtkPVClassNameInformation.cxx
  vtkPVClientServerModule.cxx
  vtkPVCompositeDataInformation.cxx
  vtkPVCreateProcessModule.cxx
  vtkPVDataInformation.cxx
  vtkPVDataSetAttributesInformation.cxx
  vtkPVDisplayInformation.cxx
  vtkPVGenericAttributeInformation.cxx
  vtkPVInformation.cxx
  vtkPVLODPartDisplayInformation.cxx
  vtkPVMPIProcessModule.cxx
  vtkPVNumberOfOutputsInformation.cxx
  vtkPVOptions.cxx
  vtkPVOptionsXMLParser.cxx
  vtkPVProcessModule.cxx
  vtkPVProgressHandler.cxx
  vtkPVServerInformation.cxx
  vtkPVServerOptions.cxx
  vtkPVTimerInformation.cxx
  vtkProcessModule.cxx
  vtkProcessModuleGUIHelper.cxx
  vtkStringList.cxx  
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkProcessModuleGUIHelper
  vtkProcessModule
  vtkPVInformation
  vtkPVCreateProcessModule
  ABSTRACT
  )
SET_SOURCE_FILES_PROPERTIES(
  vtkCommandOptions.cxx
  vtkCommandOptionsXMLParser.cxx
  vtkPVProgressHandler.cxx
  vtkStringList.cxx
  vtkPVOptions.cxx
  vtkPVOptionsXMLParser.cxx
  vtkPVServerOptions.cxx
  vtkPVCreateProcessModule
  WRAP_EXCLUDE)

SET(_VTK_BINARY_DIR "${VTK_BINARY_DIR}")
SET(VTK_BINARY_DIR "${PVServerCommon_BINARY_DIR}")
INCLUDE(${VTK_SOURCE_DIR}/CMake/vtkExportKit.cmake)
VTK_EXPORT_KIT("PVServerCommon" "PVSERVERCOMMON" "${Common_SRCS}")
SET(VTK_BINARY_DIR "${_VTK_BINARY_DIR}")

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
VTK_MAKE_INSTANTIATOR3(vtkPVServerCommonInstantiator CommonInst_SRCS 
  "${Common_SRCS}"
  VTK_EXPORT
  "${PVServerCommon_BINARY_DIR}" "")

# Create server common library.
ADD_LIBRARY(vtkPVServerCommon ${Common_SRCS} ${CommonInst_SRCS})
TARGET_LINK_LIBRARIES(vtkPVServerCommon vtkHybridCS vtkParallelCS KWCommon vtksys)
TARGET_LINK_LIBRARIES(vtkPVServerCommon ${PARAVIEW_HDF5_LIBRARIES})
TARGET_LINK_LIBRARIES(vtkPVServerCommon vtkClientServer)

# load the ClientServer command
INCLUDE(${ParaView_SOURCE_DIR}/Utilities/VTKClientServer/vtkClientServer.cmake)
CS_INITIALIZE_WRAP()
INCLUDE_DIRECTORIES(${VTKCLIENTSERVER_INCLUDE_DIR})

CONFIGURE_FILE(${PVServerCommon_SOURCE_DIR}/vtkPVPaths.h.in
  ${PVServerCommon_BINARY_DIR}/vtkPVPaths.h
  ESCAPE_QUOTES)

# Wrap PVServerCommon
SET(KIT_WRAP_DEPS Parallel Hybrid Rendering)
SET(VTK_KITS_DIR "${ParaView_BINARY_DIR}/Servers/Common/Utilities")
PV_WRAP_VTK_CS(PVServerCommon PVSERVERCOMMON "${KIT_WRAP_DEPS}")

# Create Tcl wrappers for common classes.
IF (PARAVIEW_BUILD_GUI)
  SET(VTK_WRAP_TCL_EXE ${PARAVIEW_VTK_WRAP_TCL_EXE})
  SET(VTK_WRAP_TCL_INIT_EXE ${PARAVIEW_VTK_WRAP_TCL_INIT_EXE})
  SET(VTK_WRAP_TCL3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping")
  INCLUDE("${VTK_SOURCE_DIR}/CMake/vtkWrapTcl.cmake")
  VTK_WRAP_TCL3(
    vtkPVServerCommonTCL 
    ServerCommonTCL_SRCS 
    "${Common_SRCS};${CommonInst_SRCS}"
    ""
    )
  ADD_LIBRARY(vtkPVServerCommonTCL ${ServerCommonTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkPVServerCommonTCL vtkPVServerCommon vtkCommonTCL)
ENDIF (PARAVIEW_BUILD_GUI)

IF (PARAVIEW_WRAP_PYTHON)
  SET(VTK_WRAP_PYTHON_EXE ${PARAVIEW_VTK_WRAP_PYTHON_EXE})
  SET(VTK_WRAP_PYTHON_INIT_EXE ${PARAVIEW_VTK_WRAP_PYTHON_INIT_EXE})
  SET(VTK_WRAP_PYTHON3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping")
  INCLUDE("${VTK_SOURCE_DIR}/CMake/vtkWrapPython.cmake")
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  VTK_WRAP_PYTHON3(
    vtkPVServerCommonPython 
    ServerCommonPython_SRCS 
    "${Common_SRCS};${SMInstantiator_SRCS}"
    ""
    )
  IF(NOT WIN32)
    SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
    ADD_DEFINITIONS(-D_HPUX_SOURCE)
  ENDIF(NOT WIN32)
  ADD_LIBRARY(vtkPVServerCommonPython ${ServerCommonPython_SRCS})
  TARGET_LINK_LIBRARIES (vtkPVServerCommonPython 
    vtkPVServerCommon vtkCommonPython)
ENDIF (PARAVIEW_WRAP_PYTHON)

IF(NOT PV_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS(${PV_INSTALL_LIB_DIR} vtkPVServerCommon vtkPVServerCommonCS vtkPVServerCommonTCL)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
