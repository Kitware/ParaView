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
  vtkPVCreateProcessModule.cxx
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
  vtkPVCreateProcessModule
  WRAP_EXCLUDE)

SET(_VTK_BINARY_DIR "${VTK_BINARY_DIR}")
SET(VTK_BINARY_DIR "${PVCommon_BINARY_DIR}")
INCLUDE(${VTK_SOURCE_DIR}/CMake/vtkExportKit.cmake)
VTK_EXPORT_KIT("PVCommon" "PVFILTERS"
"${Common_SRCS};${ParaView_SOURCE_DIR}/Common/KWCommon/vtkCornerAnnotation.cxx")
SET(VTK_BINARY_DIR "${_VTK_BINARY_DIR}")

INCLUDE_DIRECTORIES(
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
TARGET_LINK_LIBRARIES(vtkPVCommon vtkKWCommon vtkHybridCS vtkParallelCS vtkKWCommon)
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

IF(BUILD_SHARED_LIBS)
  INSTALL_TARGETS(/lib vtkPVCommon vtkPVCommonCS)
ENDIF(BUILD_SHARED_LIBS)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
