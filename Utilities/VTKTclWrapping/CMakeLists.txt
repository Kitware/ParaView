PROJECT(VTKTclWrapping)

INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Wrapping)

ADD_EXECUTABLE(vtkWrapTcl 
  ${VTK_SOURCE_DIR}/Wrapping/vtkWrapTcl.c 
  ${VTK_SOURCE_DIR}/Wrapping/vtkParse.tab.c)
ADD_EXECUTABLE(vtkWrapTclInit 
  ${VTK_SOURCE_DIR}/Wrapping/vtkWrapTclInit.c)

SET(VTK_WRAP_TCL_EXE "${EXECUTABLE_OUTPUT_PATH}/vtkWrapTcl")
SET(VTK_WRAP_TCL_INIT_EXE "${EXECUTABLE_OUTPUT_PATH}/vtkWrapTclInit")


SET(VTK_TCL_FILES
  "${ParaView_SOURCE_DIR}/VTK/Common/vtkObject.cxx"
  "${ParaView_SOURCE_DIR}/VTK/Common/vtkObjectBase.cxx")

INCLUDE("${VTKTclWrapping_SOURCE_DIR}/vtkWrapTcl.cmake")
VTK_WRAP_TCL3(vtkCommonTCL vtkTCLSources "${VTK_TCL_FILES}" "")

INCLUDE_DIRECTORIES(${TCL_INCLUDE_PATH})
MESSAGE("VTK tcl sources: ${vtkTCLSources}")
ADD_LIBRARY(vtkCommonTCL ${vtkTCLSources})
TARGET_LINK_LIBRARIES(vtkCommonTCL vtkCommon)

