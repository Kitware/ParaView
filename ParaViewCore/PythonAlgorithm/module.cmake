if (VTK_WRAP_PYTHON)
# the `if()` can be removed once we remove the if() in `vtkFiltersPython`.
# it's not needed.
vtk_module(vtkPVPythonAlgorithm
  OPTIONAL_PYTHON_LINK
  DEPENDS
    vtkPVServerImplementationCore
  PRIVATE_DEPENDS
    vtkFiltersPython
    vtkPVClientServerCoreCore
    vtkPythonInterpreter
    vtksys
    vtkWrappingPythonCore
  TEST_LABELS
    PARAVIEW
)
endif()
