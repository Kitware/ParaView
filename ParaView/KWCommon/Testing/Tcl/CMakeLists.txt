IF (EXECUTABLE_OUTPUT_PATH)
  SET (VTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/vtk)
ELSE (EXECUTABLE_OUTPUT_PATH)
  SET (VTK_EXECUTABLE ${VTK_BINARY_DIR}/Wrapping/Tcl/${CMAKE_CFG_INTDIR}/vtk)
ENDIF (EXECUTABLE_OUTPUT_PATH)

ADD_TEST(PrintSelf-KWCommon ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${KWCommon_SOURCE_DIR})

ADD_TEST(TestSetObjectMacro-KWCommon ${TCL_TCLSH}
         ${KWCommon_SOURCE_DIR}/Testing/Tcl/FindString.tcl
	 "${KWCommon_SOURCE_DIR}/vtk\\\\*.h"
	 "vtkSetObjectMacro")
