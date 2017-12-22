set(_dependencies)
if (PARAVIEW_ENABLE_PYTHON)
  set(_dependencies
    vtkPython
    vtkWrappingPythonCore
    vtkPythonInterpreter)
endif ()

vtk_module(vtkClientServer
  DEPENDS
    vtkCommonCore
    ${_dependencies}
  PRIVATE_DEPENDS
    vtksys
  TEST_DEPENDS
    vtkCommonCore
    vtkTestingCore
  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVCore
)
