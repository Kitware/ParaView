ADD_TEST(PrintSelf-KWCommon ${TCL_TCLSH}
         ${PARAVIEW_SOURCE_DIR}/KWCommon/Testing/Tcl/PrintSelfCheck.tcl  
         ${KWCommon_SOURCE_DIR})

ADD_TEST(TestSetObjectMacro-KWCommon ${TCL_TCLSH}
         ${PARAVIEW_SOURCE_DIR}/KWCommon/Testing/Tcl/FindString.tcl
	 "${KWCommon_SOURCE_DIR}/vtk\\\\*.h"
	 "vtkSetObjectMacro")
