PROJECT(VTKTclWrapping)

INCLUDE_DIRECTORIES(
  ${VTK_SOURCE_DIR}/Wrapping
  ${VTK_SOURCE_DIR}
  ${VTK_BINARY_DIR}
  ${VTK_SOURCE_DIR}/Common
  ${VTK_BINARY_DIR}/Rendering
  ${TK_INCLUDE_PATH}
  ${TCL_INCLUDE_PATH}
  )

ADD_EXECUTABLE(vtkWrapTcl 
  ${VTK_SOURCE_DIR}/Wrapping/vtkWrapTcl.c 
  ${VTK_SOURCE_DIR}/Wrapping/vtkParse.tab.c)
ADD_EXECUTABLE(vtkWrapTclInit 
  ${VTK_SOURCE_DIR}/Wrapping/vtkWrapTclInit.c)

SET(VTK_TCL_FILES
  "${ParaView_SOURCE_DIR}/VTK/Common/vtkObject.cxx"
  "${ParaView_SOURCE_DIR}/VTK/Common/vtkObjectBase.cxx")

SET(VTK_WRAP_TCL_EXE "${EXECUTABLE_OUTPUT_PATH}/vtkWrapTcl")
SET(VTK_WRAP_TCL_INIT_EXE "${EXECUTABLE_OUTPUT_PATH}/vtkWrapTclInit")
INCLUDE("${VTKTclWrapping_SOURCE_DIR}/vtkWrapTcl.cmake")
VTK_WRAP_TCL3(vtkCommonTCL vtkTCLSources "${VTK_TCL_FILES}" "")

INCLUDE_DIRECTORIES(${TCL_INCLUDE_PATH})
ADD_LIBRARY(vtkCommonTCL ${vtkTCLSources}
  "${ParaView_SOURCE_DIR}/VTK/Common/vtkTclUtil.cxx"
  "${ParaView_SOURCE_DIR}/VTK/Rendering/vtkTkRenderWidget.cxx"
  "${ParaView_SOURCE_DIR}/VTK/Rendering/vtkTkImageViewerWidget.cxx"
  )
TARGET_LINK_LIBRARIES(vtkCommonTCL 
  vtkCommon
  ${TCL_LIBRARY}
  ${TK_LIBRARY}
  )

