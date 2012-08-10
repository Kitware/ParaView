set (__depends)
if (PARAVIEW_ENABLE_PYTHON)
  list (APPEND __depends vtkPVPythonSupport)
endif()

vtk_module(vtkCoProcessorImplementation
  GROUPS
    CoProcessing
  DEPENDS
    vtkCoProcessor
    vtkPVServerManagerApplication
    ${__depends}
  EXCLUDE_FROM_WRAPPING
)
