PROJECT(PVCommon)
INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

SET(Common_SRCS
  vtkPVRenderModule.cxx
  vtkPVProcessModule.cxx
  vtkPVMPIProcessModule.cxx
  vtkPVClientServerModule.cxx
  vtkProcessModule.cxx
  vtkProcessModuleGUIHelper.cxx
  vtkStringList.cxx  
  vtkPVInformation.cxx
  vtkPVTimerInformation.cxx
  vtkPVNumberOfOutputsInformation.cxx
  vtkPVLODPartDisplayInformation.cxx
  vtkPVDataSetAttributesInformation.cxx
  vtkPVDataInformation.cxx
  vtkPVArrayInformation.cxx
  vtkPVClassNameInformation.cxx
  vtkPVServerInformation.cxx
  vtkMPIMToNSocketConnectionPortInformation.cxx
  vtkMPIMToNSocketConnection.cxx
  vtkPVProgressHandler.cxx
  vtkPVOptions.cxx
  vtkPVServerOptions.cxx
  vtkPVOptionsXMLParser.cxx
  vtkPVCreateProcessModule.cxx
  vtkPVGenericAttributeInformation.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkPVRenderModule
  vtkProcessModuleGUIHelper
  vtkProcessModule
  vtkPVInformation
  vtkPVCreateProcessModule
  ABSTRACT
  )
SET_SOURCE_FILES_PROPERTIES(
  vtkPVProgressHandler.cxx
  vtkStringList.cxx
  vtkPVOptions.cxx
  vtkPVOptionsXMLParser.cxx
  vtkPVServerOptions.cxx
  vtkPVCreateProcessModule
  WRAP_EXCLUDE)

SET(_VTK_BINARY_DIR "${VTK_BINARY_DIR}")
SET(VTK_BINARY_DIR "${PVCommon_BINARY_DIR}")
INCLUDE(${VTK_SOURCE_DIR}/CMake/vtkExportKit.cmake)
VTK_EXPORT_KIT("PVCommon" "PVFILTERS" "${Common_SRCS}")
SET(VTK_BINARY_DIR "${_VTK_BINARY_DIR}")

INCLUDE_DIRECTORIES(
  ${TCL_INCLUDE_PATH}
  ${KWSYS_INCLUDE_DIR}
  ${PVCommon_BINARY_DIR}
  ${PVCommon_SOURCE_DIR}
  ${KWCOMMON_INCLUDE_DIR}
  ${HDF5_INCLUDE_DIR}
  ${VTK_INCLUDE_DIR}
  )

# Setup vtkInstantiator registration for this library's classes.
INCLUDE(${ParaView_SOURCE_DIR}/VTK/CMake/vtkMakeInstantiator.cmake)
VTK_MAKE_INSTANTIATOR3(vtkPVCommonInstantiator CommonInst_SRCS 
  "${Common_SRCS}"
  VTK_EXPORT
  "${PVCommon_BINARY_DIR}" "")

# Create filters library.
ADD_LIBRARY(vtkPVCommon ${Common_SRCS} ${CommonInst_SRCS})
TARGET_LINK_LIBRARIES(vtkPVCommon vtkHybridCS vtkParallelCS KWCommon)
IF(KWSYS_NAMESPACE)
  TARGET_LINK_LIBRARIES (vtkPVCommon ${KWSYS_NAMESPACE})
ENDIF(KWSYS_NAMESPACE)
IF (VTK_USE_PATENTED)
  TARGET_LINK_LIBRARIES(vtkPVCommon vtkPatented)
ENDIF (VTK_USE_PATENTED)
TARGET_LINK_LIBRARIES(vtkPVCommon ${PARAVIEW_HDF5_LIBRARIES})
TARGET_LINK_LIBRARIES(vtkPVCommon vtkClientServer)

# load the ClientServer command
INCLUDE(${ParaView_SOURCE_DIR}/Utilities/VTKClientServer/vtkClientServer.cmake)
CS_INITIALIZE_WRAP()
INCLUDE_DIRECTORIES(${VTKCLIENTSERVER_INCLUDE_DIR})

CONFIGURE_FILE(${PVCommon_SOURCE_DIR}/vtkPVDemoPaths.h.in
  ${PVCommon_BINARY_DIR}/vtkPVDemoPaths.h
  ESCAPE_QUOTES)

# Wrap PVCommon
SET(KIT_WRAP_DEPS Parallel Hybrid Rendering)
IF(VTK_USE_PATENTED)
  SET(KIT_WRAP_DEPS Patented)
ENDIF(VTK_USE_PATENTED)
SET(VTK_KITS_DIR "${ParaView_BINARY_DIR}/Servers/Common/Utilities")
PV_WRAP_VTK_CS(PVCommon PVFILTERS "${KIT_WRAP_DEPS}")

# Create Tcl wrappers for common classes.
IF (PARAVIEW_BUILD_GUI)
  SET(VTK_WRAP_TCL_EXE ${PARAVIEW_VTK_WRAP_TCL_EXE})
  SET(VTK_WRAP_TCL_INIT_EXE ${PARAVIEW_VTK_WRAP_TCL_INIT_EXE})
  SET(VTK_WRAP_TCL3_INIT_DIR "${VTKTclWrapping_SOURCE_DIR}")
  INCLUDE("${VTKTclWrapping_SOURCE_DIR}/vtkWrapTcl.cmake")
  VTK_WRAP_TCL3(
    vtkPVServerCommonTCL 
    ServerCommonTCL_SRCS 
    "${Common_SRCS};${CommonInst_SRCS}"
    ""
    )
  ADD_LIBRARY(vtkPVServerCommonTCL ${ServerCommonTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkPVServerCommonTCL vtkPVCommon vtkCommonTCL)
ENDIF (PARAVIEW_BUILD_GUI)

IF(BUILD_SHARED_LIBS)
  INSTALL_TARGETS(${PV_INSTALL_LIB_DIR} vtkPVCommon vtkPVCommonCS vtkPVServerCommonTCL)
ENDIF(BUILD_SHARED_LIBS)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
