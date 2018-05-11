vtk_module(vtkPVCatalyst
  DEPENDS
    vtkPVServerManagerApplication
  PRIVATE_DEPENDS
    vtkFiltersGeneral
    vtksys

  TEST_DEPENDS
    vtkPVCatalystTestDriver
    vtkTestingCore

  TEST_LABELS
    PARAVIEW CATALYST
)
