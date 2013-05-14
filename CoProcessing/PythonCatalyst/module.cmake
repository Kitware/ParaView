vtk_module(vtkPVPythonCatalyst
  DEPENDS
    vtkPVCatalyst
    vtkPythonInterpreter
  TEST_DEPENDS
    vtkIOImage
    vtkTestingRendering
    vtkPVCatalystTestDriver
  TEST_LABELS
    PARAVIEW CATALYST
)
