# vtkCommonCore is needed here because Catalyst will
# not try to automatically satisfy dependencies.
vtk_module(vtkPVCatalyst
  DEPENDS
    vtkPVServerManagerApplication
    vtkCommonCore

  TEST_DEPENDS
    vtkPVCatalystTestDriver

  TEST_LABELS
    PARAVIEW CATALYST
)
