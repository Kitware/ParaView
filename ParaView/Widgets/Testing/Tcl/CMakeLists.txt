ADD_TEST(PrintSelf-Widgets ${TCL_TCLSH}
         ${PARAVIEW_SOURCE_DIR}/KWCommon/Testing/Tcl/PrintSelfCheck.tcl  
         ${Widgets_SOURCE_DIR})

ADD_TEST(TestSetObjectMacro-Widgets ${TCL_TCLSH}
         ${PARAVIEW_SOURCE_DIR}/KWCommon/Testing/Tcl/FindString.tcl
	 "${Widgets_SOURCE_DIR}/vtk\\\\*.h"
	 "vtkSetObjectMacro")
