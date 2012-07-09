set (__depends)
if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND __depends vtkPVPythonSupport)
endif()
vtk_module(pvCommandLineExecutables
  GROUPS
    ParaView
  DEPENDS
    vtkPVServerManagerApplication
    ${__depends}
  TEST_DEPENDS
    vtksys
  EXCLUDE_FROM_WRAPPING
)
