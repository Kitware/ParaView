PROJECT(VTKTclWrapping)

# We only wrap subportion of VTK, so include only that
INCLUDE_DIRECTORIES(
  ${VTK_SOURCE_DIR}/Wrapping
  ${VTK_SOURCE_DIR}
  ${VTK_BINARY_DIR}
  ${VTK_SOURCE_DIR}/Common
  ${VTK_BINARY_DIR}/Rendering
  ${TK_INCLUDE_PATH}
  ${TCL_INCLUDE_PATH}
  ${TK_INTERNAL_PATH}
  )

ADD_EXECUTABLE(vtkWrapTcl 
  ${VTK_SOURCE_DIR}/Wrapping/vtkWrapTcl.c 
  ${VTK_SOURCE_DIR}/Wrapping/vtkParse.tab.c)
ADD_EXECUTABLE(vtkWrapTclInit 
  ${VTK_SOURCE_DIR}/Wrapping/vtkWrapTclInit.c)

SET(VTK_TCL_FILES
  "${ParaView_SOURCE_DIR}/VTK/Common/vtkObject.cxx"
  "${ParaView_SOURCE_DIR}/VTK/Common/vtkObjectBase.cxx")

SET(EXE_EXT "")
IF(WIN32)
  SET(EXE_EXT ".exe")
ENDIF(WIN32)

SET(MAKE_SYSTEM)
IF(CMAKE_BUILD_TOOL MATCHES "make")
  SET(MAKE_SYSTEM 1)
ENDIF(CMAKE_BUILD_TOOL MATCHES "make")

SET(CFG_INIT "/${CMAKE_CFG_INTDIR}")
IF(MAKE_SYSTEM OR UNIX)
  SET(CFG_INIT "")
ENDIF(MAKE_SYSTEM OR UNIX)

SET(PARAVIEW_VTK_WRAP_TCL_EXE 
  "${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/vtkWrapTcl${EXE_EXT}"
  CACHE INTERNAL "Location of program to do tcl wrapping")
SET(PARAVIEW_VTK_WRAP_TCL_INIT_EXE 
  "${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/vtkWrapTclInit${EXE_EXT}"
  CACHE INTERNAL "Location of program to do tcl wrapping")
SET(VTK_WRAP_TCL_EXE ${PARAVIEW_VTK_WRAP_TCL_EXE})
SET(VTK_WRAP_TCL_INIT_EXE ${PARAVIEW_VTK_WRAP_TCL_INIT_EXE})
INCLUDE("${VTKTclWrapping_SOURCE_DIR}/vtkWrapTcl.cmake")
VTK_WRAP_TCL3(vtkCommonTCL vtkTCLSources "${VTK_TCL_FILES}" "")

ADD_LIBRARY(vtkCommonTCL ${vtkTCLSources}
  "${ParaView_SOURCE_DIR}/VTK/Common/vtkTclUtil.cxx"
  "${ParaView_SOURCE_DIR}/VTK/Rendering/vtkTkRenderWidget.cxx"
  "${ParaView_SOURCE_DIR}/VTK/Rendering/vtkTkImageViewerWidget.cxx"
  )
TARGET_LINK_LIBRARIES(vtkCommonTCL 
  vtkCommon
  vtkRendering
  ${TCL_LIBRARY}
  ${TK_LIBRARY}
  )

