IF (EXECUTABLE_OUTPUT_PATH)
  SET (VTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/vtk)
ELSE (EXECUTABLE_OUTPUT_PATH)
  SET (VTK_EXECUTABLE ${VTK_BINARY_DIR}/Wrapping/Tcl/${CMAKE_CFG_INTDIR}/vtk)
ENDIF (EXECUTABLE_OUTPUT_PATH)

ADD_TEST(PrintSelf-Widgets ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${Widgets_SOURCE_DIR})

ADD_TEST(TestSetObjectMacro-Widgets ${TCL_TCLSH}
         ${KWCommon_SOURCE_DIR}/Testing/Tcl/FindString.tcl
	 "${Widgets_SOURCE_DIR}/vtk\\\\*.h"
	 "vtkSetObjectMacro")
