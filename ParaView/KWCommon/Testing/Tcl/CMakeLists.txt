IF(VTK_PRINT_SELF_CHECK_TCL)
  ADD_TEST(PrintSelf-KWCommon ${TCL_TCLSH}
           ${VTK_PRINT_SELF_CHECK_TCL}
           ${KWCommon_SOURCE_DIR})
ENDIF(VTK_PRINT_SELF_CHECK_TCL)

IF(VTK_FIND_STRING_TCL)
  ADD_TEST(TestSetObjectMacro-KWCommon ${TCL_TCLSH}
           ${VTK_FIND_STRING_TCL}
           "${KWCommon_SOURCE_DIR}/vtk\\\\*.h"
           "vtkSetObjectMacro")
ENDIF(VTK_FIND_STRING_TCL)
