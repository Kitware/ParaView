IF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)
  FOREACH(part Common Filters ServerManager)
    ADD_TEST(HeaderTesting-PVServer-${part} ${PYTHON_EXECUTABLE}
      ${VTK_HEADER_TESTING_PY}
      "${PVServer_SOURCE_DIR}/${part}"
      vtkPVServerOptionsInternals.h
      vtkSMPropertyInternals.h
      vtkSMProxyInternals.h
      vtkSMPropertyInternals.h
      vtkSMProxyManagerInternals.h      
      )
  ENDFOREACH(part)
ENDIF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)

IF(TCL_TCLSH)
  IF(VTK_PRINT_SELF_CHECK_TCL)
    FOREACH(part Common Filters ServerManager)
      ADD_TEST(PrintSelf-PVServer-${part} ${TCL_TCLSH}
        ${VTK_PRINT_SELF_CHECK_TCL}
        ${PVServer_SOURCE_DIR}/${part})
    ENDFOREACH(part)
  ENDIF(VTK_PRINT_SELF_CHECK_TCL)

  IF(VTK_FIND_STRING_TCL)
    FOREACH(part Common Filters ServerManager)
      ADD_TEST(TestSetObjectMacro-PVServer-${part} ${TCL_TCLSH}
        ${VTK_FIND_STRING_TCL}
        "${PVServer_SOURCE_DIR}/${part}/vtk\\\\*.h"
        "vtkSetObjectMacro")
    ENDFOREACH(part)
  ENDIF(VTK_FIND_STRING_TCL)
ENDIF(TCL_TCLSH)
