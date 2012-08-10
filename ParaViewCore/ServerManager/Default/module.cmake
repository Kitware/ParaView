vtk_module(vtkPVServerManagerDefault
  DEPENDS
    vtkPVServerImplementationDefault
    vtkPVServerManagerRendering
    vtkTestingRendering

  TEST_DEPENDS
    vtkPVServerManagerApplication
)
