vtk_module(vtkCoProcessor
  GROUPS
    CoProcessing
  DEPENDS
    vtkPVServerManagerCore
  COMPILE_DEPENDS
    vtkUtilitiesEncodeString
  TEST_LABELS
    PARAVIEW CATALYST
)
