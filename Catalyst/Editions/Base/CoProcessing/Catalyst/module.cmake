# vtkCommonCore is needed here because Catalyst will
# not try to automatically satisfy dependencies.
vtk_module(vtkPVCatalyst
  DEPENDS
    vtkPVServerManagerApplication
    vtkCommonCore
  PRIVATE_DEPENDS
    vtksys

  TEST_DEPENDS
    vtkPVCatalystTestDriver

  TEST_LABELS
    PARAVIEW CATALYST
)
