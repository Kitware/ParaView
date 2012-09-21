vtk_module(FortranAdaptor
  GROUPS
    CoProcessing
  DEPENDS
    vtkCoProcessorImplementation
  EXCLUDE_FROM_WRAPPING
  TEST_LABELS
    PARAVIEW CATALYST
)
