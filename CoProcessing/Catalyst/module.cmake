vtk_module(vtkPVCatalyst
  DEPENDS
    vtkPVServerManagerApplication
  PRIVATE_DEPENDS
    vtksys

  TEST_DEPENDS
    vtkPVCatalystTestDriver

  TEST_LABELS
    PARAVIEW CATALYST
)
