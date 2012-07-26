set (__depends)
if (PARAVIEW_ENABLE_PYTHON)
  list (APPEND __depends vtkPVPythonSupport)
endif()

vtk_module(vtkCoProcessor
  GROUPS
    ParaViewCoProcessor
  DEPENDS
    vtkCoProcessorCore
    vtkPVServerManagerApplication
    ${__depends}
  EXCLUDE_FROM_WRAPPING
)
