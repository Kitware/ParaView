PROJECT(PVClientTests)

SET(PVClientTestData ${ParaView_SOURCE_DIR}/Data)
SET(PVCompareImages ${ParaView_SOURCE_DIR}/GUI/Widgets/Testing/Tcl/CompareImages.tcl)
SUBDIRS(Tcl)

IF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)
  FOREACH(part Client)
    ADD_TEST(HeaderTesting-PVGUI-${part} ${PYTHON_EXECUTABLE}
      ${VTK_HEADER_TESTING_PY}
      "${PVGUI_SOURCE_DIR}/${part}"
      pvTestDriver.h
      ParaViewRC.h
      )
  ENDFOREACH(part)
ENDIF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)

IF(TCL_TCLSH)
  IF(VTK_PRINT_SELF_CHECK_TCL)
    FOREACH(part Client)
      ADD_TEST(PrintSelf-PVGUI-${part} ${TCL_TCLSH}
        ${VTK_PRINT_SELF_CHECK_TCL}
        ${PVGUI_SOURCE_DIR}/${part})
    ENDFOREACH(part)
  ENDIF(VTK_PRINT_SELF_CHECK_TCL)

  IF(VTK_FIND_STRING_TCL)
    FOREACH(part Client)
      ADD_TEST(TestSetObjectMacro-PVGUI-${part} ${TCL_TCLSH}
        ${VTK_FIND_STRING_TCL}
        "${PVGUI_SOURCE_DIR}/${part}/vtk\\\\*.h"
        "vtkSetObjectMacro")
    ENDFOREACH(part)
  ENDIF(VTK_FIND_STRING_TCL)
ENDIF(TCL_TCLSH)
