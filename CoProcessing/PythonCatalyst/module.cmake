vtk_module(vtkPVPythonCatalyst
  DEPENDS
    vtkPVCatalyst
    vtkPVPythonSupport
  TEST_DEPENDS
    vtkIOImage
    vtkTestingRendering
    vtkPVCatalystTestDriver
  TEST_LABELS
    PARAVIEW CATALYST
  COMPILE_DEPENDS
    vtkUtilitiesEncodeString
)
