vtk_module(vtkPVServerManagerApplication
  DEPENDS
    # When creating a "custom" application, simply change this to
    # depend on the appropriate module.
    vtkPVServerManagerDefault
  COMPILE_DEPENDS
    vtkUtilitiesProcessXML
)
