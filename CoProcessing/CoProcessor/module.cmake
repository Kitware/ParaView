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
  TEST_DEPENDS
    vtkTestingRendering
  EXCLUDE_FROM_WRAPPING
)
