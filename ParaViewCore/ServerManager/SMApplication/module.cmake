vtk_module(vtkPVServerManagerApplication
  GROUPS
    ParaView
  DEPENDS
    # When creating a "custom" application, simply change this to
    # depend on the appropriate module.
    vtkPVServerManagerDefault
)
